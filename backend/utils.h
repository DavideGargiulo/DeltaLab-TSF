#ifndef UTILS_H
#define UTILS_H

#define ROOM_ID_LEN 6

void parse_json_field(const char *json, const char *field, char *output);

void initialize_rng();

void generate_random_id(char *buffer);

#endif
