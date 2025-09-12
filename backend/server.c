#include <stdio.h>
#include <stdlib.h>
#include "moduli/dbConnection.h"

int main() {
  // Connessione con parametri personalizzati
  printf("Inizio test...\n");
  DBConnection *conn = dbConnectWithOptions("db", "deltalabtsf", "postgres", "admin", 30);
   if (!conn) {
        fprintf(stderr, "Connessione fallita\n");
        return 1;
    }

    if (!dbBeginTransaction(conn)) {
        fprintf(stderr, "Impossibile iniziare transazione\n");
        dbDisconnect(conn);
        return 1;
    }

    // Query valida
    PGresult *res = dbExecuteQuery(conn, "INSERT INTO test_table(val) VALUES ('ok')");
    if (res) PQclear(res);

    // Query volutamente sbagliata
    res = dbExecuteQuery(conn, "INSERT INTO test_table(val) VALUES ('errore')");
    if (!res) {
        fprintf(stderr, "Errore rilevato, eseguo rollback\n");
        dbRollbackTransaction(conn);
    } else {
        PQclear(res);
        dbCommitTransaction(conn);
    }

    dbDisconnect(conn);
  printf("Test completato\n");
  return 0;
}