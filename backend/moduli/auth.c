#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <unistd.h>
#include "auth.h"
#include "../utils.h"

void authRegister(int client_socket, const char *request) {
  char username[50], password[50];
  parse_json_field(request, "username", username);
  parse_json_field(request, "password", password);

  printf("%s : %s\n", username, password);
  bool response_db = false;
  // Connessione al DB

  if (response_db) {
    response_db = true;
    char *response =
      "HTTP/1.1 200 OK\r\n"
      "Content-Type: application/json\r\n\r\n"
      "{\"status\":\"success\",\"message\":\"Registrazione completata\"}";
    send(client_socket, response, strlen(response), 0);
  } else {
    char *response =
      "HTTP/1.1 500 Internal Server Error\r\n"
      "Content-Type: application/json\r\n\r\n"
      "{\"status\":\"error\",\"message\":\"Errore durante la registrazione\"}";
    send(client_socket, response, strlen(response), 0);
  }
}

void authLogin(int client_socket, const char *request) {
    char username[50], password[50];
    parse_json_field(request, "username", username);
    parse_json_field(request, "password", password);
    // password to md5
    bool response_db = false;
    // connessione al DB

    if (response_db) {
        char *response =
            "HTTP/1.1 200 OK\r\n"
            "Content-Type: application/json\r\n\r\n"
            "{\"status\":\"success\",\"message\":\"Login riuscito\"}";
        send(client_socket, response, strlen(response), 0);
    } else {
        char *response =
            "HTTP/1.1 401 Unauthorized\r\n"
            "Content-Type: application/json\r\n\r\n"
            "{\"status\":\"error\",\"message\":\"Credenziali errate\"}";
        send(client_socket, response, strlen(response), 0);
    }
}
