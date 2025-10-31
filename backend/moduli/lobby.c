#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdbool.h>
#include <libpq-fe.h>
#include <pthread.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <time.h>
#include <unistd.h>
#include <ctype.h>
#include <errno.h>
#include "dbConnection.h"
#include "lobby.h"

#define MIN_PLAYERS 4
#define MAX_PLAYERS 8
#define LOBBY_ID_LENGTH 6
#define MAX_ID_GENERATION_ATTEMPTS 10

#define JSON_BUFFER_SMALL 512
#define JSON_BUFFER_MEDIUM 1024
#define JSON_BUFFER_LARGE 8192

#define DEFAULT_DB_HOST "db"
#define DEFAULT_DB_NAME "deltalabtsf"
#define DEFAULT_DB_USER "postgres"
#define DEFAULT_DB_PASS "admin"
#define DB_CONNECTION_TIMEOUT 30
#define DB_QUERY_TIMEOUT 10

#define STATUS_WAITING "waiting"
#define STATUS_STARTED "started"
#define STATUS_ACTIVE "active"

#define ROTATION_CLOCKWISE_DB "orario"
#define ROTATION_COUNTERCLOCKWISE_DB "antiorario"
#define ROTATION_CLOCKWISE_JSON "clockwise"
#define ROTATION_COUNTERCLOCKWISE_JSON "counterclockwise"

static char* createJsonError(const char* message) {
  if (!message) {
    message = "Errore sconosciuto";
  }

  size_t bufferSize = strlen(message) + 128;
  char* json = malloc(bufferSize);
  if (!json) {
    return strdup("{\"result\":false,\"message\":\"Errore nell'allocazione di memoria\",\"content\":null}");
  }

  snprintf(json, bufferSize, "{\"result\":false,\"message\":\"%.100s\",\"content\":null}", message);
  return json;
}

static char* createJsonSuccess(const char* message, const char* content) {
  size_t bufferSize = JSON_BUFFER_MEDIUM;
  if (content) {
    bufferSize += strlen(content);
  }

  char* json = malloc(bufferSize);
  if (!json) {
    return createJsonError("Errore nell'allocazione di memoria");
  }

  if (content) {
    snprintf(json, bufferSize, "{\"result\":true,\"message\":\"%.100s\",\"content\":%s}", message, content);
  } else {
    snprintf(json, bufferSize, "{\"result\":true,\"message\":\"%.100s\",\"content\":null}", message);
  }

  return json;
}

static bool isValidLobbyId(const char* id) {
  if (!id) return false;

  size_t len = strlen(id);
  if (len != LOBBY_ID_LENGTH) return false;

  for (size_t i = 0; i < len; i++) {
    if (!isalnum((unsigned char)id[i])) {
      return false;
    }
  }

  return true;
}

static bool isValidPlayerId(int playerId) {
  return playerId > 0;
}

static const char* rotationDbToJson(const char* dbRotation) {
  if (!dbRotation) {
    return ROTATION_CLOCKWISE_JSON;
  }

  if (strcasecmp(dbRotation, ROTATION_COUNTERCLOCKWISE_DB) == 0 ||
      strcasecmp(dbRotation, ROTATION_COUNTERCLOCKWISE_JSON) == 0) {
    return ROTATION_COUNTERCLOCKWISE_JSON;
  }

  return ROTATION_CLOCKWISE_JSON;
}

static const char* rotationEnumToDb(LobbyRotation rotation) {
  return (rotation == CLOCKWISE) ? ROTATION_CLOCKWISE_DB : ROTATION_COUNTERCLOCKWISE_DB;
}

