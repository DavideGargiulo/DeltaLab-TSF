#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include "dbConnection.h"

// Variabile globale per il timeout
static volatile int queryTimeoutOccurred = 0;

// Handler per il timeout
static void timeoutHandler(int sig) {
  queryTimeoutOccurred = 1;
}

DBConnection* dbConnect(const char *conninfo) {
  return dbConnectWithOptions(NULL, NULL, NULL, NULL, 30);
}

DBConnection* dbConnectWithOptions(const char *host, const char *dbname, 
                                   const char *user, const char *password, 
                                   int timeout) {
  DBConnection *dbConn = malloc(sizeof(DBConnection));
  if (!dbConn) {
    fprintf(stderr, "Errore allocazione memoria per DBConnection\n");
    return NULL;
  }
  
  // Inizializza la struttura
  dbConn->conn = NULL;
  dbConn->inTransaction = 0;
  dbConn->connectionTimeout = timeout;
  dbConn->lastActivity = time(NULL);

  // Costruisce la stringa di connessione per libpq
  char conninfoBuffer[512];
  if (host && dbname && user && password) {
    snprintf(conninfoBuffer, sizeof(conninfoBuffer),
             "host=%s dbname=%s user=%s password=%s connect_timeout=%d",
             host, dbname, user, password, timeout);
  } else {
    // Default: connessione locale su postgres
    snprintf(conninfoBuffer, sizeof(conninfoBuffer),
             "host=localhost dbname=postgres user=postgres connect_timeout=%d",
             timeout);
  }
  
  printf("Tentativo di connessione al database...\n");
  
  dbConn->conn = PQconnectdb(conninfoBuffer);
  
  if (PQstatus(dbConn->conn) != CONNECTION_OK) {
    fprintf(stderr, "Connessione fallita: %s", PQerrorMessage(dbConn->conn));
    PQfinish(dbConn->conn);
    free(dbConn);
    return NULL;
  }
  
  // Configura encoding UTF8
  PQsetClientEncoding(dbConn->conn, "UTF8");
  
  printf("Connessione al database stabilita con successo\n");
  return dbConn;
}

void dbDisconnect(DBConnection *dbConn) {
  if (dbConn) {
    // Se c'è una transazione aperta, fa rollback automatico
    if (dbConn->inTransaction) {
      fprintf(stderr, "Rollback automatico della transazione pendente\n");
      dbRollbackTransaction(dbConn);
    }
    
    if (dbConn->conn) {
      PQfinish(dbConn->conn);
    }
    
    printf("Connessione al database chiusa\n");
    free(dbConn);
  }
}

int dbIsConnected(DBConnection *dbConn) {
  if (!dbConn || !dbConn->conn) {
    return 0;
  }
  
  return PQstatus(dbConn->conn) == CONNECTION_OK;
}

int dbPing(DBConnection *dbConn) {
  if (!dbIsConnected(dbConn)) {
    return 0;
  }
  
  PGresult *res = PQexec(dbConn->conn, "SELECT 1");
  if (!res || PQresultStatus(res) != PGRES_TUPLES_OK) {
    if (res) PQclear(res);
    return 0;
  }
  
  PQclear(res);
  dbConn->lastActivity = time(NULL);
  return 1;
}

int dbBeginTransaction(DBConnection *dbConn) {
  if (!dbConn || !dbConn->conn) {
    fprintf(stderr, "Connessione database non valida\n");
    return 0;
  }
  
  if (!dbIsConnected(dbConn)) {
    fprintf(stderr, "Connessione database non attiva\n");
    return 0;
  }
  
  if (dbConn->inTransaction) {
    fprintf(stderr, "Transazione già in corso\n");
    return 0;
  }
  
  // Avvia transazione
  PGresult *res = PQexec(dbConn->conn, "BEGIN");
  if (PQresultStatus(res) != PGRES_COMMAND_OK) {
    fprintf(stderr, "Errore nell'avvio della transazione: %s", 
            PQerrorMessage(dbConn->conn));
    PQclear(res);
    return 0;
  }
  
  PQclear(res);
  dbConn->inTransaction = 1; // Segna che una transazione è attiva
  dbConn->lastActivity = time(NULL);
  printf("Transazione avviata con successo\n");
  return 1;
}

