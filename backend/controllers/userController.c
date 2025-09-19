#include "userController.h"
#include "../httpUtils.h"
#include "../moduli/lobby.h"   // o il modulo dove c'è getPlayerInfoById
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

bool user_controller(struct mg_connection* c, struct mg_http_message* hm) {
  if (path_starts(hm->uri, "/user/") && mg_strcmp(hm->method, mg_str("GET")) == 0) {
    size_t id_len = hm->uri.len - 6;
    if (id_len < 1 || id_len > 10) {
      send_json(c, 400, "{\"result\":false,\"message\":\"ID giocatore non valido\",\"content\":null}");
      return true;
    }
    char idstr[12] = {0};
    snprintf(idstr, sizeof(idstr), "%.*s", (int) id_len, hm->uri.buf + 6);
    int player_id = atoi(idstr);

    char *resp = getPlayerInfoById(player_id);
    int code = (strstr(resp, "\"result\":true") != NULL) ? 200 : 404;
    send_json(c, code, resp);
    free(resp);
    return true;
  }
  return false;
}
