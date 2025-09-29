#include "lobbyController.h"
#include "../httpUtils.h"
#include "../moduli/lobby.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

bool lobby_controller(struct mg_connection* c, struct mg_http_message* hm) {
  // GET /lobby
  if (path_eq(hm->uri, "/lobby") && mg_strcmp(hm->method, mg_str("GET")) == 0) {
    char *response = getAllLobbies();
    int code = (strstr(response, "\"result\":true") != NULL) ? 200 : 500;
    send_json(c, code, response);
    free(response);
    return true;
  }

  // GET /lobby/:id/players
  if (path_starts(hm->uri, "/lobby/") && mg_strcmp(hm->method, mg_str("GET")) == 0) {
    size_t uri_len = hm->uri.len;
    if (uri_len > 8 && strncmp(hm->uri.buf + uri_len - 8, "/players", 8) == 0) {
      size_t id_len = uri_len - 15;  // "/lobby/" (7) + "/players" (8) = 15, so id_len = uri_len - 15
      if (id_len < 1 || id_len > 6) {
        send_json(c, 400, "{\"result\":false,\"message\":\"ID lobby non valido\",\"content\":null}");
        return true;
      }
      char lobby_id[8] = {0};
      snprintf(lobby_id, sizeof(lobby_id), "%.*s", (int)id_len, hm->uri.buf + 7);
      char *response = getLobbyPlayers(lobby_id);
      int code = (strstr(response, "\"result\":true") != NULL) ? 200 : 404;
      send_json(c, code, response);
      free(response);
      return true;
    }
  }

  // POST /lobby
  if (path_eq(hm->uri, "/lobby") && mg_strcmp(hm->method, mg_str("POST")) == 0) {
    char *body = (char *) malloc(hm->body.len + 1);
    if (!body) { send_json(c, 500, "{\"result\":false,\"message\":\"Errore di memoria\",\"content\":null}"); return true; }
    memcpy(body, hm->body.buf, hm->body.len); body[hm->body.len] = '\0';

    char *response = createLobbyEndpoint(body);
    int code = (strstr(response, "\"result\":true") != NULL) ? 201 : 400;
    send_json(c, code, response);
    free(body);
    free(response);
    return true;
  }

  // DELETE /lobby/:id
  if (path_starts(hm->uri, "/lobby/") && mg_strcmp(hm->method, mg_str("DELETE")) == 0) {
    size_t id_len = hm->uri.len - 7;
    if (id_len < 1 || id_len > 6) {
      send_json(c, 400, "{\"result\":false,\"message\":\"ID lobby non valido\",\"content\":null}");
      return true;
    }
    char lobby_id[8] = {0};
    snprintf(lobby_id, sizeof(lobby_id), "%.*s", (int) id_len, hm->uri.buf + 7);

    int creatorId = 0;
    if (!json_get_int(hm->body, "$.creatorId", &creatorId) || creatorId <= 0) {
      send_json(c, 400, "{\"result\":false,\"message\":\"creatorId richiesto\",\"content\":null}");
      return true;
    }
    char *response = deleteLobby(lobby_id, creatorId);
    int code = (strstr(response, "\"result\":true") != NULL) ? 200 : 404;
    send_json(c, code, response);
    free(response);
    return true;
  }

  // GET /lobby/:id
  if (path_starts(hm->uri, "/lobby/") && mg_strcmp(hm->method, mg_str("GET")) == 0) {
    size_t id_len = hm->uri.len - 7;
    if (id_len < 1 || id_len > 6) {
      send_json(c, 400, "{\"result\":false,\"message\":\"ID lobby non valido\",\"content\":null}");
      return true;
    }
    char lobby_id[8] = {0};
    snprintf(lobby_id, sizeof(lobby_id), "%.*s", (int) id_len, hm->uri.buf + 7);
    char *response = getLobbyById(lobby_id);
    int code = (strstr(response, "\"result\":true") != NULL) ? 200 : 404;
    send_json(c, code, response);
    free(response);
    return true;
  }

  return false;
}
