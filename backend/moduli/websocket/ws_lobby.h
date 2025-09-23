#pragma once
#include "mongoose.h"
#include <stdbool.h>

// Config base
#define LOBBY_MAX_PLAYERS_DEFAULT  2
#define LOBBY_ID_MAXLEN           32

// Inizializza il sottosistema WS (richiamala nel main)
void ws_lobby_init(struct mg_mgr *mgr);

// Gestisce l'upgrade HTTP -> WS se l'URI combacia (ritorna true se gestito)
bool ws_lobby_handle_http(struct mg_connection *c, struct mg_http_message *hm);

// Invio server-initiato a tutti i client di una lobby
void ws_lobby_notify_all(const char *lobby_id, const char *json);

// Hook DB: implementali altrove (o lascia gli stub nel .c)
void db_on_player_join(const char *lobby_id, const char *player_id, const char *username);
void db_on_player_leave(const char *lobby_id, const char *player_id);
void db_on_lobby_full(const char *lobby_id, int players_count);
