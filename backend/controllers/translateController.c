#include "translateController.h"
#include "../httpUtils.h"
#include "../moduli/translate.h"
#include <string.h>
#include <stdio.h>

bool translate_controller(struct mg_connection* c, struct mg_http_message* hm) {
  if (!(path_eq(hm->uri, "/translate") && mg_strcmp(hm->method, mg_str("POST")) == 0))
    return false;

  char text[4096]={0}, source[16]={0}, target[16]={0};
  if (!json_get_str(hm->body, "$.text", text, sizeof(text)) ||
      !json_get_str(hm->body, "$.source", source, sizeof(source)) ||
      !json_get_str(hm->body, "$.target", target, sizeof(target))) {
    send_json(c, 400, "{\"result\":false,\"message\":\"text/source/target richiesti\",\"content\":null}");
    return true;
  }

  char *resp = translate_text(text, source, target);
  if (!resp) { send_json(c, 502, "{\"result\":false,\"message\":\"Traduttore non disponibile\",\"content\":null}"); return true; }

  // Se contiene "error", trattala come errore (copertura robusta)
  if (strstr(resp, "\"error\"")) {
    int toklen = 0, ofs = mg_json_get(mg_str(resp), "$.error", &toklen);
    char msg[512] = {0};
    if (ofs >= 0 && toklen > 0) {
      const char *ptr = resp + ofs;
      if (toklen >= 2 && ptr[0] == '"' && ptr[toklen-1] == '"') { ptr++; toklen -= 2; }
      size_t copy = (size_t) toklen < sizeof(msg) - 1 ? (size_t) toklen : sizeof(msg) - 1;
      memcpy(msg, ptr, copy); msg[copy] = '\0';
    } else {
      snprintf(msg, sizeof(msg), "Errore traduzione");
    }
    char out[800];
    snprintf(out, sizeof(out), "{\"result\":false,\"message\":\"%s\",\"content\":null}", msg);
    send_json(c, 502, out);
    free(resp);
    return true;
  }

  // Successo: resp = testo tradotto (plain), escapalo
  char esc[8192];
  escape_json_string(esc, resp, sizeof(esc));
  char out[8448];
  snprintf(out, sizeof(out),
           "{\"result\":true,\"message\":\"Traduzione OK\",\"content\":{\"translated\":\"%s\"}}",
           esc);
  send_json(c, 200, out);
  free(resp);
  return true;
}
