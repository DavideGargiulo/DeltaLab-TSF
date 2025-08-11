#ifndef DB_CONNECTION_H
#define DB_CONNECTION_H

#include <libpq-fe.h>
#include <time.h>

typedef struct {
  PGconn *conn;
  int inTransaction;
  int connectionTimeout;
  time_t lastActivity;
} DBConnection;

// Connection
DBConnection* dbConnectWithOptions(const char *host, const char *dbname, 
                                   const char *user, const char *password, 
                                   int timeout);
void dbDisconnect(DBConnection *dbConn);
int dbIsConnected(DBConnection *dbConn);

// Transaction
int dbBeginTransaction(DBConnection *dbConn);
int dbCommitTransaction(DBConnection *dbConn);
int dbRollbackTransaction(DBConnection *dbConn);
int dbIsInTransaction(DBConnection *dbConn);

// Query
PGresult* dbExecuteQuery(DBConnection *dbConn, const char *query);
PGresult* dbExecutePrepared(DBConnection *dbConn, const char *query, 
                            int nParams, const char * const *paramValues);
PGresult* dbExecuteQueryWithTimeout(DBConnection *dbConn, const char *query, 
                                    int timeoutSeconds);

// Utility
const char* dbGetErrorMessage(DBConnection *dbConn);
int dbEscapeString(DBConnection *dbConn, char *to, const char *from, size_t length);

// Savepoints
int dbSavepoint(DBConnection *dbConn, const char *name);
int dbRollbackToSavepoint(DBConnection *dbConn, const char *name);
int dbReleaseSavepoint(DBConnection *dbConn, const char *name);

#endif