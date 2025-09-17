#ifndef AUTH_H
#define AUTH_H

#include <stdbool.h>
#include "dbConnection.h"

typedef enum {
    AUTH_OK = 0,
    AUTH_ERR_DB,
    AUTH_ERR_NOT_FOUND,
    AUTH_ERR_EXEC
} AuthResult;

typedef struct {
    int  id;
    char username[50];
    char email[100];
    char lingua[8];
} AuthUser;


AuthResult authRegister(
    const char *username,
    const char *password_plain,
    const char *email,
    const char *lingua
);


AuthResult authLogin(
    const char *email,
    const char *password_plain,
    AuthUser *out_user
);

#endif
