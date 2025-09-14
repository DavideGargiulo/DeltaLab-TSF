#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include <sys/socket.h>

#include "auth.h"
#include "dbConnection.h"

static void send_json(int client_socket, int status_code, const char* status_text, const char* json_body) {
    char header[256];
    snprintf(header, sizeof(header),
             "HTTP/1.1 %d %s\r\n"
             "Content-Type: application/json\r\n"
             "Content-Length: %zu\r\n\r\n",
             status_code, status_text, strlen(json_body));
    send(client_socket, header, strlen(header), 0);
    send(client_socket, json_body, strlen(json_body), 0);
}



void authRegister(int client_socket, const char *request) {
    char username[50] = {0};
    char password[255] = {0};
    char email[100] = {0};
    char lingua[8] = {0};

    if (!parse_json_field(request, "username", username) || username[0] == '\0' ||
        !parse_json_field(request, "password", password) || password[0] == '\0' ||
        !parse_json_field(request, "email", email)       || email[0] == '\0') {

        send_json(client_socket, 400, "Bad Request",
                  "{\"status\":\"error\",\"message\":\"username, password ed email sono obbligatori\"}");
        return;
    }
    if (!parse_json_field(request, "lingua", lingua) || lingua[0] == '\0') {
        strcpy(lingua, "it");
    }

    DBConnection *db = dbConnectWithOptions("db", "deltalabtsf", "postgres", "admin", 30);
    if (!db) {
        send_json(client_socket, 500, "Internal Server Error",
                  "{\"status\":\"error\",\"message\":\"Connessione al DB fallita\"}");
        return;
    }

    if (!dbBeginTransaction(db)) {
        dbDisconnect(db);
        send_json(client_socket, 500, "Internal Server Error",
                  "{\"status\":\"error\",\"message\":\"Impossibile avviare transazione\"}");
        return;
    }

    srand((unsigned int)time(NULL) ^ (unsigned int)getpid());

    const char *sql =
        "INSERT INTO account (nickname, email, password, lingua) "
        "VALUES ($1, $2, md5($3), $4)";

    const char *params[4] = { username, email, password, lingua };

    PGresult *res = dbExecutePrepared(db, sql, 4, params);
    if (!res) {
        dbRollbackTransaction(db);
        dbDisconnect(db);
        send_json(client_socket, 500, "Internal Server Error",
                  "{\"status\":\"error\",\"message\":\"Errore durante la registrazione\"}");
        return;
    }

    ExecStatusType st = PQresultStatus(res);
    if (st != PGRES_COMMAND_OK) {
        const char *sqlstate = PQresultErrorField(res, PG_DIAG_SQLSTATE);
        PQclear(res);
        dbRollbackTransaction(db);
        dbDisconnect(db);

        if (sqlstate && strcmp(sqlstate, "23505") == 0) {
            send_json(client_socket, 409, "Conflict",
                      "{\"status\":\"error\",\"message\":\"Username o email già in uso\"}");
        } else {
            send_json(client_socket, 500, "Internal Server Error",
                      "{\"status\":\"error\",\"message\":\"Errore durante la registrazione\"}");
        }
        return;
    }

    PQclear(res);

    if (!dbCommitTransaction(db)) {
        dbDisconnect(db);
        send_json(client_socket, 500, "Internal Server Error",
                  "{\"status\":\"error\",\"message\":\"Commit fallito\"}");
        return;
    }

    dbDisconnect(db);
    send_json(client_socket, 200, "OK",
              "{\"status\":\"success\",\"message\":\"Registrazione completata\"}");
}



void authLogin(int client_socket, const char *request) {
    char username[100] = {0};
    char password[255] = {0};

    if (!parse_json_field(request, "username", username) || username[0] == '\0' ||
        !parse_json_field(request, "password", password) || password[0] == '\0') {
        send_json(client_socket, 400, "Bad Request",
                  "{\"status\":\"error\",\"message\":\"username e password sono obbligatori\"}");
        return;
    }

    DBConnection *db = dbConnectWithOptions("db", "deltalabtsf", "postgres", "admin", 30);
    if (!db) {
        send_json(client_socket, 500, "Internal Server Error",
                  "{\"status\":\"error\",\"message\":\"Connessione al DB fallita\"}");
        return;
    }


    const char *sql =
        "SELECT 1 FROM account WHERE nickname = $1 AND password = md5($2) LIMIT 1";

    const char *params[2] = { username, password };

    PGresult *res = dbExecutePrepared(db, sql, 2, params);
    if (!res) {
        dbDisconnect(db);
        send_json(client_socket, 500, "Internal Server Error",
                  "{\"status\":\"error\",\"message\":\"Errore server\"}");
        return;
    }

    bool ok = (PQresultStatus(res) == PGRES_TUPLES_OK && PQntuples(res) > 0);
    PQclear(res);
    dbDisconnect(db);

    if (ok) {
        send_json(client_socket, 200, "OK",
                  "{\"status\":\"success\",\"message\":\"Login riuscito\"}");
    } else {
        send_json(client_socket, 401, "Unauthorized",
                  "{\"status\":\"error\",\"message\":\"Credenziali errate\"}");
    }
}
