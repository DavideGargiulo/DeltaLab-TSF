#pragma once
#include "mongoose.h"
#include <stdbool.h>
#include <stddef.h>

void send_json(struct mg_connection *c, int code, const char *json);
bool handle_cors(struct mg_connection *c, struct mg_http_message *hm);

// JSON helpers
bool json_get_str(struct mg_str body, const char *jp, char *out, size_t n);
bool json_get_int(struct mg_str body, const char *jp, int *out);

// Safe string escape per inserire testo in JSON
void escape_json_string(char *dest, const char *src, size_t dest_size);

// Match helper per path tipo "/lobby/:id"
bool path_starts(struct mg_str uri, const char* prefix);
bool path_eq(struct mg_str uri, const char* exact);
