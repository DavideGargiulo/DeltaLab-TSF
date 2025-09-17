#include "auth.h"
#include <string.h>
#include <stdlib.h>
#include <libpq-fe.h>

AuthResult authRegister(
    const char *username,
    const char *password_plain,
    const char *email,
    const char *lingua
) {
    if (!username || !password_plain || !email) {
        return AUTH_ERR_EXEC;
    }
    if (!lingua || lingua[0] == '\0') lingua = "it";

    DBConnection *db = dbConnectWithOptions("db", "deltalabtsf", "postgres", "admin", 30);
    if (!db) return AUTH_ERR_DB;

    if (!dbBeginTransaction(db)) {
        dbDisconnect(db);
        return AUTH_ERR_DB;
    }

    const char *sql =
        "INSERT INTO account (nickname, email, password, lingua) "
        "VALUES ($1, $2, md5($3), $4)";

    const char *params[4] = { username, email, password_plain, lingua };

    PGresult *res = dbExecutePrepared(db, sql, 4, params);
    if (!res) {
        dbRollbackTransaction(db);
        dbDisconnect(db);
        return AUTH_ERR_DB;
    }

    if (PQresultStatus(res) != PGRES_COMMAND_OK) {
        const char *sqlstate = PQresultErrorField(res, PG_DIAG_SQLSTATE);
        PQclear(res);
        dbRollbackTransaction(db);
        dbDisconnect(db);

        if (sqlstate && strcmp(sqlstate, "23505") == 0) {
            return AUTH_ERR_EXEC; // duplicate key → potresti mappare AUTH_ERR_UNIQUE se vuoi mantenerlo
        }
        return AUTH_ERR_EXEC;
    }

    PQclear(res);

    if (!dbCommitTransaction(db)) {
        dbDisconnect(db);
        return AUTH_ERR_DB;
    }

    dbDisconnect(db);
    return AUTH_OK;
}

AuthResult authLogin(
    const char *email,
    const char *password_plain,
    AuthUser *out_user
) {
    if (!email || !password_plain || !out_user) {
        return AUTH_ERR_EXEC;
    }

    DBConnection *db = dbConnectWithOptions("db", "deltalabtsf", "postgres", "admin", 30);
    if (!db) return AUTH_ERR_DB;

    const char *sql =
        "SELECT id, nickname, email, lingua "
        "FROM account "
        "WHERE email = $1 AND password = md5($2) "
        "LIMIT 1";
    const char *params[2] = { email, password_plain };

    PGresult *res = dbExecutePrepared(db, sql, 2, params);
    if (!res) {
        dbDisconnect(db);
        return AUTH_ERR_DB;
    }

    if (PQresultStatus(res) != PGRES_TUPLES_OK) {
        PQclear(res);
        dbDisconnect(db);
        return AUTH_ERR_EXEC;
    }

    if (PQntuples(res) == 0) {
        PQclear(res);
        dbDisconnect(db);
        return AUTH_ERR_NOT_FOUND;
    }

    // Copia i valori nella struct
    memset(out_user, 0, sizeof(AuthUser));
    out_user->id = atoi(PQgetvalue(res, 0, 0));
    strncpy(out_user->username, PQgetvalue(res, 0, 1), sizeof(out_user->username) - 1);
    strncpy(out_user->email,    PQgetvalue(res, 0, 2), sizeof(out_user->email) - 1);
    strncpy(out_user->lingua,   PQgetvalue(res, 0, 3), sizeof(out_user->lingua) - 1);

    PQclear(res);
    dbDisconnect(db);
    return AUTH_OK;
}