static void generateLobbyId(char* buffer) {
  static const char charset[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
  static const int charsetSize = sizeof(charset) - 1;

  static bool seeded = false;
  if (!seeded) {
    srand((unsigned int)time(NULL) ^ (unsigned int)getpid());
    seeded = true;
  }

  for (int i = 0; i < LOBBY_ID_LENGTH; i++) {
    buffer[i] = charset[rand() % charsetSize];
  }
  buffer[LOBBY_ID_LENGTH] = '\0';
}

static DBConnection* connectToDatabase(void) {
  DBConnection* conn = dbConnectWithOptions(DEFAULT_DB_HOST, DEFAULT_DB_NAME, DEFAULT_DB_USER, DEFAULT_DB_PASS, DB_CONNECTION_TIMEOUT);
  if (!conn || !dbIsConnected(conn)) {
    if (conn) {
      dbDisconnect(conn);
    }
    return NULL;
  }

  return conn;
}

static bool lobbyIdExists(DBConnection* conn, const char* id) {
  const char* sql = "SELECT 1 FROM lobby WHERE id = $1 LIMIT 1";
  const char* paramValues[1] = { id };

  PGresult* res = dbExecutePrepared(conn, sql, 1, paramValues);
  if (!res) {
    return false;
  }

  int nrows = PQntuples(res);
  PQclear(res);

  return nrows > 0;
}

static const char* getSafeValue(PGresult* res, int row, int col, const char* defaultValue) {
  if (PQgetisnull(res, row, col)) {
    return defaultValue;
  }
  return PQgetvalue(res, row, col);
}

static bool stringToBool(const char* str) {
  if (!str) return false;
  return (str[0] == 't' || str[0] == 'T' || strcmp(str, "true") == 0);
}

char* getAllLobbies() {
  DBConnection* conn = connectToDatabase();
  if (!conn) {
    return createJsonError("Connessione al database fallita");
  }

  const char* sql = "SELECT id, utenti_connessi, rotazione, id_accountcreatore, "
                    "is_private, status FROM lobby "
                    "WHERE is_private = false OR is_private IS NULL";

  PGresult* res = dbExecuteQueryWithTimeout(conn, sql, DB_QUERY_TIMEOUT);
  if (!res) {
    dbDisconnect(conn);
    return createJsonError("Errore nell'esecuzione della query");
  }

  int nrows = PQntuples(res);

  /* Get column indices */
  int colId = PQfnumber(res, "id");
  int colUsers = PQfnumber(res, "utenti_connessi");
  int colRotation = PQfnumber(res, "rotazione");
  int colCreator = PQfnumber(res, "id_accountcreatore");
  int colPrivate = PQfnumber(res, "is_private");
  int colStatus = PQfnumber(res, "status");

  /* Allocate buffer for JSON response */
  char* jsonResponse = malloc(JSON_BUFFER_LARGE);
  if (!jsonResponse) {
    PQclear(res);
    dbDisconnect(conn);
    return createJsonError("Errore nell'allocazione di memoria");
  }

  strcpy(jsonResponse, "{\"result\":true,\"message\":\"Lobby pubbliche trovate\",\"content\":[");

  char tempBuffer[JSON_BUFFER_SMALL];
  bool firstLobby = true;

  for (int r = 0; r < nrows; r++) {
    if (!firstLobby) {
      strcat(jsonResponse, ",");
    }
    firstLobby = false;

    const char* id = getSafeValue(res, r, colId, "");
    const char* users = getSafeValue(res, r, colUsers, "0");
    const char* rotation = getSafeValue(res, r, colRotation, ROTATION_CLOCKWISE_DB);
    const char* creator = getSafeValue(res, r, colCreator, "");
    const char* isPrivate = getSafeValue(res, r, colPrivate, "f");
    const char* status = getSafeValue(res, r, colStatus, STATUS_WAITING);

    const char* rotationJson = rotationDbToJson(rotation);
    bool isPrivateBool = stringToBool(isPrivate);

    snprintf(tempBuffer, sizeof(tempBuffer),
              "{\"id\":\"%s\",\"utentiConnessi\":%s,\"rotation\":\"%s\","
              "\"creator\":\"%s\",\"isPrivate\":%s,\"status\":\"%s\"}",
              id, users, rotationJson, creator,
              isPrivateBool ? "true" : "false", status);

    /* Check buffer capacity */
    if (strlen(jsonResponse) + strlen(tempBuffer) + 10 >= JSON_BUFFER_LARGE) {
      break;
    }

    strcat(jsonResponse, tempBuffer);
  }

  strcat(jsonResponse, "]}");

  PQclear(res);
  dbDisconnect(conn);

  return jsonResponse;
}

char* getLobbyById(const char* id) {
  if (!isValidLobbyId(id)) {
    return createJsonError("Invalid lobby ID");
  }

  DBConnection* conn = connectToDatabase();
  if (!conn) {
    return createJsonError("Connessione al database fallita");
  }

  const char* sql = "SELECT id, utenti_connessi, rotazione, id_accountcreatore, is_private, status FROM lobby WHERE id = $1";
  const char* paramValues[1] = { id };

  PGresult* res = dbExecutePrepared(conn, sql, 1, paramValues);
  if (!res) {
    dbDisconnect(conn);
    return createJsonError("Errore nella query del database");
  }

  if (PQntuples(res) == 0) {
    PQclear(res);
    dbDisconnect(conn);
    return createJsonError("Lobby non trovata");
  }

  /* Get column indices */
  int colId = PQfnumber(res, "id");
  int colUsers = PQfnumber(res, "utenti_connessi");
  int colRotation = PQfnumber(res, "rotazione");
  int colCreator = PQfnumber(res, "id_accountcreatore");
  int colPrivate = PQfnumber(res, "is_private");
  int colStatus = PQfnumber(res, "status");

  const char* lobbyId = getSafeValue(res, 0, colId, "");
  const char* users = getSafeValue(res, 0, colUsers, "0");
  const char* rotation = getSafeValue(res, 0, colRotation, ROTATION_COUNTERCLOCKWISE_JSON);
  const char* creator = getSafeValue(res, 0, colCreator, "");
  const char* isPrivate = getSafeValue(res, 0, colPrivate, "f");
  const char* status = getSafeValue(res, 0, colStatus, STATUS_WAITING);

  char* jsonResponse = malloc(JSON_BUFFER_MEDIUM);
  if (!jsonResponse) {
    PQclear(res);
    dbDisconnect(conn);
    return createJsonError("Errore nell'allocazione di memoria");
  }

  const char* rotationJson = rotationDbToJson(rotation);
  bool isPrivateBool = stringToBool(isPrivate);

  snprintf(jsonResponse, JSON_BUFFER_MEDIUM,
            "{\"result\":true,\"message\":\"Lobby trovata\",\"content\":{"
            "\"id\":\"%s\",\"utentiConnessi\":%s,\"rotation\":\"%s\","
            "\"creator\":\"%s\",\"isPrivate\":%s,\"status\":\"%s\"}}",
            lobbyId, users, rotationJson, creator,
            isPrivateBool ? "true" : "false", status);

  PQclear(res);
  dbDisconnect(conn);

  return jsonResponse;
}

char* getPlayerInfoById(int playerId) {
  if (!isValidPlayerId(playerId)) {
    return createJsonError("ID giocatore non valido");
  }

  DBConnection* conn = connectToDatabase();
  if (!conn) {
    return createJsonError("Connessione al database fallita");
  }

  const char* sql = "SELECT id, nickname, email, lingua FROM account WHERE id = $1";

  char playerIdStr[16];
  snprintf(playerIdStr, sizeof(playerIdStr), "%d", playerId);
  const char* params[1] = { playerIdStr };

  PGresult* res = dbExecutePrepared(conn, sql, 1, params);
  if (!res) {
    dbDisconnect(conn);
    return createJsonError("Errore nella query del database");
  }

  if (PQntuples(res) == 0) {
    PQclear(res);
    dbDisconnect(conn);
    return createJsonError("Giocatore non trovato");
  }

  /* Get column indices */
  int colId = PQfnumber(res, "id");
  int colNickname = PQfnumber(res, "nickname");
  int colEmail = PQfnumber(res, "email");
  int colLingua = PQfnumber(res, "lingua");

  const char* id = getSafeValue(res, 0, colId, "");
  const char* nickname = getSafeValue(res, 0, colNickname, "");
  const char* email = getSafeValue(res, 0, colEmail, "");
  const char* lingua = getSafeValue(res, 0, colLingua, "it");

  char* jsonResponse = malloc(JSON_BUFFER_MEDIUM);
  if (!jsonResponse) {
    PQclear(res);
    dbDisconnect(conn);
    return createJsonError("Errore nell'allocazione di memoria");
  }

  snprintf(jsonResponse, JSON_BUFFER_MEDIUM,
            "{\"result\":true,\"message\":\"Giocatore trovato\",\"content\":{"
            "\"id\":%s,\"nickname\":\"%s\",\"email\":\"%s\",\"lingua\":\"%s\"}}",
            id, nickname, email, lingua);

  PQclear(res);
  dbDisconnect(conn);

  return jsonResponse;
}

char* getLobbyPlayers(const char* lobbyId) {
  if (!isValidLobbyId(lobbyId)) {
    return createJsonError("Invalid lobby ID");
  }

  DBConnection* conn = connectToDatabase();
  if (!conn) {
    return createJsonError("Connessione al database fallita");
  }

  const char* sql = "SELECT lp.player_id, lp.status, lp.position, a.nickname "
                    "FROM lobby_players lp "
                    "JOIN account a ON lp.player_id = a.id "
                    "WHERE lp.lobby_id = $1 AND lp.status IN ('active', 'waiting') "
                    "ORDER BY "
                    "  CASE WHEN lp.status = 'active' THEN 0 ELSE 1 END, "
                    "  lp.joined_at";

  const char* params[1] = { lobbyId };
  PGresult* res = dbExecutePrepared(conn, sql, 1, params);

  if (!res) {
    dbDisconnect(conn);
    return createJsonError("Errore nella query del database");
  }

  int nrows = PQntuples(res);

  char* jsonResponse = malloc(JSON_BUFFER_LARGE / 2);
  if (!jsonResponse) {
    PQclear(res);
    dbDisconnect(conn);
    return createJsonError("Errore nell'allocazione di memoria");
  }

  strcpy(jsonResponse, "{\"result\":true,\"message\":\"Giocatori trovati\",\"content\":{");
  strcat(jsonResponse, "\"activePlayers\":[");

  char tempBuffer[256];
  bool firstActive = true;
  int activeCount = 0;

  /* First pass: active players */
  for (int i = 0; i < nrows; i++) {
    const char* status = PQgetvalue(res, i, 1);
    if (strcmp(status, STATUS_ACTIVE) == 0) {
      if (!firstActive) strcat(jsonResponse, ",");
      firstActive = false;
      activeCount++;

      const char* playerId = PQgetvalue(res, i, 0);
      const char* nickname = PQgetvalue(res, i, 3);

      snprintf(tempBuffer, sizeof(tempBuffer), "{\"id\":%s,\"nickname\":\"%s\",\"status\":\"active\"}", playerId, nickname);
      strcat(jsonResponse, tempBuffer);
    }
  }

  strcat(jsonResponse, "],\"waitingPlayers\":[");

  bool firstWaiting = true;
  int waitingCount = 0;

  /* Second pass: waiting players */
  for (int i = 0; i < nrows; i++) {
    const char* status = PQgetvalue(res, i, 1);
    if (strcmp(status, STATUS_WAITING) == 0) {
      if (!firstWaiting) strcat(jsonResponse, ",");
      firstWaiting = false;
      waitingCount++;

      const char* playerId = PQgetvalue(res, i, 0);
      const char* position = PQgetvalue(res, i, 2);
      const char* nickname = PQgetvalue(res, i, 3);

      snprintf(tempBuffer, sizeof(tempBuffer), "{\"id\":%s,\"nickname\":\"%s\",\"status\":\"waiting\",\"position\":%s}", playerId, nickname, position);
      strcat(jsonResponse, tempBuffer);
    }
  }

  snprintf(tempBuffer, sizeof(tempBuffer), "],\"activeCount\":%d,\"waitingCount\":%d}}", activeCount, waitingCount);
  strcat(jsonResponse, tempBuffer);

  PQclear(res);
  dbDisconnect(conn);

  return jsonResponse;
}

Lobby* createLobby(int idCreator, bool isPrivate, LobbyRotation rotation) {
  if (!isValidPlayerId(idCreator)) {
    fprintf(stderr, "ID creatore non valido: %d\n", idCreator);
    return NULL;
  }

  DBConnection* conn = connectToDatabase();
  if (!conn) {
    fprintf(stderr, "Connessione al database fallita\n");
    return NULL;
  }

  /* Verify user exists */
  const char* userCheckSql = "SELECT 1 FROM account WHERE id = $1 LIMIT 1";
  char idCreatorStr[16];
  snprintf(idCreatorStr, sizeof(idCreatorStr), "%d", idCreator);
  const char* userParams[1] = { idCreatorStr };

  PGresult* userRes = dbExecutePrepared(conn, userCheckSql, 1, userParams);
  if (!userRes || PQntuples(userRes) == 0) {
    if (userRes) PQclear(userRes);
    fprintf(stderr, "Utente con ID %d non trovato\n", idCreator);
    dbDisconnect(conn);
    return NULL;
  }
  PQclear(userRes);

  /* Generate unique lobby ID */
  char lobbyId[LOBBY_ID_LENGTH + 1];
  int attempts = 0;

  do {
    generateLobbyId(lobbyId);
    attempts++;

    if (attempts >= MAX_ID_GENERATION_ATTEMPTS) {
      fprintf(stderr, "Impossibile generare un ID unico dopo %d tentativi\n", MAX_ID_GENERATION_ATTEMPTS);
      dbDisconnect(conn);
      return NULL;
    }
  } while (lobbyIdExists(conn, lobbyId));

  /* Begin transaction */
  if (!dbBeginTransaction(conn)) {
    fprintf(stderr, "Impossibile avviare la transazione\n");
    dbDisconnect(conn);
    return NULL;
  }

  /* Insert into database */
  const char* sql = "INSERT INTO lobby (id, utenti_connessi, rotazione, id_accountcreatore, is_private, status) "
                    "VALUES ($1, $2, $3, $4, $5, $6)";

  const char* rotationStr = rotationEnumToDb(rotation);
  const char* isPrivateStr = isPrivate ? "t" : "f";
  const char* statusStr = STATUS_WAITING;
  const char* connectedUsers = "0";

  const char* paramValues[6] = {
    lobbyId,
    connectedUsers,
    rotationStr,
    idCreatorStr,
    isPrivateStr,
    statusStr
  };

  PGresult* res = dbExecutePrepared(conn, sql, 6, paramValues);
  if (!res) {
    fprintf(stderr, "Inserimento fallito: %s\n", dbGetErrorMessage(conn));
    dbRollbackTransaction(conn);
    dbDisconnect(conn);
    return NULL;
  }
  PQclear(res);

  /* Commit transaction */
  if (!dbCommitTransaction(conn)) {
    fprintf(stderr, "Impossibile completare la transazione\n");
    dbDisconnect(conn);
    return NULL;
  }

  dbDisconnect(conn);

  /* Create Lobby structure */
  Lobby* lobby = malloc(sizeof(Lobby));
  if (!lobby) {
    fprintf(stderr, "Errore nell'allocazione di memoria per Lobby\n");
    return NULL;
  }

  strncpy(lobby->id, lobbyId, sizeof(lobby->id) - 1);
  lobby->id[sizeof(lobby->id) - 1] = '\0';
  lobby->idCreator = idCreator;
  lobby->rotation = rotation;
  lobby->isPrivate = isPrivate;
  lobby->status = WAITING;

  return lobby;
}

static bool parseCreateLobbyRequest(const char* requestBody, int* outIdCreator, bool* outIsPrivate, LobbyRotation* outRotation) {
  if (!requestBody || !outIdCreator || !outIsPrivate || !outRotation) {
    return false;
  }

  /* Default values */
  *outIdCreator = 0;
  *outIsPrivate = false;
  *outRotation = CLOCKWISE;

  /* Create copy for parsing */
  char* copy = strdup(requestBody);
  if (!copy) {
    return false;
  }

  /* Remove whitespace */
  char* dst = copy;
  for (const char* src = copy; *src; src++) {
    if (!isspace((unsigned char)*src)) {
      *dst++ = *src;
    }
  }
  *dst = '\0';

  /* Parse idCreator */
  char* idStart = strstr(copy, "\"idCreator\":");
  if (idStart) {
    idStart += strlen("\"idCreator\":");
    *outIdCreator = atoi(idStart);
  }

  /* Parse isPrivate */
  char* privateStart = strstr(copy, "\"isPrivate\":");
  if (privateStart) {
    privateStart += strlen("\"isPrivate\":");
    *outIsPrivate = (strncmp(privateStart, "true", 4) == 0);
  }

  /* Parse rotation */
  char* rotationStart = strstr(copy, "\"rotation\":\"");
  if (rotationStart) {
    rotationStart += strlen("\"rotation\":\"");
    char* rotationEnd = strchr(rotationStart, '"');
    if (rotationEnd) {
      size_t len = rotationEnd - rotationStart;
      if (strncmp(rotationStart, ROTATION_COUNTERCLOCKWISE_JSON, len) == 0 ||
          strncmp(rotationStart, ROTATION_COUNTERCLOCKWISE_DB, len) == 0) {
        *outRotation = COUNTERCLOCKWISE;
      }
    }
  }

  free(copy);
  return (*outIdCreator > 0);
}

char* createLobbyEndpoint(const char* requestBody) {
  if (!requestBody) {
    return createJsonError("Richiesta invalida");
  }

  int idCreator;
  bool isPrivate;
  LobbyRotation rotation;

  if (!parseCreateLobbyRequest(requestBody, &idCreator, &isPrivate, &rotation)) {
    return createJsonError("Parametri della richiesta non validi");
  }

  if (!isValidPlayerId(idCreator)) {
    return createJsonError("L'ID del creatore deve essere un numero positivo");
  }

  Lobby* lobby = createLobby(idCreator, isPrivate, rotation);
  if (!lobby) {
    return createJsonError("Impossibile creare la lobby");
  }

  char* jsonResponse = malloc(JSON_BUFFER_MEDIUM);
  if (!jsonResponse) {
    free(lobby);
    return createJsonError("Errore nell'allocazione di memoria");
  }

  snprintf(jsonResponse, JSON_BUFFER_MEDIUM,
            "{\"result\":true,\"message\":\"Lobby creata con successo\",\"content\":{"
            "\"id\":\"%s\","
            "\"connectedUsers\":1,"
            "\"rotation\":\"%s\","
            "\"creator\":%d,"
            "\"isPrivate\":%s,"
            "\"status\":\"waiting\""
            "}}",
            lobby->id,
            lobby->rotation == CLOCKWISE ? ROTATION_CLOCKWISE_JSON : ROTATION_COUNTERCLOCKWISE_JSON,
            lobby->idCreator,
            lobby->isPrivate ? "true" : "false");

  free(lobby);
  return jsonResponse;
}

char* deleteLobby(const char* lobbyId, int creatorId) {
  if (!isValidLobbyId(lobbyId)) {
    return createJsonError("ID lobby non valido");
  }

  if (!isValidPlayerId(creatorId)) {
    return createJsonError("ID creatore non valido");
  }

  DBConnection* conn = connectToDatabase();
  if (!conn) {
    return createJsonError("Connessione al database fallita");
  }

  /* Verify lobby exists and check creator */
  const char* checkSql = "SELECT id_accountcreatore FROM lobby WHERE id = $1";
  const char* checkParams[1] = { lobbyId };

  PGresult* checkRes = dbExecutePrepared(conn, checkSql, 1, checkParams);
  if (!checkRes || PQntuples(checkRes) == 0) {
    if (checkRes) PQclear(checkRes);
    dbDisconnect(conn);
    return createJsonError("Lobby non trovata");
  }

  /* Check if user is creator */
  int dbCreatorId = atoi(PQgetvalue(checkRes, 0, 0));
  PQclear(checkRes);

  if (dbCreatorId != creatorId) {
    dbDisconnect(conn);
    return createJsonError("Solo il creatore può eliminare la lobby");
  }

  /* Delete lobby */
  const char* deleteSql = "DELETE FROM lobby WHERE id = $1 AND id_accountcreatore = $2";

  char creatorIdStr[16];
  snprintf(creatorIdStr, sizeof(creatorIdStr), "%d", creatorId);

  const char* deleteParams[2] = { lobbyId, creatorIdStr };

  PGresult* deleteRes = dbExecutePrepared(conn, deleteSql, 2, deleteParams);
  if (!deleteRes) {
    dbDisconnect(conn);
    return createJsonError("Impossibile eliminare la lobby");
  }

  int affectedRows = atoi(PQcmdTuples(deleteRes));
  PQclear(deleteRes);
  dbDisconnect(conn);

  if (affectedRows == 0) {
    return createJsonError("Impossibile eliminare la lobby");
  }

  return createJsonSuccess("Lobby eliminata con successo", NULL);
}

char* joinLobby(const char* lobbyId, int playerId) {
  if (!isValidLobbyId(lobbyId) || !isValidPlayerId(playerId)) {
    return createJsonError("Parametri non validi");
  }

  DBConnection* conn = connectToDatabase();
  if (!conn) {
    return createJsonError("Connessione al database fallita");
  }

  if (!dbBeginTransaction(conn)) {
    dbDisconnect(conn);
    return createJsonError("Errore nella transazione");
  }

  /* Verify lobby exists and is not started */
  const char* lobbyCheck = "SELECT utenti_connessi, status FROM lobby WHERE id = $1";
  const char* lobbyParams[1] = { lobbyId };

  PGresult* lobbyRes = dbExecutePrepared(conn, lobbyCheck, 1, lobbyParams);
  if (!lobbyRes || PQntuples(lobbyRes) == 0) {
    if (lobbyRes) PQclear(lobbyRes);
    dbRollbackTransaction(conn);
    dbDisconnect(conn);
    return createJsonError("Lobby non trovata");
  }

  const char* lobbyStatus = PQgetvalue(lobbyRes, 0, 1);

  /* Check if player is already in lobby */
  const char* playerCheck = "SELECT status FROM lobby_players WHERE lobby_id = $1 AND player_id = $2";

  char playerIdStr[16];
  snprintf(playerIdStr, sizeof(playerIdStr), "%d", playerId);
  const char* playerParams[2] = { lobbyId, playerIdStr };

  PGresult* playerRes = dbExecutePrepared(conn, playerCheck, 2, playerParams);
  if (!playerRes) {
    dbRollbackTransaction(conn);
    dbDisconnect(conn);
    return createJsonError("Errore nella verifica del giocatore");
  }

  if (PQntuples(playerRes) > 0) {
    const char* playerStatus = PQgetvalue(playerRes, 0, 0);
    PQclear(playerRes);

    if (strcmp(playerStatus, STATUS_ACTIVE) == 0 || strcmp(playerStatus, STATUS_WAITING) == 0) {
      dbRollbackTransaction(conn);
      dbDisconnect(conn);
      return createJsonError("Giocatore già presente nella lobby");
    }
  } else {
    PQclear(playerRes);
  }

  /* Count active players */
  const char* activeCountSql = "SELECT COUNT(*) FROM lobby_players WHERE lobby_id = $1 AND status = 'active'";

  PGresult* countRes = dbExecutePrepared(conn, activeCountSql, 1, lobbyParams);
  if (!countRes) {
    dbRollbackTransaction(conn);
    dbDisconnect(conn);
    return createJsonError("Errore nel conteggio dei giocatori");
  }

  int activePlayers = atoi(PQgetvalue(countRes, 0, 0));
  PQclear(countRes);

  const char* newStatus = NULL;

  /* Determine new player status */
  if (strcmp(lobbyStatus, STATUS_STARTED) == 0) {
    newStatus = STATUS_WAITING;
  } else if (activePlayers >= MAX_PLAYERS) {
    newStatus = STATUS_WAITING;
  } else {
    newStatus = STATUS_ACTIVE;
  }

  int position = 0;

  if (strcmp(newStatus, STATUS_WAITING) == 0) {
    /* Find next position in queue */
    const char* positionSql = "SELECT COALESCE(MAX(position), 0) + 1 FROM lobby_players "
                              "WHERE lobby_id = $1 AND status = 'waiting'";

    PGresult* posRes = dbExecutePrepared(conn, positionSql, 1, lobbyParams);
    if (posRes && PQntuples(posRes) > 0) {
      position = atoi(PQgetvalue(posRes, 0, 0));
    }
    if (posRes) PQclear(posRes);
  }

  /* Insert/update player */
  const char* insertSql = "INSERT INTO lobby_players (lobby_id, player_id, status, position) "
                          "VALUES ($1, $2, $3, $4) "
                          "ON CONFLICT (lobby_id, player_id) "
                          "DO UPDATE SET status = $3, position = $4, joined_at = CURRENT_TIMESTAMP";

  char positionStr[16];
  snprintf(positionStr, sizeof(positionStr), "%d", position);
  const char* insertParams[4] = { lobbyId, playerIdStr, newStatus, positionStr };

  PGresult* insertRes = dbExecutePrepared(conn, insertSql, 4, insertParams);
  if (!insertRes) {
    dbRollbackTransaction(conn);
    dbDisconnect(conn);
    return createJsonError("Impossibile aggiungere il giocatore");
  }
  PQclear(insertRes);

  if (!dbCommitTransaction(conn)) {
    dbDisconnect(conn);
    return createJsonError("Errore nel commit della transazione");
  }

  dbDisconnect(conn);

  /* Build response */
  char content[128];
  snprintf(content, sizeof(content), "{\"status\":\"%s\",\"position\":%d}", newStatus, position);

  return createJsonSuccess("Giocatore aggiunto con successo", content);
}

static bool promoteNextWaitingPlayer(DBConnection* conn, const char* lobbyId) {
  if (!conn || !lobbyId) {
    return false;
  }

  /* Find next player in queue */
  const char* nextSql = "SELECT player_id FROM lobby_players "
                        "WHERE lobby_id = $1 AND status = 'waiting' "
                        "ORDER BY position ASC LIMIT 1 FOR UPDATE";
  const char* nextParams[1] = { lobbyId };

  PGresult* nextRes = dbExecutePrepared(conn, nextSql, 1, nextParams);
  if (!nextRes || PQntuples(nextRes) == 0) {
    if (nextRes) PQclear(nextRes);
    return false;
  }

  const char* nextPlayerId = PQgetvalue(nextRes, 0, 0);

  /* Promote player */
  const char* promoteSql = "UPDATE lobby_players SET status = 'active', position = 0 "
                          "WHERE lobby_id = $1 AND player_id = $2";
  const char* promoteParams[2] = { lobbyId, nextPlayerId };

  PGresult* promoteRes = dbExecutePrepared(conn, promoteSql, 2, promoteParams);
  bool success = (promoteRes != NULL);

  if (promoteRes) PQclear(promoteRes);
  PQclear(nextRes);

  return success;
}

char* leaveLobby(const char* lobbyId, int playerId) {
  if (!isValidLobbyId(lobbyId) || !isValidPlayerId(playerId)) {
    return createJsonError("Parametri non validi");
  }

  DBConnection* conn = connectToDatabase();
  if (!conn) {
    return createJsonError("Connessione al database fallita");
  }

  if (!dbBeginTransaction(conn)) {
    dbDisconnect(conn);
    return createJsonError("Errore nella transazione");
  }

  /* Check if lobby exists and get creator */
  const char* creatorSql = "SELECT id_accountcreatore FROM lobby WHERE id = $1 FOR UPDATE";
  const char* creatorParams[1] = { lobbyId };

  PGresult* creatorRes = dbExecutePrepared(conn, creatorSql, 1, creatorParams);
  if (!creatorRes || PQntuples(creatorRes) == 0) {
    if (creatorRes) PQclear(creatorRes);
    dbRollbackTransaction(conn);
    dbDisconnect(conn);
    return createJsonError("Lobby non trovata");
  }

  char* endptr = NULL;
  long lobbyCreatorLong = strtol(PQgetvalue(creatorRes, 0, 0), &endptr, 10);

  if (endptr == PQgetvalue(creatorRes, 0, 0) || lobbyCreatorLong <= 0) {
    PQclear(creatorRes);
    dbRollbackTransaction(conn);
    dbDisconnect(conn);
    return createJsonError("Id creatore non valido");
  }

  int lobbyCreator = (int)lobbyCreatorLong;
  bool isCreator = (lobbyCreator == playerId);
  PQclear(creatorRes);

  char playerIdStr[16];
  snprintf(playerIdStr, sizeof(playerIdStr), "%d", playerId);

  if (isCreator) {
    /* Creator deletes entire lobby */
    const char* deleteLobby = "DELETE FROM lobby WHERE id = $1 AND id_accountcreatore = $2";
    const char* deleteParams[2] = { lobbyId, playerIdStr };

    PGresult* deleteRes = dbExecutePrepared(conn, deleteLobby, 2, deleteParams);
    if (!deleteRes) {
      dbRollbackTransaction(conn);
      dbDisconnect(conn);
      return createJsonError("Errore nella cancellazione della lobby");
    }

    long affected = strtol(PQcmdTuples(deleteRes), NULL, 10);
    PQclear(deleteRes);

    if (affected == 0) {
      dbRollbackTransaction(conn);
      dbDisconnect(conn);
      return createJsonError("Impossibile eliminare la lobby");
    }

    if (!dbCommitTransaction(conn)) {
      dbDisconnect(conn);
      return createJsonError("Errore nel commit");
    }

    dbDisconnect(conn);

    char content[64];
    snprintf(content, sizeof(content), "{\"lobby_deleted\":true}");
    return createJsonSuccess("Lobby eliminata - creatore uscito", content);
  }

  /* Regular player: verify presence */
  const char* checkSql = "SELECT status FROM lobby_players "
                        "WHERE lobby_id = $1 AND player_id = $2 FOR UPDATE";
  const char* checkParams[2] = { lobbyId, playerIdStr };

  PGresult* checkRes = dbExecutePrepared(conn, checkSql, 2, checkParams);
  if (!checkRes || PQntuples(checkRes) == 0) {
    if (checkRes) PQclear(checkRes);
    dbRollbackTransaction(conn);
    dbDisconnect(conn);
    return createJsonError("Giocatore non nella lobby");
  }

  const char* currentStatus = PQgetvalue(checkRes, 0, 0);
  bool wasActive = (strcmp(currentStatus, STATUS_ACTIVE) == 0);
  PQclear(checkRes);

  /* Remove player */
  const char* deleteSql = "DELETE FROM lobby_players WHERE lobby_id = $1 AND player_id = $2";
  PGresult* deleteRes = dbExecutePrepared(conn, deleteSql, 2, checkParams);
  if (!deleteRes) {
    dbRollbackTransaction(conn);
    dbDisconnect(conn);
    return createJsonError("Errore nella rimozione del giocatore");
  }
  PQclear(deleteRes);

  /* If active player left, promote next waiting player */
  if (wasActive) {
    promoteNextWaitingPlayer(conn, lobbyId);
  }

  if (!dbCommitTransaction(conn)) {
    dbDisconnect(conn);
    return createJsonError("Errore nel commit");
  }

  dbDisconnect(conn);
  return createJsonSuccess("Giocatore rimosso con successo", NULL);
}

char* startGame(const char* lobbyId, int creatorId) {
  if (!isValidLobbyId(lobbyId)) {
    return createJsonError("ID lobby non valido");
  }

  if (!isValidPlayerId(creatorId)) {
    return createJsonError("ID creatore non valido");
  }

  DBConnection* conn = connectToDatabase();
  if (!conn) {
    return createJsonError("Connessione al database fallita");
  }

  if (!dbBeginTransaction(conn)) {
    dbDisconnect(conn);
    return createJsonError("Errore nella transazione");
  }

  // Verifica che l'utente sia il creatore
  const char* checkSql = "SELECT id_accountcreatore, status FROM lobby WHERE id = $1 FOR UPDATE";
  const char* checkParams[1] = { lobbyId };

  PGresult* checkRes = dbExecutePrepared(conn, checkSql, 1, checkParams);
  if (!checkRes || PQntuples(checkRes) == 0) {
    if (checkRes) PQclear(checkRes);
    dbRollbackTransaction(conn);
    dbDisconnect(conn);
    return createJsonError("Lobby non trovata");
  }

  int dbCreatorId = atoi(PQgetvalue(checkRes, 0, 0));
  const char* currentStatus = PQgetvalue(checkRes, 0, 1);
  PQclear(checkRes);

  if (dbCreatorId != creatorId) {
    dbRollbackTransaction(conn);
    dbDisconnect(conn);
    return createJsonError("Solo il creatore può avviare la partita");
  }

  if (strcmp(currentStatus, STATUS_STARTED) == 0) {
    dbRollbackTransaction(conn);
    dbDisconnect(conn);
    return createJsonError("La partita è già iniziata");
  }

  // Conta i giocatori attivi
  const char* countSql = "SELECT COUNT(*) FROM lobby_players WHERE lobby_id = $1 AND status = 'active'";
  PGresult* countRes = dbExecutePrepared(conn, countSql, 1, checkParams);

  int activeCount = 0;
  if (countRes && PQntuples(countRes) > 0) {
    activeCount = atoi(PQgetvalue(countRes, 0, 0));
  }
  if (countRes) PQclear(countRes);

  if (activeCount < MIN_PLAYERS) {
    dbRollbackTransaction(conn);
    dbDisconnect(conn);

    char errorMsg[128];
    snprintf(errorMsg, sizeof(errorMsg), "Servono almeno %d giocatori per iniziare", MIN_PLAYERS);
    return createJsonError(errorMsg);
  }

  const char* updateSql = "UPDATE lobby SET status = $1 WHERE id = $2";
  const char* updateParams[2] = { STATUS_STARTED, lobbyId };

  PGresult* updateRes = dbExecutePrepared(conn, updateSql, 2, updateParams);
  if (!updateRes) {
    dbRollbackTransaction(conn);
    dbDisconnect(conn);
    return createJsonError("Errore nell'aggiornamento della lobby");
  }

  int affected = atoi(PQcmdTuples(updateRes));
  PQclear(updateRes);

  if (affected == 0) {
    dbRollbackTransaction(conn);
    dbDisconnect(conn);
    return createJsonError("Impossibile avviare la partita");
  }

  if (!dbCommitTransaction(conn)) {
    dbDisconnect(conn);
    return createJsonError("Errore nel commit della transazione");
  }

  dbDisconnect(conn);

  return createJsonSuccess("Partita avviata con successo", NULL);
}

char* endGame(const char* lobbyId, int creatorId) {
  if (!isValidLobbyId(lobbyId)) {
    return createJsonError("ID lobby non valido");
  }

  if (!isValidPlayerId(creatorId)) {
    return createJsonError("ID creatore non valido");
  }

  DBConnection* conn = connectToDatabase();
  if (!conn) {
    return createJsonError("Connessione al database fallita");
  }

  // Verifica che l'utente sia il creatore
  const char* checkSql = "SELECT id_accountcreatore, status FROM lobby WHERE id = $1";
  const char* checkParams[1] = { lobbyId };

  PGresult* checkRes = dbExecutePrepared(conn, checkSql, 1, checkParams);
  if (!checkRes || PQntuples(checkRes) == 0) {
    if (checkRes) PQclear(checkRes);
    dbDisconnect(conn);
    return createJsonError("Lobby non trovata");
  }

  int dbCreatorId = atoi(PQgetvalue(checkRes, 0, 0));
  const char* currentStatus = PQgetvalue(checkRes, 0, 1);
  PQclear(checkRes);

  if (dbCreatorId != creatorId) {
    dbDisconnect(conn);
    return createJsonError("Solo il creatore può terminare la partita");
  }

  if (strcmp(currentStatus, "finished") == 0) {
    dbDisconnect(conn);
    return createJsonError("La partita è già terminata");
  }

  // Aggiorna status a 'waiting'
  const char* updateSql = "UPDATE lobby SET status = $1 WHERE id = $2";
  const char* updateParams[2] = { "waiting", lobbyId };

  PGresult* updateRes = dbExecutePrepared(conn, updateSql, 2, updateParams);
  if (!updateRes) {
    dbDisconnect(conn);
    return createJsonError("Errore nell'aggiornamento della lobby");
  }

  int affected = atoi(PQcmdTuples(updateRes));
  PQclear(updateRes);
  dbDisconnect(conn);

  if (affected == 0) {
    return createJsonError("Impossibile terminare la partita");
  }

  return createJsonSuccess("Partita terminata con successo", NULL);
}