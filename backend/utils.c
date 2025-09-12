#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "utils.h"

static const char CHARSET[] = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
static const int CHARSET_SIZE = sizeof(CHARSET) - 1;

// Semplice parser JSON (non robusto, solo per demo)
void parse_json_field(const char *json, const char *field, char *output) {
    char *pos = strstr(json, field);
    if (pos) {
        sscanf(pos + strlen(field) + 3, "%[^\"]", output); // Salta \":\"
    }
}

void initializeRng() {
  srand(time(NULL));
}

void generateRandomId(char *buffer) {
  for (size_t i = 0; i < ROOM_ID_LEN; ++i) {
    buffer[i] = CHARSET[rand() % CHARSET_SIZE];
  }
  buffer[ROOM_ID_LEN] = '\0';
}