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
  const int charset_size = sizeof(charset) - 1;
  srand((unsigned int)time(NULL) ^ getpid());
  
  for (int i = 0; i < 6; i++) {
    int key = rand() % charset_size;
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
  int col_id       = PQfnumber(res, "id");
  int col_users    = PQfnumber(res, "utenti_connessi");
  int col_rotation = PQfnumber(res, "rotazione");
  int col_creator  = PQfnumber(res, "id_accountcreatore");
  int col_private  = PQfnumber(res, "is_private");
  int col_status   = PQfnumber(res, "status");

  // Alloca buffer più grande per la risposta JSON
  char *json_response = malloc(8192);
  if (!json_response) {
    PQclear(res);
    dbDisconnect(conn);
    return strdup("{\"result\":false,\"message\":\"Errore memoria\",\"content\":null}");
  }

  // Inizia la risposta JSON
  strcpy(json_response, "{\"result\":true,\"message\":\"Lobby pubbliche trovate\",\"content\":[");
  
  char temp_buffer[512];
  bool first_lobby = true;

  for (int r = 0; r < nrows; ++r) {
    if (!first_lobby) {
      strcat(json_response, ",");
    }
    first_lobby = false;

    // Estrai i valori con controllo null
    const char *id = PQgetisnull(res, r, col_id) ? "" : PQgetvalue(res, r, col_id);
    const char *users = PQgetisnull(res, r, col_users) ? "0" : PQgetvalue(res, r, col_users);
    const char *rotation = PQgetisnull(res, r, col_rotation) ? "orario" : PQgetvalue(res, r, col_rotation);
    const char *creator = PQgetisnull(res, r, col_creator) ? "" : PQgetvalue(res, r, col_creator);
    const char *is_private = PQgetisnull(res, r, col_private) ? "f" : PQgetvalue(res, r, col_private);
    const char *status = PQgetisnull(res, r, col_status) ? "waiting" : PQgetvalue(res, r, col_status);

    // Converti rotation per la risposta JSON
    const char *rotation_json = "clockwise";
    if (strcasecmp(rotation, "antiorario") == 0 || strcasecmp(rotation, "counterclockwise") == 0) {
      rotation_json = "counterclockwise";
    }

    // Costruisci l'oggetto JSON per ogni lobby
    snprintf(temp_buffer, sizeof(temp_buffer),
              "{\"id\":\"%s\",\"utentiConnessi\":%s,\"rotation\":\"%s\","
              "\"creator\":\"%s\",\"isPrivate\":%s,\"status\":\"%s\"}",
              id, users, rotation_json, creator,
              (is_private[0] == 't') ? "true" : "false",
              status);

    // Controlla se abbiamo spazio sufficiente
    if (strlen(json_response) + strlen(temp_buffer) + 10 >= 8192) {
      // Buffer troppo piccolo, interrompi (potresti anche riallocare)
      break;
    }
    
    strcat(json_response, temp_buffer);
  }

  strcat(json_response, "]}");

  PQclear(res);
  dbDisconnect(conn);

  return json_response;
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
  int col_id = PQfnumber(res, "id");
  int col_users = PQfnumber(res, "utenti_connessi");
  int col_rotation = PQfnumber(res, "rotazione");
  int col_creator = PQfnumber(res, "id_accountcreatore");
  int col_private = PQfnumber(res, "is_private");
  int col_status = PQfnumber(res, "status");

  const char *lobby_id = PQgetisnull(res, 0, col_id) ? "" : PQgetvalue(res, 0, col_id);
  const char *users = PQgetisnull(res, 0, col_users) ? "0" : PQgetvalue(res, 0, col_users);
  const char *rotation = PQgetisnull(res, 0, col_rotation) ? "clockwise" : PQgetvalue(res, 0, col_rotation);
  const char *creator = PQgetisnull(res, 0, col_creator) ? "" : PQgetvalue(res, 0, col_creator);
  const char *is_private = PQgetisnull(res, 0, col_private) ? "false" : PQgetvalue(res, 0, col_private);
  const char *status = PQgetisnull(res, 0, col_status) ? "waiting" : PQgetvalue(res, 0, col_status);

  // Alloca buffer per la risposta JSON
  char *json_response = malloc(1024);
  if (!json_response) {
    PQclear(res);
    dbDisconnect(conn);
    return strdup("{\"result\":false,\"message\":\"Errore memoria\",\"content\":null}");
  }

  // Costruisci la risposta JSON
  snprintf(json_response, 1024,
    "{\"result\":true,\"message\":\"Lobby trovata\",\"content\":{"
    "\"id\":\"%s\",\"utentiConnessi\":%s,\"rotation\":\"%s\","
    "\"creator\":\"%s\",\"isPrivate\":%s,\"status\":\"%s\"}}",
    lobby_id, users, rotation, creator, 
    (strcmp(is_private, "t") == 0 || strcmp(is_private, "true") == 0) ? "true" : "false",
    status);

  PQclear(res);
  dbDisconnect(conn);
  
  return json_response;
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
  char *json_response = malloc(1024);
  if (!json_response) {
    return strdup("{\"result\":false,\"message\":\"Errore di memoria\",\"content\":null}");
  }
  
  snprintf(json_response, 1024,
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
  
  return json_response;
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
  int col_id = PQfnumber(res, "id");
  int col_nickname = PQfnumber(res, "nickname");
  int col_email = PQfnumber(res, "email");
  int col_lingua = PQfnumber(res, "lingua");

  const char *id = PQgetisnull(res, 0, col_id) ? "" : PQgetvalue(res, 0, col_id);
  const char *nickname = PQgetisnull(res, 0, col_nickname) ? "" : PQgetvalue(res, 0, col_nickname);
  const char *email = PQgetisnull(res, 0, col_email) ? "" : PQgetvalue(res, 0, col_email);
  const char *lingua = PQgetisnull(res, 0, col_lingua) ? "it" : PQgetvalue(res, 0, col_lingua);

  // Alloca buffer per la risposta JSON
  char *json_response = malloc(1024);
  if (!json_response) {
    PQclear(res);
    dbDisconnect(conn);
    return strdup("{\"result\":false,\"message\":\"Errore memoria\",\"content\":null}");
  }

  // Costruisci la risposta JSON
  snprintf(json_response, 1024,
    "{\"result\":true,\"message\":\"Giocatore trovato\",\"content\":{"
    "\"id\":%s,\"nickname\":\"%s\",\"email\":\"%s\",\"lingua\":\"%s\"}}",
    id, nickname, email, lingua);

  PQclear(res);
  dbDisconnect(conn);
  
  return json_response;
}