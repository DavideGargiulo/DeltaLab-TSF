#include "httpUtils.h"
#include <string.h>
#include <stdio.h>

void send_json(struct mg_connection *c, int code, const char *json) {
  mg_http_reply(c, code,
    "Content-Type: application/json\r\n"
    "Access-Control-Allow-Origin: *\r\n"
    "Access-Control-Allow-Headers: Content-Type, Authorization\r\n"
    "Access-Control-Allow-Methods: GET, POST, DELETE, OPTIONS\r\n",
    "%s", json);
}

bool handle_cors(struct mg_connection *c, struct mg_http_message *hm) {
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

bool json_get_str(struct mg_str body, const char *jp, char *out, size_t n) {
  int toklen = 0;
  int ofs = mg_json_get(body, jp, &toklen);
  if (ofs < 0 || toklen <= 0) return false;
  const char *ptr = body.buf + ofs;
  if (toklen >= 2 && ptr[0] == '"' && ptr[toklen-1] == '"') { ptr++; toklen -= 2; }
  if ((size_t) toklen >= n) toklen = (int) n - 1;
  memcpy(out, ptr, (size_t) toklen);
  out[toklen] = 0;
  return true;
}

bool json_get_int(struct mg_str body, const char *jp, int *out) {
  int toklen = 0;
  int ofs = mg_json_get(body, jp, &toklen);
  if (ofs < 0 || toklen <= 0 || !out) return false;
  char buf[32] = {0};
  int copy = toklen < (int) sizeof(buf) - 1 ? toklen : (int) sizeof(buf) - 1;
  memcpy(buf, body.buf + ofs, copy);
  *out = atoi(buf);
  return true;
}

void escape_json_string(char *dest, const char *src, size_t dest_size) {
  size_t j = 0;
  if (!dest || dest_size == 0) return;
  if (!src) { dest[0] = '\0'; return; }
  for (size_t i = 0; src[i] && j < dest_size - 1; i++) {
    if (j >= dest_size - 3) break;
    switch (src[i]) {
      case '"':  dest[j++]='\\'; dest[j++]='"'; break;
      case '\\': dest[j++]='\\'; dest[j++]='\\'; break;
      case '\n': dest[j++]='\\'; dest[j++]='n';  break;
      case '\r': dest[j++]='\\'; dest[j++]='r';  break;
      case '\t': dest[j++]='\\'; dest[j++]='t';  break;
      default:   dest[j++]=src[i];               break;
    }
  }
  dest[j] = '\0';
}

bool path_starts(struct mg_str uri, const char* prefix) {
  size_t n = strlen(prefix);
  return uri.len >= (int) n && memcmp(uri.buf, prefix, n) == 0;
}

bool path_eq(struct mg_str uri, const char* exact) {
  size_t n = strlen(exact);
  return uri.len == (int) n && memcmp(uri.buf, exact, n) == 0;
}
