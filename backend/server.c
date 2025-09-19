#include "mongoose.h"
#include <stdio.h>
#include "httpUtils.h"

// Controllers
#include "controllers/authController.h"
#include "controllers/lobbyController.h"
#include "controllers/userController.h"
#include "controllers/translateController.h"

static const char *s_listen = "http://0.0.0.0:8080";

static void fn(struct mg_connection *c, int ev, void *ev_data) {
  if (ev != MG_EV_HTTP_MSG) return;
  struct mg_http_message *hm = (struct mg_http_message *) ev_data;

  if (handle_cors(c, hm)) return;

  // Health
  if (path_eq(hm->uri, "/health") && mg_strcmp(hm->method, mg_str("GET")) == 0) {
    send_json(c, 200, "{\"result\":true,\"message\":\"OK\",\"content\":null}");
    return;
  }

  // Delego ai controller (ordine: auth, lobby, user, translate)
  if (auth_controller(c, hm)) return;
  if (lobby_controller(c, hm)) return;
  if (user_controller(c, hm)) return;
  if (translate_controller(c, hm)) return;

  // 404
  send_json(c, 404, "{\"result\":false,\"message\":\"Route non trovata\",\"content\":null}");
}

int main(void) {
  struct mg_mgr mgr;
  mg_mgr_init(&mgr);
  struct mg_connection *lc = mg_http_listen(&mgr, s_listen, fn, NULL);
  if (!lc) {
    fprintf(stderr, "ERRORE: bind fallita su %s\n", s_listen);
    mg_mgr_free(&mgr);
    return 1;
  }
  printf("API up su %s\n", s_listen);
  for (;;) mg_mgr_poll(&mgr, 100);
  mg_mgr_free(&mgr);
  return 0;
}