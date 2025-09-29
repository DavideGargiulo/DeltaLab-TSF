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
#include "dbConnection.h"
#include "lobby.h"

#define MIN_PLAYERS 4
#define MAX_PLAYERS 8

static void generateLobbyId(char *buffer) {
  const char charset[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
  const int charsetSize = sizeof(charset) - 1;
  srand((unsigned int)time(NULL) ^ getpid());

  for (int i = 0; i < 6; i++) {
    int key = rand() % charsetSize;
    buffer[i] = charset[key];
  }
  buffer[6] = '\0';
}

static bool lobbyIdExists(DBConnection *conn, const char *id) {
  const char *sql = "SELECT 1 FROM lobby WHERE id = $1 LIMIT 1";
  const char *paramValues[1] = { id };

  PGresult *res = dbExecutePrepared(conn, sql, 1, paramValues);
  if (!res) {
    return false;
  }

  int nrows = PQntuples(res);
  PQclear(res);

  return nrows > 0;
}

// Nuova funzione che restituisce JSON invece di inviarlo
char* getAllLobbies() {
  const char *host = getenv("DB_HOST");
  if (!host) host = "db";

  DBConnection *conn = dbConnectWithOptions("db", "deltalabtsf", "postgres", "admin", 30);
  if (!conn || !dbIsConnected(conn)) {
    if (conn) dbDisconnect(conn);
    return strdup("{\"result\":false,\"message\":\"Connessione al database fallita\",\"content\":null}");
  }

  // Modifica la query per escludere le lobby private
  const char *sql = "SELECT * FROM lobby WHERE is_private = false OR is_private IS NULL";
  PGresult *res = dbExecuteQueryWithTimeout(conn, sql, 10);
  if (!res) {
    dbDisconnect(conn);
    return strdup("{\"result\":false,\"message\":\"Errore nell'esecuzione della query\",\"content\":null}");
  }

  int nrows = PQntuples(res);

  // Risolvi indici colonne basati sulla struttura reale
  int colId       = PQfnumber(res, "id");
  int colUsers    = PQfnumber(res, "utenti_connessi");
  int colRotation = PQfnumber(res, "rotazione");
  int colCreator  = PQfnumber(res, "id_accountcreatore");
  int colPrivate  = PQfnumber(res, "is_private");
  int colStatus  = PQfnumber(res, "status");

  // Alloca buffer più grande per la risposta JSON
  char *jsonResponse = malloc(8192);
  if (!jsonResponse) {
    PQclear(res);
    dbDisconnect(conn);
    return strdup("{\"result\":false,\"message\":\"Errore memoria\",\"content\":null}");
  }

  // Inizia la risposta JSON
  strcpy(jsonResponse, "{\"result\":true,\"message\":\"Lobby pubbliche trovate\",\"content\":[");

  char tempBuffer[512];
  bool firstLobby = true;

  for (int r = 0; r < nrows; ++r) {
    if (!firstLobby) {
      strcat(jsonResponse, ",");
    }
    firstLobby = false;

    // Estrai i valori con controllo null
    const char *id = PQgetisnull(res, r, colId) ? "" : PQgetvalue(res, r, colId);
    const char *users = PQgetisnull(res, r, colUsers) ? "0" : PQgetvalue(res, r, colUsers);
    const char *rotation = PQgetisnull(res, r, colRotation) ? "orario" : PQgetvalue(res, r, colRotation);
    const char *creator = PQgetisnull(res, r, colCreator) ? "" : PQgetvalue(res, r, colCreator);
    const char *isPrivate = PQgetisnull(res, r, colPrivate) ? "f" : PQgetvalue(res, r, colPrivate);
    const char *status = PQgetisnull(res, r, colStatus) ? "waiting" : PQgetvalue(res, r, colStatus);

    // Converti rotation per la risposta JSON
    const char *rotation_json = "clockwise";
    if (strcasecmp(rotation, "antiorario") == 0 || strcasecmp(rotation, "counterclockwise") == 0) {
      rotation_json = "counterclockwise";
    }

    // Costruisci l'oggetto JSON per ogni lobby
    snprintf(tempBuffer, sizeof(tempBuffer),
              "{\"id\":\"%s\",\"utentiConnessi\":%s,\"rotation\":\"%s\","
              "\"creator\":\"%s\",\"isPrivate\":%s,\"status\":\"%s\"}",
              id, users, rotation_json, creator,
              (isPrivate[0] == 't') ? "true" : "false",
              status);

    // Controlla se abbiamo spazio sufficiente
    if (strlen(jsonResponse) + strlen(tempBuffer) + 10 >= 8192) {
      // Buffer troppo piccolo, interrompi (potresti anche riallocare)
      break;
    }

    strcat(jsonResponse, tempBuffer);
  }

  strcat(jsonResponse, "]}");

  PQclear(res);
  dbDisconnect(conn);

  return jsonResponse;
}

// Nuova funzione che restituisce JSON invece di inviarlo
char* getLobbyById(const char* id) {
  if (!id || strlen(id) == 0 || strlen(id) > 7) {
    return strdup("{\"result\":false,\"message\":\"ID lobby non valido\",\"content\":null}");
  }

  // Validazione caratteri
  for (const char *p = id; *p; p++) {
    if (!isalnum(*p)) {
      return strdup("{\"result\":false,\"message\":\"ID lobby contiene caratteri non validi\",\"content\":null}");
    }
  }

  const char *host = getenv("DB_HOST");
  if (!host) host = "db";

  DBConnection *conn = dbConnectWithOptions("db", "deltalabtsf", "postgres", "admin", 30);
  if (!conn || !dbIsConnected(conn)) {
    if (conn) dbDisconnect(conn);
    return strdup("{\"result\":false,\"message\":\"Errore connessione database\",\"content\":null}");
  }

  const char *sql = "SELECT * FROM lobby WHERE id = $1";
  const char *paramValues[1] = { id };

  PGresult *res = dbExecutePrepared(conn, sql, 1, paramValues);
  if (!res) {
    dbDisconnect(conn);
    return strdup("{\"result\":false,\"message\":\"Errore query database\",\"content\":null}");
  }

  int nrows = PQntuples(res);
  if (nrows == 0) {
    PQclear(res);
    dbDisconnect(conn);
    return strdup("{\"result\":false,\"message\":\"Lobby non trovata\",\"content\":null}");
  }

  // Estrai i dati dalla query (come nella funzione originale)
  int colId = PQfnumber(res, "id");
  int colUsers = PQfnumber(res, "utenti_connessi");
  int colRotation = PQfnumber(res, "rotazione");
  int colCreator = PQfnumber(res, "id_accountcreatore");
  int colPrivate = PQfnumber(res, "is_private");
  int colStatus= PQfnumber(res, "status");

  const char *lobby_id = PQgetisnull(res, 0, colId) ? "" : PQgetvalue(res, 0, colId);
  const char *users = PQgetisnull(res, 0, colUsers) ? "0" : PQgetvalue(res, 0, colUsers);
  const char *rotation = PQgetisnull(res, 0, colRotation) ? "clockwise" : PQgetvalue(res, 0, colRotation);
  const char *creator = PQgetisnull(res, 0, colCreator) ? "" : PQgetvalue(res, 0, colCreator);
  const char *isPrivate = PQgetisnull(res, 0, colPrivate) ? "false" : PQgetvalue(res, 0, colPrivate);
  const char *status = PQgetisnull(res, 0, colStatus) ? "waiting" : PQgetvalue(res, 0, colStatus);

  // Alloca buffer per la risposta JSON
  char *jsonResponse = malloc(1024);
  if (!jsonResponse) {
    PQclear(res);
    dbDisconnect(conn);
    return strdup("{\"result\":false,\"message\":\"Errore memoria\",\"content\":null}");
  }

  // Costruisci la risposta JSON
  snprintf(jsonResponse, 1024,
    "{\"result\":true,\"message\":\"Lobby trovata\",\"content\":{"
    "\"id\":\"%s\",\"utentiConnessi\":%s,\"rotation\":\"%s\","
    "\"creator\":\"%s\",\"isPrivate\":%s,\"status\":\"%s\"}}",
    lobby_id, users, rotation, creator,
    (strcmp(isPrivate, "t") == 0 || strcmp(isPrivate, "true") == 0) ? "true" : "false",
    status);

  PQclear(res);
  dbDisconnect(conn);

  return jsonResponse;
}

Lobby* createLobby(int idCreator, bool isPrivate, LobbyRotation rotation) {
  // Validazione input
  if (idCreator <= 0) {
    fprintf(stderr, "ID creator non valido: %d\n", idCreator);
    return NULL;
  }

  // Connessione al database
  const char *host = getenv("DB_HOST");
  if (!host) host = "db";

  DBConnection *conn = dbConnectWithOptions("db", "deltalabtsf", "postgres", "admin", 30);
  if (!conn || !dbIsConnected(conn)) {
    fprintf(stderr, "Impossibile connettersi al database\n");
    if (conn) dbDisconnect(conn);
    return NULL;
  }

  const char *user_check_sql = "SELECT 1 FROM account WHERE id = $1 LIMIT 1";
  char idCreatorStr[16];
  snprintf(idCreatorStr, sizeof(idCreatorStr), "%d", idCreator);
  const char *user_params[1] = { idCreatorStr };

  PGresult *user_res = dbExecutePrepared(conn, user_check_sql, 1, user_params);
  if (!user_res) {
    fprintf(stderr, "Errore nella verifica dell'utente\n");
    dbDisconnect(conn);
    return NULL;
  }

  int user_exists = PQntuples(user_res);
  PQclear(user_res);

  if (user_exists == 0) {
    fprintf(stderr, "Utente con ID %d non trovato\n", idCreator);
    dbDisconnect(conn);
    return NULL;
  }

  // Genera ID univoco per la lobby
  char lobbyId[7];
  int attempts = 0;
  const int MAX_ATTEMPTS = 10;

  do {
    generateLobbyId(lobbyId);
    attempts++;

    if (attempts >= MAX_ATTEMPTS) {
      fprintf(stderr, "Impossibile generare ID univoco dopo %d tentativi\n", MAX_ATTEMPTS);
      dbDisconnect(conn);
      return NULL;
    }
  } while (lobbyIdExists(conn, lobbyId));

  // Inizia transazione
  if (!dbBeginTransaction(conn)) {
    fprintf(stderr, "Impossibile iniziare transazione\n");
    dbDisconnect(conn);
    return NULL;
  }

  // Inserisci nel database
  const char *sql =
      "INSERT INTO lobby (id, utenti_connessi, rotazione, id_accountcreatore, is_private, status) "
      "VALUES ($1, $2, $3, $4, $5, $6)";

  const char *rotationStr = (rotation == CLOCKWISE) ? "orario" : "antiorario";
  const char *isPrivateStr = isPrivate ? "t" : "f";  // Postgres boolean
  const char *statusStr = "waiting";
  const char *connectedUsers = "1"; // creatore è il primo connesso

  // Converti idCreator da int a stringa
  snprintf(idCreatorStr, sizeof(idCreatorStr), "%d", idCreator);

  const char *paramValues[6] = {
    lobbyId,
    connectedUsers,
    rotationStr,
    idCreatorStr,
    isPrivateStr,
    statusStr
  };

  PGresult *res = dbExecutePrepared(conn, sql, 6, paramValues);

  if (!res) {
    fprintf(stderr, "Errore nell'esecuzione di dbExecutePrepared: %s\n", dbGetErrorMessage(conn));
    dbRollbackTransaction(conn);
    dbDisconnect(conn);
    return NULL;
  }

  PQclear(res);

  // Commit transazione
  if (!dbCommitTransaction(conn)) {
    fprintf(stderr, "Errore nel commit della transazione\n");
    dbDisconnect(conn);
    return NULL;
  }

  dbDisconnect(conn);

  // Crea struttura Lobby in memoria
  Lobby *lobby = malloc(sizeof(Lobby));
  if (!lobby) {
    fprintf(stderr, "Errore allocazione memoria per Lobby\n");
    return NULL;
  }

  // Inizializza i campi
  strncpy(lobby->id, lobbyId, sizeof(lobby->id) - 1);
  lobby->id[sizeof(lobby->id) - 1] = '\0';

  lobby->idCreator = idCreator;  // Assegnazione diretta di int, NON strncpy

  lobby->rotation = rotation;
  lobby->isPrivate = isPrivate;
  lobby->status = WAITING;

  return lobby;
}

char* createLobbyEndpoint(const char* requestBody) {
  if (!requestBody) {
    return strdup("{\"result\":false,\"message\":\"Request body mancante\",\"content\":null}");
  }

  int idCreator = 0;
  bool isPrivate = false;
  LobbyRotation rotation = CLOCKWISE;

  // Crea una copia del request body per il parsing
  char *copy = strdup(requestBody);
  if (!copy) {
    return strdup("{\"result\":false,\"message\":\"Errore di memoria\",\"content\":null}");
  }

  // Rimuovi spazi e newlines per semplificare il parsing
  char *p = copy;
  while (*p) {
    if (*p == ' ' || *p == '\n' || *p == '\t' || *p == '\r') {
      memmove(p, p + 1, strlen(p));
    } else {
      p++;
    }
  }

  // Cerca idCreator come numero (senza virgolette)
  char *idStart = strstr(copy, "\"idCreator\":");
  if (idStart) {
    idStart += strlen("\"idCreator\":");
    idCreator = atoi(idStart);  // Converte direttamente a intero
  }

  // Cerca isPrivate
  char *privateStart = strstr(copy, "\"isPrivate\":");
  if (privateStart) {
    privateStart += strlen("\"isPrivate\":");
    if (strncmp(privateStart, "true", 4) == 0) {
      isPrivate = true;
    } else if (strncmp(privateStart, "false", 5) == 0) {
      isPrivate = false;
    }
  }

  // Cerca rotation
  char *rotationStart = strstr(copy, "\"rotation\":\"");
  if (rotationStart) {
    rotationStart += strlen("\"rotation\":\"");
    char *rotationEnd = strchr(rotationStart, '"');
    if (rotationEnd) {
      size_t len = rotationEnd - rotationStart;
      if (strncmp(rotationStart, "counterclockwise", len) == 0 ||
        strncmp(rotationStart, "antiorario", len) == 0) {
        rotation = COUNTERCLOCKWISE;
      } else {
        rotation = CLOCKWISE;
      }
    }
  }

  free(copy);

  // Validazione
  if (idCreator <= 0) {
    return strdup("{\"result\":false,\"message\":\"ID creator deve essere un numero positivo\",\"content\":null}");
  }

  // Crea la lobby
  Lobby *lobby = createLobby(idCreator, isPrivate, rotation);
  if (!lobby) {
    return strdup("{\"result\":false,\"message\":\"Impossibile creare la lobby\",\"content\":null}");
  }

  // Costruisci risposta JSON di successo
  char *jsonResponse = malloc(1024);
  if (!jsonResponse) {
    return strdup("{\"result\":false,\"message\":\"Errore di memoria\",\"content\":null}");
  }

  snprintf(jsonResponse, 1024,
            "{\"result\":true,\"message\":\"Lobby creata con successo\",\"content\":{"
              "\"id\":\"%s\","
              "\"connectedUsers\":1,"
              "\"rotation\":\"%s\","
              "\"creator\":%d,"
              "\"isPrivate\":%s,"
              "\"status\":\"waiting\""
            "}}",
            lobby->id,
            lobby->rotation == CLOCKWISE ? "clockwise" : "counterclockwise",
            lobby->idCreator,
            lobby->isPrivate ? "true" : "false");

  return jsonResponse;
}

char* deleteLobby(const char* lobbyId, int creatorId) {
  if (!lobbyId || strlen(lobbyId) != 6) {
    return strdup("{\"result\":false,\"message\":\"ID lobby non valido\",\"content\":null}");
  }

  for (int i = 0; i < 6; i++) {
    if (!isalnum(lobbyId[i])) {
      return strdup("{\"result\":false,\"message\":\"ID contiene caratteri non validi\",\"content\":null}");
    }
  }

  if (creatorId <= 0) {
    return strdup("{\"result\":false,\"message\":\"ID creator non valido\",\"content\":null}");
  }

  // Connessione al database
  const char *host = getenv("DB_HOST");
  if (!host) host = "db";

  DBConnection *conn = dbConnectWithOptions("db", "deltalabtsf", "postgres", "admin", 30);
  if (!conn || !dbIsConnected(conn)) {
    if (conn) dbDisconnect(conn);
    return strdup("{\"result\":false,\"message\":\"Connessione al database fallita\",\"content\":null}");
  }

  // Verifica che la lobby esista e che l'utente sia il creatore
  const char *check_sql = "SELECT id_accountcreatore FROM lobby WHERE id = $1";
  const char *check_params[1] = { lobbyId };

  PGresult *check_res = dbExecutePrepared(conn, check_sql, 1, check_params);
  if (!check_res) {
    dbDisconnect(conn);
    return strdup("{\"result\":false,\"message\":\"Errore query verifica\",\"content\":null}");
  }

  int nrows = PQntuples(check_res);
  if (nrows == 0) {
    PQclear(check_res);
    dbDisconnect(conn);
    return strdup("{\"result\":false,\"message\":\"Lobby non trovata\",\"content\":null}");
  }

  // Controlla se l'utente è il creatore
  const char *db_creator = PQgetvalue(check_res, 0, 0);
  int db_creator_id = atoi(db_creator);
  PQclear(check_res);

  if (db_creator_id != creatorId) {
    dbDisconnect(conn);
    return strdup("{\"result\":false,\"message\":\"Solo il creatore può eliminare la lobby\",\"content\":null}");
  }

  // Elimina la lobby
  const char *delete_sql = "DELETE FROM lobby WHERE id = $1 AND id_accountcreatore = $2";

  char creatorIdStr[16];
  snprintf(creatorIdStr, sizeof(creatorIdStr), "%d", creatorId);

  const char *delete_params[2] = { lobbyId, creatorIdStr };

  PGresult *delete_res = dbExecutePrepared(conn, delete_sql, 2, delete_params);
  if (!delete_res) {
    dbDisconnect(conn);
    return strdup("{\"result\":false,\"message\":\"Errore eliminazione lobby\",\"content\":null}");
  }

  int affected_rows = atoi(PQcmdTuples(delete_res));
  PQclear(delete_res);
  dbDisconnect(conn);

  if (affected_rows == 0) {
    return strdup("{\"result\":false,\"message\":\"Impossibile eliminare la lobby\",\"content\":null}");
  }

  return strdup("{\"result\":true,\"message\":\"Lobby eliminata con successo\",\"content\":null}");
}

// Funzione per ottenere informazioni di un giocatore dato il suo ID
char* getPlayerInfoById(int playerId) {
  if (playerId <= 0) {
    return strdup("{\"result\":false,\"message\":\"ID giocatore non valido\",\"content\":null}");
  }

  // Connessione al database
  const char *host = getenv("DB_HOST");
  if (!host) host = "db";

  DBConnection *conn = dbConnectWithOptions("db", "deltalabtsf", "postgres", "admin", 30);
  if (!conn || !dbIsConnected(conn)) {
    if (conn) dbDisconnect(conn);
    return strdup("{\"result\":false,\"message\":\"Connessione al database fallita\",\"content\":null}");
  }

  // Query per ottenere informazioni del giocatore
  const char *sql = "SELECT id, nickname, email, lingua FROM account WHERE id = $1";

  char playerIdStr[16];
  snprintf(playerIdStr, sizeof(playerIdStr), "%d", playerId);
  const char *params[1] = { playerIdStr };

  PGresult *res = dbExecutePrepared(conn, sql, 1, params);
  if (!res) {
    dbDisconnect(conn);
    return strdup("{\"result\":false,\"message\":\"Errore query database\",\"content\":null}");
  }

  int nrows = PQntuples(res);
  if (nrows == 0) {
    PQclear(res);
    dbDisconnect(conn);
    return strdup("{\"result\":false,\"message\":\"Giocatore non trovato\",\"content\":null}");
  }

  // Estrai i dati dalla query
  int colId = PQfnumber(res, "id");
  int col_nickname = PQfnumber(res, "nickname");
  int col_email = PQfnumber(res, "email");
  int col_lingua = PQfnumber(res, "lingua");

  const char *id = PQgetisnull(res, 0, colId) ? "" : PQgetvalue(res, 0, colId);
  const char *nickname = PQgetisnull(res, 0, col_nickname) ? "" : PQgetvalue(res, 0, col_nickname);
  const char *email = PQgetisnull(res, 0, col_email) ? "" : PQgetvalue(res, 0, col_email);
  const char *lingua = PQgetisnull(res, 0, col_lingua) ? "it" : PQgetvalue(res, 0, col_lingua);

  // Alloca buffer per la risposta JSON
  char *jsonResponse = malloc(1024);
  if (!jsonResponse) {
    PQclear(res);
    dbDisconnect(conn);
    return strdup("{\"result\":false,\"message\":\"Errore memoria\",\"content\":null}");
  }

  // Costruisci la risposta JSON
  snprintf(jsonResponse, 1024,
    "{\"result\":true,\"message\":\"Giocatore trovato\",\"content\":{"
    "\"id\":%s,\"nickname\":\"%s\",\"email\":\"%s\",\"lingua\":\"%s\"}}",
    id, nickname, email, lingua);

  PQclear(res);
  dbDisconnect(conn);

  return jsonResponse;
}

char* joinLobby(const char* lobbyId, int playerId) {
  if (!lobbyId || strlen(lobbyId) != 6 || playerId <= 0) {
    return strdup("{\"result\":false,\"message\":\"Parametri non validi\",\"content\":null}");
  }

  const char *host = getenv("DB_HOST");
  if (!host) host = "db";

  DBConnection *conn = dbConnectWithOptions("db", "deltalabtsf", "postgres", "admin", 30);
  if (!conn || !dbIsConnected(conn)) {
    if (conn) dbDisconnect(conn);
    return strdup("{\"result\":false,\"message\":\"Connessione al database fallita\",\"content\":null}");
  }

  if (!dbBeginTransaction(conn)) {
    dbDisconnect(conn);
    return strdup("{\"result\":false,\"message\":\"Errore transazione\",\"content\":null}");
  }

  // Verifica che la lobby esista e non sia piena
  const char *lobby_check = "SELECT utenti_connessi, status FROM lobby WHERE id = $1";
  const char *lobby_params[1] = { lobbyId };

  PGresult *lobby_res = dbExecutePrepared(conn, lobby_check, 1, lobby_params);
  if (!lobby_res) {
    dbRollbackTransaction(conn);
    dbDisconnect(conn);
    return strdup("{\"result\":false,\"message\":\"Errore verifica lobby\",\"content\":null}");
  }

  int lobby_rows = PQntuples(lobby_res);
  if (lobby_rows == 0) {
    PQclear(lobby_res);
    dbRollbackTransaction(conn);
    dbDisconnect(conn);
    return strdup("{\"result\":false,\"message\":\"Lobby non trovata\",\"content\":null}");
  }

  // const char *current_users_str = PQgetvalue(lobby_res, 0, 0);
  const char *lobby_status = PQgetvalue(lobby_res, 0, 1);
  // int current_users = atoi(current_users_str);
  PQclear(lobby_res);

  if (strcmp(lobby_status, "started") == 0) {
    dbRollbackTransaction(conn);
    dbDisconnect(conn);
    return strdup("{\"result\":false,\"message\":\"Lobby giÃ  iniziata\",\"content\":null}");
  }

  // Verifica se il giocatore Ã¨ giÃ  nella lobby
  const char *player_check = "SELECT status FROM lobby_players WHERE lobby_id = $1 AND player_id = $2";

  char playerIdStr[16];
  snprintf(playerIdStr, sizeof(playerIdStr), "%d", playerId);
  const char *player_params[2] = { lobbyId, playerIdStr };

  PGresult *player_res = dbExecutePrepared(conn, player_check, 2, player_params);
  if (!player_res) {
    dbRollbackTransaction(conn);
    dbDisconnect(conn);
    return strdup("{\"result\":false,\"message\":\"Errore verifica giocatore\",\"content\":null}");
  }

  int player_rows = PQntuples(player_res);
  if (player_rows > 0) {
    const char *player_status = PQgetvalue(player_res, 0, 0);
    PQclear(player_res);

    if (strcmp(player_status, "active") == 0 || strcmp(player_status, "waiting") == 0) {
      dbRollbackTransaction(conn);
      dbDisconnect(conn);
      return strdup("{\"result\":false,\"message\":\"Giocatore giÃ  nella lobby\",\"content\":null}");
    }
  } else {
    PQclear(player_res);
  }

  // Conta i giocatori attivi
  const char *active_count_sql = "SELECT COUNT(*) FROM lobby_players WHERE lobby_id = $1 AND status = 'active'";

  PGresult *count_res = dbExecutePrepared(conn, active_count_sql, 1, lobby_params);
  if (!count_res) {
    dbRollbackTransaction(conn);
    dbDisconnect(conn);
    return strdup("{\"result\":false,\"message\":\"Errore conteggio giocatori\",\"content\":null}");
  }

  int active_players = atoi(PQgetvalue(count_res, 0, 0));
  PQclear(count_res);

  // Determina lo status del nuovo giocatore
  const char *new_status = (active_players < MAX_PLAYERS) ? "active" : "waiting";
  int position = 0;

  if (strcmp(new_status, "waiting") == 0) {
    // Trova la prossima posizione in coda
    const char *position_sql = "SELECT COALESCE(MAX(position), 0) + 1 FROM lobby_players "
                               "WHERE lobby_id = $1 AND status = 'waiting'";

    PGresult *pos_res = dbExecutePrepared(conn, position_sql, 1, lobby_params);
    if (pos_res && PQntuples(pos_res) > 0) {
      position = atoi(PQgetvalue(pos_res, 0, 0));
    }
    if (pos_res) PQclear(pos_res);
  }

  // Inserisci/aggiorna il giocatore
  const char *insert_sql =
      "INSERT INTO lobby_players (lobby_id, player_id, status, position) "
      "VALUES ($1, $2, $3, $4) "
      "ON CONFLICT (lobby_id, player_id) "
      "DO UPDATE SET status = $3, position = $4, joined_at = CURRENT_TIMESTAMP";

  char positionStr[16];
  snprintf(positionStr, sizeof(positionStr), "%d", position);
  const char *insert_params[4] = { lobbyId, playerIdStr, new_status, positionStr };

  PGresult *insert_res = dbExecutePrepared(conn, insert_sql, 4, insert_params);
  if (!insert_res) {
    dbRollbackTransaction(conn);
    dbDisconnect(conn);
    return strdup("{\"result\":false,\"message\":\"Errore inserimento giocatore\",\"content\":null}");
  }
  PQclear(insert_res);

  // Aggiorna il contatore nella tabella lobby se il giocatore Ã¨ attivo
  if (strcmp(new_status, "active") == 0) {
    const char *update_lobby_sql = "UPDATE lobby SET utenti_connessi = utenti_connessi + 1 WHERE id = $1";

    PGresult *update_res = dbExecutePrepared(conn, update_lobby_sql, 1, lobby_params);
    if (update_res) PQclear(update_res);
  }

  if (!dbCommitTransaction(conn)) {
    dbDisconnect(conn);
    return strdup("{\"result\":false,\"message\":\"Errore commit transazione\",\"content\":null}");
  }

  dbDisconnect(conn);

  // Costruisci risposta JSON
  char *response = malloc(512);
  if (!response) {
    return strdup("{\"result\":false,\"message\":\"Errore memoria\",\"content\":null}");
  }

  snprintf(response, 512,
      "{\"result\":true,\"message\":\"Giocatore aggiunto con successo\",\"content\":{"
      "\"status\":\"%s\",\"position\":%d}}",
      new_status, position);

  return response;
}

char* getLobbyPlayers(const char* lobbyId) {
  if (!lobbyId || strlen(lobbyId) != 6) {
    return strdup("{\"result\":false,\"message\":\"ID lobby non valido\",\"content\":null}");
  }

  const char *host = getenv("DB_HOST");
  if (!host) host = "db";

  DBConnection *conn = dbConnectWithOptions("db", "deltalabtsf", "postgres", "admin", 30);
  if (!conn || !dbIsConnected(conn)) {
    if (conn) dbDisconnect(conn);
    return strdup("{\"result\":false,\"message\":\"Connessione al database fallita\",\"content\":null}");
  }

  const char *sql =
      "SELECT lp.player_id, lp.status, lp.position, a.nickname "
      "FROM lobby_players lp "
      "JOIN account a ON lp.player_id = a.id "
      "WHERE lp.lobby_id = $1 AND lp.status IN ('active', 'waiting') "
      "ORDER BY "
      "  CASE WHEN lp.status = 'active' THEN 0 ELSE 1 END, "
      "  lp.joined_at";

  const char *params[1] = { lobbyId };
  PGresult *res = dbExecutePrepared(conn, sql, 1, params);

  if (!res) {
    dbDisconnect(conn);
    return strdup("{\"result\":false,\"message\":\"Errore query database\",\"content\":null}");
  }

  int nrows = PQntuples(res);

  // Alloca buffer per la risposta JSON
  char *jsonResponse = malloc(4096);
  if (!jsonResponse) {
    PQclear(res);
    dbDisconnect(conn);
    return strdup("{\"result\":false,\"message\":\"Errore memoria\",\"content\":null}");
  }

  strcpy(jsonResponse, "{\"result\":true,\"message\":\"Giocatori trovati\",\"content\":{");
  strcat(jsonResponse, "\"activePlayers\":[");

  char tempBuffer[256];
  bool first_active = true;
  int active_count = 0;

  // Prima passa: giocatori attivi
  for (int i = 0; i < nrows; i++) {
    const char *status = PQgetvalue(res, i, 1);
    if (strcmp(status, "active") == 0) {
      if (!first_active) strcat(jsonResponse, ",");
      first_active = false;
      active_count++;

      const char *player_id = PQgetvalue(res, i, 0);
      const char *nickname = PQgetvalue(res, i, 3);

      snprintf(tempBuffer, sizeof(tempBuffer),
          "{\"id\":%s,\"nickname\":\"%s\",\"status\":\"active\"}",
          player_id, nickname);
      strcat(jsonResponse, tempBuffer);
    }
  }

  strcat(jsonResponse, "],\"waitingPlayers\":[");

  bool first_waiting = true;
  int waiting_count = 0;

  // Seconda passa: giocatori in attesa
  for (int i = 0; i < nrows; i++) {
    const char *status = PQgetvalue(res, i, 1);
    if (strcmp(status, "waiting") == 0) {
      if (!first_waiting) strcat(jsonResponse, ",");
      first_waiting = false;
      waiting_count++;

      const char *player_id = PQgetvalue(res, i, 0);
      const char *position = PQgetvalue(res, i, 2);
      const char *nickname = PQgetvalue(res, i, 3);

      snprintf(tempBuffer, sizeof(tempBuffer),
          "{\"id\":%s,\"nickname\":\"%s\",\"status\":\"waiting\",\"position\":%s}",
          player_id, nickname, position);
      strcat(jsonResponse, tempBuffer);
    }
  }

  // Chiudi la risposta JSON
  snprintf(tempBuffer, sizeof(tempBuffer), "],\"activeCount\":%d,\"waitingCount\":%d}}", active_count, waiting_count);
  strcat(jsonResponse, tempBuffer);

  PQclear(res);
  dbDisconnect(conn);

  return jsonResponse;
}

char* leaveLobby(const char* lobbyId, int playerId) {
  if (!lobbyId || strlen(lobbyId) != 6 || playerId <= 0) {
    return strdup("{\"result\":false,\"message\":\"Parametri non validi\",\"content\":null}");
  }

  const char *host = getenv("DB_HOST");
  if (!host) host = "db";

  DBConnection *conn = dbConnectWithOptions("db", "deltalabtsf", "postgres", "admin", 30);
  if (!conn || !dbIsConnected(conn)) {
    if (conn) dbDisconnect(conn);
    return strdup("{\"result\":false,\"message\":\"Connessione al database fallita\",\"content\":null}");
  }

  if (!dbBeginTransaction(conn)) {
    dbDisconnect(conn);
    return strdup("{\"result\":false,\"message\":\"Errore transazione\",\"content\":null}");
  }

  // Verifica se la lobby esiste e chi è il creatore
  const char *creator_sql = "SELECT id_accountcreatore FROM lobby WHERE id = $1 FOR UPDATE"; // lock row per sicurezza
  const char *creator_params[1] = { lobbyId };

  PGresult *creator_res = dbExecutePrepared(conn, creator_sql, 1, creator_params);
  if (!creator_res) {
    dbRollbackTransaction(conn);
    dbDisconnect(conn);
    return strdup("{\"result\":false,\"message\":\"Errore query creatore\",\"content\":null}");
  }

  if (PQntuples(creator_res) == 0) {
    PQclear(creator_res);
    dbRollbackTransaction(conn);
    dbDisconnect(conn);
    return strdup("{\"result\":false,\"message\":\"Lobby non trovata\",\"content\":null}");
  }

  char *endptr = NULL;
  long lobby_creator_long = strtol(PQgetvalue(creator_res, 0, 0), &endptr, 10);
  if (endptr == PQgetvalue(creator_res, 0, 0) || lobby_creator_long <= 0) {
    PQclear(creator_res);
    dbRollbackTransaction(conn);
    dbDisconnect(conn);
    return strdup("{\"result\":false,\"message\":\"Dati creatore lobby non validi\",\"content\":null}");
  }

  int lobby_creator = (int)lobby_creator_long;
  bool is_creator = (lobby_creator == playerId);
  PQclear(creator_res);

  // Unica stringa per playerId usata nelle prepared statements
  char playerIdStr[16];
  snprintf(playerIdStr, sizeof(playerIdStr), "%d", playerId);

  if (is_creator) {
    // Il creatore elimina la lobby (cascade su lobby_players)
    const char *delete_lobby_sql = "DELETE FROM lobby WHERE id = $1 AND id_accountcreatore = $2";
    const char *delete_params[2] = { lobbyId, playerIdStr };

    PGresult *delete_res = dbExecutePrepared(conn, delete_lobby_sql, 2, delete_params);
    if (!delete_res) {
      dbRollbackTransaction(conn);
      dbDisconnect(conn);
      return strdup("{\"result\":false,\"message\":\"Errore eliminazione lobby\",\"content\":null}");
    }

    long affected = strtol(PQcmdTuples(delete_res), NULL, 10);
    PQclear(delete_res);

    if (affected == 0) {
      dbRollbackTransaction(conn);
      dbDisconnect(conn);
      return strdup("{\"result\":false,\"message\":\"Impossibile eliminare la lobby\",\"content\":null}");
    }

    if (!dbCommitTransaction(conn)) {
      dbDisconnect(conn);
      return strdup("{\"result\":false,\"message\":\"Errore commit eliminazione\",\"content\":null}");
    }

    dbDisconnect(conn);

    return strdup("{\"result\":true,\"message\":\"Lobby eliminata - creatore uscito\",\"content\":{\"lobby_deleted\":true}}");
  }

  // Giocatore normale: verifica presenza e status
  const char *check_sql = "SELECT status FROM lobby_players WHERE lobby_id = $1 AND player_id = $2 FOR UPDATE";
  const char *check_params[2] = { lobbyId, playerIdStr };

  PGresult *check_res = dbExecutePrepared(conn, check_sql, 2, check_params);
  if (!check_res) {
    dbRollbackTransaction(conn);
    dbDisconnect(conn);
    return strdup("{\"result\":false,\"message\":\"Errore verifica giocatore\",\"content\":null}");
  }

  if (PQntuples(check_res) == 0) {
    PQclear(check_res);
    dbRollbackTransaction(conn);
    dbDisconnect(conn);
    return strdup("{\"result\":false,\"message\":\"Giocatore non trovato nella lobby\",\"content\":null}");
  }

  const char *current_status = PQgetvalue(check_res, 0, 0);
  bool was_active = (strcmp(current_status, "active") == 0);
  PQclear(check_res);

  // Rimuovi il giocatore
  const char *delete_sql = "DELETE FROM lobby_players WHERE lobby_id = $1 AND player_id = $2";
  PGresult *delete_res = dbExecutePrepared(conn, delete_sql, 2, check_params);
  if (!delete_res) {
    dbRollbackTransaction(conn);
    dbDisconnect(conn);
    return strdup("{\"result\":false,\"message\":\"Errore rimozione giocatore\",\"content\":null}");
  }
  PQclear(delete_res);

  if (was_active) {
    // Decrementa il contatore utenti_connessi
    const char *decr_sql = "UPDATE lobby SET utenti_connessi = utenti_connessi - 1 WHERE id = $1";
    const char *lobby_params[1] = { lobbyId };
    PGresult *decr_res = dbExecutePrepared(conn, decr_sql, 1, lobby_params);
    if (decr_res) PQclear(decr_res);

    // Trova il prossimo in coda (il più piccolo position)
    const char *next_sql = "SELECT player_id FROM lobby_players WHERE lobby_id = $1 AND status = 'waiting' ORDER BY position ASC LIMIT 1 FOR UPDATE";
    PGresult *next_res = dbExecutePrepared(conn, next_sql, 1, lobby_params);
    if (next_res && PQntuples(next_res) > 0) {
      const char *next_player_id = PQgetvalue(next_res, 0, 0);

      // Promuovi quel giocatore
      const char *promote_sql = "UPDATE lobby_players SET status = 'active', position = 0 WHERE lobby_id = $1 AND player_id = $2";
      const char *promote_params[2] = { lobbyId, next_player_id };
      PGresult *promote_res = dbExecutePrepared(conn, promote_sql, 2, promote_params);
      if (promote_res) {
        long promoted = strtol(PQcmdTuples(promote_res), NULL, 10);
        PQclear(promote_res);

        if (promoted > 0) {
          // Incrementa il contatore utenti_connessi
          const char *incr_sql = "UPDATE lobby SET utenti_connessi = utenti_connessi + 1 WHERE id = $1";
          PGresult *incr_res = dbExecutePrepared(conn, incr_sql, 1, lobby_params);
          if (incr_res) PQclear(incr_res);
        }
      }

      PQclear(next_res);

      // Nota: non riordiniamo le "position" degli altri in questa funzione;
      // se necessario, aggiungi una logica che normalizzi le position nella coda.
    } else if (next_res) {
      PQclear(next_res);
    }
  }

  if (!dbCommitTransaction(conn)) {
    dbDisconnect(conn);
    return strdup("{\"result\":false,\"message\":\"Errore commit transazione\",\"content\":null}");
  }

  dbDisconnect(conn);
  return strdup("{\"result\":true,\"message\":\"Giocatore rimosso con successo\",\"content\":null}");
}