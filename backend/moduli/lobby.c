#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <libpq-fe.h>
#include "dbConnection.h"
#include "lobby.h"

List* getAllRoom() {
  DBConnection *db_conn = dbConnectWithOptions("localhost", "deltalabtsf", "postgres", "admin", 30);
  if (!db_conn) {
    return NULL;
  }

  if (!dbBeginTransaction(db_conn)) {
    dbDisconnect(db_conn);
    return NULL;
  }

  PGresult *res = dbExecuteQuery(db_conn, "SELECT * FROM rooms");
  
  if (PQresultStatus(res) != PGRES_TUPLES_OK) {
    fprintf(stderr, "Errore nella query: %s", PQerrorMessage(db_conn->conn));
    PQclear(res);
    dbRollbackTransaction(db_conn);
    dbDisconnect(db_conn);
    return NULL;
  }
  
  // Qui elabori i risultati e crei la lista
  List *room_list = NULL; // Implementa la logica per creare la lista
  
  PQclear(res);
  dbCommitTransaction(db_conn);
  dbDisconnect(db_conn);

  return room_list;
}