int dbCommitTransaction(DBConnection *dbConn) {
  if (!dbConn || !dbConn->conn) {
    fprintf(stderr, "Connessione database non valida\n");
    return 0;
  }
  
  if (!dbConn->inTransaction) {
    fprintf(stderr, "Nessuna transazione attiva da committare\n");
    return 0;
  }

  // Esegue COMMIT
  PGresult *res = PQexec(dbConn->conn, "COMMIT");
  if (PQresultStatus(res) != PGRES_COMMAND_OK) {
    fprintf(stderr, "Errore nel commit: %s", PQerrorMessage(dbConn->conn));
    PQclear(res);
    return 0;
  }
  
  PQclear(res);
  dbConn->inTransaction = 0; // Transazione conclusa
  dbConn->lastActivity = time(NULL);
  printf("Transazione committata con successo\n");
  return 1;
}

int dbRollbackTransaction(DBConnection *dbConn) {
  if (!dbConn || !dbConn->conn) {
    fprintf(stderr, "Connessione database non valida\n");
    return 0;
  }
  
  if (!dbConn->inTransaction) {
    fprintf(stderr, "Nessuna transazione attiva da rollbackare\n");
    return 0;
  }
  
  // Esegue ROLLBACK
  PGresult *res = PQexec(dbConn->conn, "ROLLBACK");
  if (PQresultStatus(res) != PGRES_COMMAND_OK) {
    fprintf(stderr, "Errore nel rollback: %s", PQerrorMessage(dbConn->conn));
    PQclear(res);
    return 0;
  }
  
  PQclear(res);
  dbConn->inTransaction = 0;
  dbConn->lastActivity = time(NULL);
  printf("Rollback eseguito con successo\n");
  return 1;
}

int dbIsInTransaction(DBConnection *dbConn) {
  return dbConn ? dbConn->inTransaction : 0;
}

PGresult* dbExecuteQuery(DBConnection *dbConn, const char *query) {
  if (!dbConn || !dbConn->conn || !query) {
    fprintf(stderr, "Parametri non validi per l'esecuzione della query\n");
    return NULL;
  }
  
  if (!dbIsConnected(dbConn)) {
    fprintf(stderr, "Connessione database non attiva\n");
    return NULL;
  }
  
  // Esegue query diretta
  PGresult *result = PQexec(dbConn->conn, query);
  dbConn->lastActivity = time(NULL);
  
  if (!result) {
    fprintf(stderr, "Errore nell'esecuzione della query: %s", 
            PQerrorMessage(dbConn->conn));
    return NULL;
  }
  
  // Controlla esito (solo OK o ritorno di tuple sono accettati)
  ExecStatusType status = PQresultStatus(result);
  if (status != PGRES_COMMAND_OK && status != PGRES_TUPLES_OK) {
    fprintf(stderr, "Query fallita: %s", PQresultErrorMessage(result));
    PQclear(result);
    return NULL;
  }

  return result; // Il chiamante deve fare PQclear()
}

PGresult* dbExecutePrepared(DBConnection *dbConn, const char *query, 
                            int nParams, const char * const *paramValues) {
  if (!dbConn || !dbConn->conn || !query) {
    fprintf(stderr, "Parametri non validi per la query preparata\n");
    return NULL;
  }
  
  if (!dbIsConnected(dbConn)) {
    fprintf(stderr, "Connessione database non attiva\n");
    return NULL;
  }
  
  // Genera un nome univoco per la query preparata
  static int stmtCounter = 0;
  char stmtName[64];
  snprintf(stmtName, sizeof(stmtName), "stmt_%d", ++stmtCounter);
  
  // Prepara la query
  PGresult *prepResult = PQprepare(dbConn->conn, stmtName, query, nParams, NULL);
  if (PQresultStatus(prepResult) != PGRES_COMMAND_OK) {
    fprintf(stderr, "Errore nella preparazione: %s", 
            PQerrorMessage(dbConn->conn));
    PQclear(prepResult);
    return NULL;
  }
  PQclear(prepResult);
  
  // Esegue la query preparata
  PGresult *result = PQexecPrepared(dbConn->conn, stmtName, nParams, 
                                    paramValues, NULL, NULL, 0);
  dbConn->lastActivity = time(NULL);
  
  if (!result) {
    fprintf(stderr, "Errore nell'esecuzione della query preparata: %s", 
            PQerrorMessage(dbConn->conn));
    return NULL;
  }
  
  ExecStatusType status = PQresultStatus(result);
  if (status != PGRES_COMMAND_OK && status != PGRES_TUPLES_OK) {
    fprintf(stderr, "Query preparata fallita: %s", 
            PQresultErrorMessage(result));
    PQclear(result);
    return NULL;
  }
  
  return result;
}

