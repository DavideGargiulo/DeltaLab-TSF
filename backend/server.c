#include <stdio.h>
#include <stdlib.h>
#include "moduli/dbConnection.h"
#include "moduli/auth.h"

int main() {
  // Connessione con parametri personalizzati
  printf("Inizio test...\n");
  DBConnection *conn = dbConnectWithOptions("db", "deltalabtsf", "postgres", "admin", 30);
   if (!conn) {
        fprintf(stderr, "Connessione fallita\n");
        return 1;
    }

    // crea le tabelle se non esistono
    // if (!createTables(conn)) {
    //     fprintf(stderr, "Creazione tabelle fallita\n");
    //     dbDisconnect(conn);
    //     return 1;
    // }

    PGresult *res;

    res = dbExecuteQuery(conn,
    "INSERT INTO account (nickname, email, password, lingua) "
    "VALUES ('gege', 'a@a.a', md5('avv'), 'it')");
    if (res) {
        PQclear(res);
        printf("✅ Account creato con successo\n");
    } else {
        fprintf(stderr, "❌ Errore creazione account\n");
    }



    dbDisconnect(conn);
  printf("Test completato\n");
  return 0;
}