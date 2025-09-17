#include "mongoose.h"
#include <stdio.h>
#include <string.h>
#include <stdbool.h>

#include "moduli/auth.h"
#include "moduli/lobby.h"
// #include "moduli/dbConnection.h"

static const char *s_listen = "http://0.0.0.0:8080";

// JSON + CORS helper
static void send_json(struct mg_connection *c, int code, const char *json) {
  mg_http_reply(c, code,
    "Content-Type: application/json\r\n"
    "Access-Control-Allow-Origin: *\r\n"
    "Access-Control-Allow-Headers: Content-Type, Authorization\r\n"
    "Access-Control-Allow-Methods: GET, POST, DELETE, OPTIONS\r\n",
    "%s", json);
}

// OPTIONS preflight
static bool handle_cors(struct mg_connection *c, struct mg_http_message *hm) {
  if (mg_strcmp(hm->method, mg_str("OPTIONS")) == 0) {
    mg_http_reply(c, 204,
      "Access-Control-Allow-Origin: *\r\n"
      "Access-Control-Allow-Methods: GET, POST, DELETE, OPTIONS\r\n"
      "Access-Control-Allow-Headers: Content-Type, Authorization\r\n",
      "");
    return true;
  }
  return false;
}

// Estrae stringa JSON usando la firma attuale di mg_json_get
static bool json_get_str(struct mg_str body, const char *jp, char *out, size_t n) {
  int toklen = 0;
  int ofs = mg_json_get(body, jp, &toklen);     // ritorna offset nel buffer
  if (ofs < 0 || toklen <= 0) return false;
  const char *ptr = body.buf + ofs;
  // rimuovi virgolette
  if (toklen >= 2 && ptr[0] == '"' && ptr[toklen - 1] == '"') { ptr++; toklen -= 2; }
  if ((size_t)toklen >= n) toklen = (int)n - 1;
  memcpy(out, ptr, (size_t)toklen);
  out[toklen] = 0;
  return true;
}