PGresult* dbExecuteQueryWithTimeout(DBConnection *dbConn, const char *query, 
                                    int timeoutSeconds) {
  if (!dbConn || !query || timeoutSeconds <= 0) {
    return NULL;
  }

  // Approccio non-blocking
  if (PQsetnonblocking(dbConn->conn, 1) != 0) {
    fprintf(stderr, "Errore nel setting non-blocking mode\n");
    return NULL;
  }

  // Invia query asincrona
  if (PQsendQuery(dbConn->conn, query) != 1) {
    fprintf(stderr, "Errore nell'invio query asincrona: %s", 
            PQerrorMessage(dbConn->conn));
    PQsetnonblocking(dbConn->conn, 0);
    return NULL;
  }

  // Attendi risultato con timeout usando select()
  fd_set input_mask;
  struct timeval timeout;
  int sock = PQsocket(dbConn->conn);
  
  time_t start_time = time(NULL);
  while (1) {
    // Controlla se ci sono dati pronti
    if (PQconsumeInput(dbConn->conn) == 0) {
      fprintf(stderr, "Errore nel consumo input: %s", 
              PQerrorMessage(dbConn->conn));
      break;
    }

    if (PQisBusy(dbConn->conn) == 0) {
      // Query completata
      PGresult *result = PQgetResult(dbConn->conn);
      PQsetnonblocking(dbConn->conn, 0);
      
      // Consuma eventuali risultati rimanenti
      PGresult *next;
      while ((next = PQgetResult(dbConn->conn)) != NULL) {
        PQclear(next);
      }
      
      return result;
    }

    // Controlla timeout
    if (time(NULL) - start_time >= timeoutSeconds) {
      fprintf(stderr, "Query timeout dopo %d secondi\n", timeoutSeconds);
      
      // Cancella query
      PGcancel *cancel = PQgetCancel(dbConn->conn);
      if (cancel) {
        PQcancel(cancel, NULL, 0);
        PQfreeCancel(cancel);
      }
      
      PQsetnonblocking(dbConn->conn, 0);
      return NULL;
    }

    // Aspetta con select()
    FD_ZERO(&input_mask);
    FD_SET(sock, &input_mask);
    timeout.tv_sec = 1;
    timeout.tv_usec = 0;
    
    if (select(sock + 1, &input_mask, NULL, NULL, &timeout) < 0) {
      fprintf(stderr, "Errore in select()\n");
      break;
    }
  }

  PQsetnonblocking(dbConn->conn, 0);
  return NULL;
}

const char* dbGetErrorMessage(DBConnection *dbConn) {
  if (!dbConn || !dbConn->conn) {
    return "Connessione non valida";
  }
  
  return PQerrorMessage(dbConn->conn);
}

int dbEscapeString(DBConnection *dbConn, char *to, const char *from, size_t length) {
  if (!dbConn || !dbConn->conn || !to || !from) {
    return 0;
  }
  
  int error;
  size_t result = PQescapeStringConn(dbConn->conn, to, from, length, &error);
  
  if (error) {
    fprintf(stderr, "Errore nell'escape della stringa: %s", 
            PQerrorMessage(dbConn->conn));
    return 0;
  }
  
  return (int)result;
}

// Funzioni per savepoint (transazioni annidate)
int dbSavepoint(DBConnection *dbConn, const char *name) {
  if (!dbConn || !name || !dbConn->inTransaction) {
    return 0;
  }
  
  char query[256];
  snprintf(query, sizeof(query), "SAVEPOINT %s", name);
  
  PGresult *res = dbExecuteQuery(dbConn, query);
  if (!res) {
    return 0;
  }
  
  PQclear(res);
  printf("Savepoint '%s' creato\n", name);
  return 1;
}

int dbRollbackToSavepoint(DBConnection *dbConn, const char *name) {
  if (!dbConn || !name || !dbConn->inTransaction) {
    return 0;
  }
  
  char query[256];
  snprintf(query, sizeof(query), "ROLLBACK TO SAVEPOINT %s", name);
  
  PGresult *res = dbExecuteQuery(dbConn, query);
  if (!res) {
    return 0;
  }
  
  PQclear(res);
  printf("Rollback al savepoint '%s' eseguito\n", name);
  return 1;
}

int dbReleaseSavepoint(DBConnection *dbConn, const char *name) {
  if (!dbConn || !name || !dbConn->inTransaction) {
    return 0;
  }
  
  char query[256];
  snprintf(query, sizeof(query), "RELEASE SAVEPOINT %s", name);
  
  PGresult *res = dbExecuteQuery(dbConn, query);
  if (!res) {
    return 0;
  }
  
  PQclear(res);
  printf("Savepoint '%s' rilasciato\n", name);
  return 1;
}