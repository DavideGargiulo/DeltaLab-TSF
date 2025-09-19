#include "translate.h"
#include "mongoose.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdbool.h>

// Lista delle lingue supportate per validazione
static const char *supported_languages[] = {
    "it", "en", "fr", "de", "es", "pt", "nl", "sv", "no", "da", 
    "fi", "pl", "cs", "sk", "hu", "ro", "bg", "el", "hr", "sl", 
    "lt", "lv", "et", "ja", "ru", "zh", NULL
};

// Verifica se una lingua è supportata
static bool is_language_supported(const char *lang) {
  if (!lang || strlen(lang) != 2) return false;
  
  for (int i = 0; supported_languages[i] != NULL; i++) {
    if (strcmp(supported_languages[i], lang) == 0) {
      return true;
    }
  }
  return false;
}

// Callback che riceve la risposta HTTP
static void translate_cb(struct mg_connection *c, int ev, void *ev_data) {
  if (ev == MG_EV_HTTP_MSG) {
    struct mg_http_message *hm = (struct mg_http_message *) ev_data;
    char **resp = (char **) c->fn_data;
    *resp = strndup(hm->body.buf, hm->body.len);
    c->is_closing = 1;
  } else if (ev == MG_EV_ERROR) {
    char **resp = (char **) c->fn_data;
    fprintf(stderr, "Errore di connessione a LibreTranslate\n");
    *resp = strdup("{\"error\":\"Connection failed\"}");
    c->is_closing = 1;
  }
}

static void escape_json_string(char *dest, const char *src, size_t dest_size) {
  size_t j = 0;
  for (size_t i = 0; src[i] && j < dest_size - 1; i++) {
    if (j >= dest_size - 3) break;
    switch (src[i]) {
      case '"':
        dest[j++] = '\\';
        dest[j++] = '"';
        break;
      case '\\':
        dest[j++] = '\\';
        dest[j++] = '\\';
        break;
      case '\n':
        dest[j++] = '\\';
        dest[j++] = 'n';
        break;
      case '\r':
        dest[j++] = '\\';
        dest[j++] = 'r';
        break;
      case '\t':
        dest[j++] = '\\';
        dest[j++] = 't';
        break;
      default:
        dest[j++] = src[i];
        break;
    }
  }
  dest[j] = '\0';
}

// Test di connettività a LibreTranslate
static bool test_libretranslate_connection(const char *translate_host) {
  struct mg_mgr mgr;
  mg_mgr_init(&mgr);
  
  char url[256];
  snprintf(url, sizeof(url), "%s/languages", translate_host);
  
  char *response = NULL;
  struct mg_connection *c = mg_http_connect(&mgr, url, translate_cb, &response);
  if (!c) {
    fprintf(stderr, "Impossibile connettersi a LibreTranslate su %s\n", url);
    mg_mgr_free(&mgr);
    return false;
  }
  
  c->fn_data = &response;
  mg_printf(c, "GET /languages HTTP/1.1\r\nHost: libretranslate:5000\r\n\r\n");
  
  uint64_t start = mg_millis();
  while (response == NULL && (mg_millis() - start) < 5000) {
    mg_mgr_poll(&mgr, 100);
  }
  
  mg_mgr_free(&mgr);
  
  bool success = (response != NULL);
  if (response) {
    free(response);
  }
  
  return success;
}

char* translate_text(const char *text, const char *source, const char *target) {
  // Validazione input
  if (!text || !source || !target) {
    return strdup("{\"error\":\"Parametri mancanti\"}");
  }
  
  if (!is_language_supported(source) || !is_language_supported(target)) {
    return strdup("{\"error\":\"Lingua non supportata\"}");
  }
  
  if (strlen(text) == 0) {
    return strdup("{\"error\":\"Testo vuoto\"}");
  }
  
  // Se source e target sono uguali, ritorna il testo originale
  if (strcmp(source, target) == 0) {
    return strdup(text);
  }

  const char *translate_host = getenv("TRANSLATE_SERVICE");
  if (!translate_host) {
    translate_host = "http://libretranslate:5000";
  }
  
  // Test di connettività
  if (!test_libretranslate_connection(translate_host)) {
    fprintf(stderr, "LibreTranslate non raggiungibile su %s\n", translate_host);
    return strdup("{\"error\":\"Servizio traduzione non disponibile\"}");
  }

  struct mg_mgr mgr;
  mg_mgr_init(&mgr);

  // Costruisci JSON body
  char escaped_text[1024];
  escape_json_string(escaped_text, text, sizeof(escaped_text));
  
  char body[2048];
  snprintf(body, sizeof(body),
      "{\"q\":\"%s\",\"source\":\"%s\",\"target\":\"%s\",\"format\":\"text\"}",
      escaped_text, source, target);

  char url[256];
  snprintf(url, sizeof(url), "%s/translate", translate_host);

  char *response = NULL;
  struct mg_connection *c = mg_http_connect(&mgr, url, translate_cb, &response);
  if (!c) {
    mg_mgr_free(&mgr);
    return strdup("{\"error\":\"Connessione fallita a LibreTranslate\"}");
  }

  c->fn_data = &response;
  mg_printf(c,
      "POST /translate HTTP/1.1\r\n"
      "Host: libretranslate\r\n"
      "Content-Type: application/json\r\n"
      "Content-Length: %d\r\n\r\n%s",
      (int)strlen(body), body);

  // Event loop con timeout aumentato
  uint64_t start = mg_millis();
  while (response == NULL && (mg_millis() - start) < 15000) { // 15 secondi
    mg_mgr_poll(&mgr, 100);
  }
  mg_mgr_free(&mgr);

  if (!response) {
    return strdup("{\"error\":\"Timeout nella traduzione\"}");
  }

  // Controlla se è un errore
  if (strstr(response, "\"error\"")) {
    return response; // Ritorna l'errore così com'è
  }

  // Parsing JSON per estrarre translatedText
  int toklen = 0;
  int ofs = mg_json_get(mg_str(response), "$.translatedText", &toklen);
  if (ofs < 0 || toklen <= 0) {
    free(response);
    return strdup("{\"error\":\"Formato risposta non valido\"}");
  }

  const char *ptr = response + ofs;
  if (toklen >= 2 && ptr[0] == '"' && ptr[toklen - 1] == '"') {
    ptr++;
    toklen -= 2;
  }

  char *translated = strndup(ptr, toklen);
  free(response);

  return translated;
}