static void fn(struct mg_connection *c, int ev, void *ev_data) {
  if (ev == MG_EV_HTTP_MSG) {
    struct mg_http_message *hm = (struct mg_http_message *) ev_data;

    if (handle_cors(c, hm)) return;

    // GET /health
    if (mg_strcmp(hm->uri, mg_str("/health")) == 0 &&
        mg_strcmp(hm->method, mg_str("GET")) == 0) {
      send_json(c, 200, "{\"result\":true,\"message\":\"OK\",\"content\":null}");
      return;
    }

    // POST /auth/login
    if (mg_strcmp(hm->uri, mg_str("/auth/login")) == 0 &&
      mg_strcmp(hm->method, mg_str("POST")) == 0) {
      char email[128]={0}, password[256]={0};
      if (!json_get_str(hm->body, "$.email", email, sizeof(email)) ||
          !json_get_str(hm->body, "$.password", password, sizeof(password))) {
        send_json(c, 400, "{\"result\":false,\"message\":\"email/password richiesti\",\"content\":null}");
        return;
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
      return;
    }

    // POST /auth/register
    if (mg_strcmp(hm->uri, mg_str("/auth/register")) == 0 &&
      mg_strcmp(hm->method, mg_str("POST")) == 0) {
      char username[64]={0}, email[128]={0}, password[256]={0}, lingua[8]={0};
      if (!json_get_str(hm->body, "$.username", username, sizeof(username)) ||
          !json_get_str(hm->body, "$.email",    email,    sizeof(email))    ||
          !json_get_str(hm->body, "$.password", password, sizeof(password)) ||
          !json_get_str(hm->body, "$.lingua",   lingua,   sizeof(lingua))) {
        send_json(c, 400, "{\"result\":false,\"message\":\"username/email/password/lingua richiesti\",\"content\":null}");
        return;
      }
      AuthResult res = authRegister(username, password, email, lingua);
      if (res == AUTH_ERR_DB) {
        send_json(c, 409, "{\"result\":false,\"message\":\"Email già registrata\",\"content\":null}");
        return;
      } else if (res != AUTH_OK) {
        send_json(c, 500, "{\"result\":false,\"message\":\"Errore DB\",\"content\":null}");
        return;
      }
      send_json(c, 201, "{\"result\":true,\"message\":\"Utente registrato\",\"content\":null}");
      return;
    }

    if (mg_strcmp(hm->method, mg_str("GET")) == 0) {
      struct mg_str path = hm->uri;

      // GET /lobby/:id (lobby specifica)
      if (path.len > 7 && memcmp(path.buf, "/lobby/", 7) == 0) {
        size_t id_len = path.len - 7;
        if (id_len >= 1 && id_len <= 6) {  // ID tra 1 e 6 caratteri
          char lobby_id[8] = {0};
          snprintf(lobby_id, sizeof(lobby_id), "%.*s", (int)id_len, path.buf + 7);

          char *response = getLobbyById(lobby_id);

          int status_code = (strstr(response, "\"result\":true") != NULL) ? 200 : 404;

          send_json(c, status_code, response);
          free(response);
          return;
        }

        send_json(c, 400, "{\"result\":false,\"message\":\"ID lobby non valido\",\"content\":null}");
        return;
      }

      // GET /lobby (tutte le lobby) - match esatto
      if (mg_strcmp(path, mg_str("/lobby")) == 0) {
        char *response = getAllLobbies();
        
        int status_code = (strstr(response, "\"result\":true") != NULL) ? 200 : 500;
        
        send_json(c, status_code, response);
        free(response);
        return;
      }
    }

    if (mg_strcmp(hm->uri, mg_str("/lobby")) == 0 &&
      mg_strcmp(hm->method, mg_str("POST")) == 0) {
      
      char *requestBody = malloc(hm->body.len + 1);
      if (!requestBody) {
        send_json(c, 500, "{\"result\":false,\"message\":\"Errore di memoria\",\"content\":null}");
        return;
      }
      
      memcpy(requestBody, hm->body.buf, hm->body.len);
      requestBody[hm->body.len] = '\0';
      
      char *response = createLobbyEndpoint(requestBody);
      
      int status_code = (strstr(response, "\"result\":true") != NULL) ? 201 : 400;
      
      send_json(c, status_code, response);
      
      free(requestBody);
      free(response);
      return;
    }

    if (mg_strcmp(hm->method, mg_str("DELETE")) == 0) {
      struct mg_str path = hm->uri;
      
      if (path.len > 7 && memcmp(path.buf, "/lobby/", 7) == 0) {
        size_t id_len = path.len - 7;
        if (id_len >= 1 && id_len <= 6) {
          char lobby_id[8] = {0};
          snprintf(lobby_id, sizeof(lobby_id), "%.*s", (int)id_len, path.buf + 7);
          
          int creatorId = 0;
          int toklen = 0;
          int ofs = mg_json_get(hm->body, "$.creatorId", &toklen);
          if (ofs >= 0 && toklen > 0) {
            char temp[16] = {0};
            if (toklen < 16) {
              memcpy(temp, hm->body.buf + ofs, toklen);
              creatorId = atoi(temp);
            }
          }
          
          if (creatorId <= 0) {
            send_json(c, 400, "{\"result\":false,\"message\":\"creatorId richiesto\",\"content\":null}");
            return;
          }
          
          char *response = deleteLobby(lobby_id, creatorId);
          
          int status_code = (strstr(response, "\"result\":true") != NULL) ? 200 : 404;
          
          send_json(c, status_code, response);
          free(response);
          return;
        }
        
        send_json(c, 400, "{\"result\":false,\"message\":\"ID lobby non valido\",\"content\":null}");
        return;
      }
    }

    // 404
    send_json(c, 404, "{\"result\":false,\"message\":\"Route non trovata\",\"content\":null}");
  }
}

int main(void) {
  struct mg_mgr mgr;
  mg_mgr_init(&mgr);
  mg_http_listen(&mgr, s_listen, fn, NULL);
  printf("API up su %s\n", s_listen);
  for (;;) mg_mgr_poll(&mgr, 100);
  mg_mgr_free(&mgr);
  return 0;
}
