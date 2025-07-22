#include <stdio.h>
#include <string.h>
#include "utils.h"

// Semplice parser JSON (non robusto, solo per demo)
void parse_json_field(const char *json, const char *field, char *output) {
    char *pos = strstr(json, field);
    if (pos) {
        sscanf(pos + strlen(field) + 3, "%[^\"]", output); // Salta \":\"
    }
}
