#include "authController.h"
#include "../httpUtils.h"
#include "../moduli/auth.h"
#include <string.h>
#include <stdio.h>

bool auth_controller(struct mg_connection* c, struct mg_http_message* hm) {
  if (path_eq(hm->uri, "/auth/login") && mg_strcmp(hm->method, mg_str("POST")) == 0) {
    char email[128]={0}, password[256]={0};
    if (!json_get_str(hm->body, "$.email", email, sizeof(email)) ||
        !json_get_str(hm->body, "$.password", password, sizeof(password))) {
      send_json(c, 400, "{\"result\":false,\"message\":\"email/password richiesti\",\"content\":null}");
      return true;
    }
    AuthUser u; AuthResult r = authLogin(email, password, &u);
    if (r == AUTH_OK) {
      char buf[512];
      snprintf(buf, sizeof(buf),
        "{\"result\":true,\"message\":\"Login OK\",\"content\":{"
        "\"id\":%d,\"username\":\"%s\",\"email\":\"%s\",\"lingua\":\"%s\"}}",
        u.id, u.username, u.email, u.lingua);
      send_json(c, 200, buf);
    } else if (r == AUTH_ERR_NOT_FOUND) {
      send_json(c, 401, "{\"result\":false,\"message\":\"Credenziali errate\",\"content\":null}");
    } else {
      send_json(c, 500, "{\"result\":false,\"message\":\"Errore DB\",\"content\":null}");
    }
    return true;
  }

  if (path_eq(hm->uri, "/auth/register") && mg_strcmp(hm->method, mg_str("POST")) == 0) {
    char username[64]={0}, email[128]={0}, password[256]={0}, lingua[8]={0};
    if (!json_get_str(hm->body, "$.username", username, sizeof(username)) ||
        !json_get_str(hm->body, "$.email",    email,    sizeof(email))    ||
        !json_get_str(hm->body, "$.password", password, sizeof(password)) ||
        !json_get_str(hm->body, "$.lingua",   lingua,   sizeof(lingua))) {
      send_json(c, 400, "{\"result\":false,\"message\":\"username/email/password/lingua richiesti\",\"content\":null}");
      return true;
    }
    AuthResult res = authRegister(username, password, email, lingua);
    if (res == AUTH_ERR_DB) {
      send_json(c, 409, "{\"result\":false,\"message\":\"Email già registrata\",\"content\":null}");
    } else if (res != AUTH_OK) {
      send_json(c, 500, "{\"result\":false,\"message\":\"Errore DB\",\"content\":null}");
    } else {
      send_json(c, 201, "{\"result\":true,\"message\":\"Utente registrato\",\"content\":null}");
    }
    return true;
  }

  return false; // non gestita da questo controller
}
