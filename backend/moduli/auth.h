#ifndef AUTH_H
#define AUTH_H

#include <stdbool.h>

// API
void authRegister(int client_socket, const char *request);
void authLogin(int client_socket, const char *request);

// Dichiariamo la funzione di parsing JSON (già implementata altrove)
bool parse_json_field(const char *json, const char *field, char *out);

#endif // AUTH_H
