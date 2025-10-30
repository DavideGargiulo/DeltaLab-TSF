#include "ws_lobby.h"
#include "mongoose.h"
#include "../lobby.h"
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <stdio.h>

// ======= Tipi interni =======

typedef enum {
  GAME_STATE_LOBBY,
  GAME_STATE_STARTING,
  GAME_STATE_PLAYING,
  GAME_STATE_ENDED
} GameState;

struct ws_client {
  struct mg_connection *c;
  char player_id[32];
  char username[64];
  bool has_played;
  struct ws_client *next;
};

struct lobby_room {
  char id[LOBBY_ID_MAXLEN];
  int max_players;
  int players_count;
  char creator_id[32];
  char current_player_id[32];
  char rotation[16];
  char last_message[512];      // ✅ Ultimo messaggio della catena
  char full_story[4096];        // ✅ NUOVO: Storia completa del gioco
  bool game_active;             // ✅ NUOVO: Flag per sapere se il gioco è attivo
  int spectators_count;
  struct ws_client *spectators;
  struct ws_client *clients;
  struct lobby_room *next;
};

struct conn_state {
  char lobby_id[LOBBY_ID_MAXLEN];
  char player_id[32];
  char username[64];
};

static struct {
  struct mg_mgr *mgr;
  struct lobby_room *rooms;
} s_ws;

typedef struct {
  bool success;
  bool lobby_deleted;
  char *error_msg;
} LeaveResult;

static LeaveResult db_on_player_leave(const char *lobby_id, int player_id);

// ======= Utility =======

static const char* find_substr(const char* hay, size_t hlen, const char* needle) {
  size_t nlen = strlen(needle);
  if (nlen == 0 || hlen < nlen) return NULL;
  for (size_t i = 0; i + nlen <= hlen; ++i) {
    if (memcmp(hay + i, needle, nlen) == 0) return hay + i;
  }
  return NULL;
}

static struct lobby_room *find_room(const char *lobby_id) {
  for (struct lobby_room *r = s_ws.rooms; r; r = r->next)
    if (strncmp(r->id, lobby_id, LOBBY_ID_MAXLEN) == 0) return r;
  return NULL;
}

static struct lobby_room *get_or_create_room(const char *lobby_id) {
  struct lobby_room *r = find_room(lobby_id);
  if (r) return r;
  r = (struct lobby_room *) calloc(1, sizeof(*r));
  snprintf(r->id, sizeof(r->id), "%s", lobby_id);
  r->max_players = LOBBY_MAX_PLAYERS_DEFAULT;
  r->players_count = 0;
  r->clients = NULL;
  r->creator_id[0] = 0;
  r->current_player_id[0] = 0;
  r->last_message[0] = 0;       // ✅ Inizializza last_message
  r->full_story[0] = 0;         // ✅ Inizializza full_story
  r->game_active = false;       // ✅ Inizializza game_active
  r->spectators = NULL;
  r->spectators_count = 0;
  snprintf(r->rotation, sizeof(r->rotation), "orario");
  r->next = s_ws.rooms;
  s_ws.rooms = r;
  return r;
}

static void delete_room_if_empty(struct lobby_room *r) {
  if (!r) return;
  if (r->players_count > 0 || r->clients || r->spectators || r->spectators_count > 0) return;
  struct lobby_room **pp = &s_ws.rooms;
  while (*pp) {
    if (*pp == r) { *pp = r->next; free(r); break; }
    pp = &(*pp)->next;
  }
}


static void remove_client(struct lobby_room *r, struct mg_connection *c, const char *player_id) {
  struct ws_client **pp = &r->clients;
  while (*pp) {
    if ((*pp)->c == c) {
      struct ws_client *dead = *pp;
      *pp = (*pp)->next;
      if (r->players_count > 0) r->players_count--;
      free(dead);
      break;
    }
    pp = &(*pp)->next;
  }
}

static void room_broadcast(struct lobby_room *r, const char *json) {
  for (struct ws_client *cl = r->clients; cl; cl = cl->next) {
    if (cl->c && !cl->c->is_closing) {
      mg_ws_send(cl->c, json, strlen(json), WEBSOCKET_OP_TEXT);
    }
  }
  for (struct ws_client *sp = r->spectators; sp; sp = sp->next) {
    if (sp->c && !sp->c->is_closing) {
      mg_ws_send(sp->c, json, strlen(json), WEBSOCKET_OP_TEXT);
    }
  }
}

void ws_lobby_notify_all(const char *lobby_id, const char *json) {
  struct lobby_room *r = find_room(lobby_id);
  if (!r) return;
  room_broadcast(r, json);
}

static bool is_creator(struct lobby_room *r, const char *player_id) {
  return r && r->creator_id[0] && strncmp(r->creator_id, player_id, sizeof(r->creator_id)) == 0;
}

static struct ws_client* find_client_by_id(struct lobby_room *r, const char *player_id) {
  for (struct ws_client *cl = r->clients; cl; cl = cl->next) {
    if (strncmp(cl->player_id, player_id, sizeof(cl->player_id)) == 0) {
      return cl;
    }
  }
  return NULL;
}

static void add_spectator(struct lobby_room *r, struct mg_connection *c, const char *player_id, const char *username) {
  struct ws_client *cl = (struct ws_client *) calloc(1, sizeof(*cl));
  if (!cl) return;
  cl->c = c;
  snprintf(cl->player_id, sizeof(cl->player_id), "%s", player_id ? player_id : "");
  snprintf(cl->username, sizeof(cl->username), "%s", username ? username : "");
  cl->next = NULL;

  if (!r->spectators) r->spectators = cl;
  else {
    struct ws_client *tail = r->spectators;
    while (tail->next) tail = tail->next;
    tail->next = cl;
  }
  r->spectators_count++;
}

static void remove_spectator(struct lobby_room *r, struct mg_connection *c, const char *player_id) {
  struct ws_client **pp = &r->spectators;
  while (*pp) {
    if ((*pp)->c == c || (player_id && strncmp((*pp)->player_id, player_id, sizeof((*pp)->player_id)) == 0)) {
      struct ws_client *dead = *pp;
      *pp = (*pp)->next;
      if (r->spectators_count > 0) r->spectators_count--;
      free(dead);
      break;
    }
    pp = &(*pp)->next;
  }
}

static void promote_spectators_if_slots(struct lobby_room *r) {
  if (!r) return;
  while (r->spectators && r->players_count < r->max_players) {
    struct ws_client *sp = r->spectators;
    r->spectators = sp->next;
    r->spectators_count--;

    sp->has_played = false;
    sp->next = NULL;

    if (!r->clients) r->clients = sp;
    else {
      struct ws_client *tail = r->clients;
      while (tail->next) tail = tail->next;
      tail->next = sp;
    }
    r->players_count++;

    char bcast[256];
    snprintf(bcast, sizeof(bcast),
             "{\"type\":\"spectator_promoted\",\"playerId\":\"%s\",\"username\":\"%s\",\"players\":%d,\"spectators\":%d}",
             sp->player_id, sp->username, r->players_count, r->spectators_count);
    room_broadcast(r, bcast);
  }
}


// ======= JSON naive (buf/len) =======

static bool json_has(const struct mg_ws_message *wm, const char *key, const char *val) {
  char needle[128];
  snprintf(needle, sizeof(needle), "\"%s\":\"%s\"", key, val);
  return find_substr((const char*) wm->data.buf, wm->data.len, needle) != NULL;
}

static bool json_get_str(const struct mg_ws_message *wm, const char *key, char *out, size_t maxlen) {
  const char *p = (const char *) wm->data.buf;
  size_t n = wm->data.len;
  char kbuf[64];
  snprintf(kbuf, sizeof(kbuf), "\"%s\":\"", key);
  const char *start = find_substr(p, n, kbuf);
  if (!start) return false;
  start += strlen(kbuf);
  const char *end = memchr(start, '"', (p + n) - start);
  if (!end) return false;
  size_t len = (size_t)(end - start);
  if (len >= maxlen) len = maxlen - 1;
  memcpy(out, start, len);
  out[len] = 0;
  return true;
}

// ======= Lifecycle =======

static void on_ws_open(struct mg_connection *c, const char *lobby_id) {
  struct lobby_room *r = get_or_create_room(lobby_id);

  struct conn_state *st = (struct conn_state *) c->fn_data;
  if (!st) {
    st = (struct conn_state *) calloc(1, sizeof(*st));
    c->fn_data = st;
  }
  snprintf(st->lobby_id, sizeof(st->lobby_id), "%s", lobby_id);

  char msg[256];
  snprintf(msg, sizeof(msg),
           "{\"type\":\"welcome\",\"lobbyId\":\"%s\",\"players\":%d,\"maxPlayers\":%d}",
           r->id, r->players_count, r->max_players);
  mg_ws_send(c, msg, strlen(msg), WEBSOCKET_OP_TEXT);
}

static void on_ws_close(struct mg_connection *c) {
  struct conn_state *st = (struct conn_state *) c->fn_data;
  if (!st || !*st->lobby_id) { free(st); return; }
  struct lobby_room *r = find_room(st->lobby_id);
  if (!r) { free(st); return; }

  remove_client(r, c, st->player_id);
  remove_spectator(r, c, st->player_id);

  char msg[256];
  snprintf(msg, sizeof(msg),
           "{\"type\":\"player_left\",\"playerId\":\"%s\",\"players\":%d}",
           *st->player_id ? st->player_id : "", r->players_count);
  room_broadcast(r, msg);

  if (is_creator(r, st->player_id)) {
    r->creator_id[0] = 0;
  }

  delete_room_if_empty(r);
  free(st);
}

// ======= Actions =======

static void handle_action_join_spectator(struct mg_connection *c, struct mg_ws_message *wm, struct conn_state *st) {
  char player_id[32] = {0}, username[64] = {0}, lobby_id[LOBBY_ID_MAXLEN] = {0};

  snprintf(lobby_id, sizeof(lobby_id), "%s", st->lobby_id);
  json_get_str(wm, "lobbyId", lobby_id, sizeof(lobby_id));
  json_get_str(wm, "playerId", player_id, sizeof(player_id));
  json_get_str(wm, "username", username, sizeof(username));

  // DB: usa lo stesso hook del join giocatore oppure salta DB se non serve registrare spettatori
  // Qui NON tocchiamo DB_*, seguiamo la tua logica esistente.
  struct lobby_room *r = get_or_create_room(lobby_id);

  // se già presente come player, non duplicare
  if (find_client_by_id(r, player_id)) {
    const char *already = "{\"type\":\"join_spectator_error\",\"message\":\"Sei già nella lobby\"}";
    mg_ws_send(c, already, strlen(already), WEBSOCKET_OP_TEXT);
    return;
  }

  // se già tra gli spettatori con stessa connessione o id, non duplicare
  for (struct ws_client *it = r->spectators; it; it = it->next) {
    if (it->c == c || strncmp(it->player_id, player_id, sizeof(it->player_id)) == 0) {
      const char *already = "{\"type\":\"join_spectator_success\",\"role\":\"spectator\"}";
      mg_ws_send(c, already, strlen(already), WEBSOCKET_OP_TEXT);
      return;
    }
  }

  add_spectator(r, c, player_id, username);

  // aggiorna stato connessione
  snprintf(st->lobby_id, sizeof(st->lobby_id), "%s", lobby_id);
  snprintf(st->player_id, sizeof(st->player_id), "%s", player_id);
  snprintf(st->username, sizeof(st->username), "%s", username);

  // conferma e broadcast
  char ok[256];
  snprintf(ok, sizeof(ok),
           "{\"type\":\"join_spectator_success\",\"playerId\":\"%s\",\"username\":\"%s\",\"lobbyId\":\"%s\",\"spectators\":%d}",
           player_id, username, lobby_id, r->spectators_count);
  mg_ws_send(c, ok, strlen(ok), WEBSOCKET_OP_TEXT);

  char bcast[256];
  snprintf(bcast, sizeof(bcast),
           "{\"type\":\"spectator_joined\",\"playerId\":\"%s\",\"username\":\"%s\",\"spectators\":%d}",
           player_id, username, r->spectators_count);
  room_broadcast(r, bcast);

  char *error_msg = NULL;
  db_on_player_join(lobby_id, atoi(player_id), &error_msg);
  if (error_msg) {
    free(error_msg);
  }
}


static void handle_action_join(struct mg_connection *c, struct mg_ws_message *wm, struct conn_state *st) {
  char player_id[32] = {0}, username[64] = {0}, lobby_id[LOBBY_ID_MAXLEN] = {0};

  snprintf(lobby_id, sizeof(lobby_id), "%s", st->lobby_id);
  json_get_str(wm, "lobbyId", lobby_id, sizeof(lobby_id));

  struct lobby_room *r = get_or_create_room(lobby_id);
  bool lobby_is_full = (r->players_count >= r->max_players);
  bool game_in_progress = r->game_active;

  if (lobby_is_full || game_in_progress) {
    handle_action_join_spectator(c, wm, st);
    return;
  }

  json_get_str(wm, "playerId", player_id, sizeof(player_id));
  json_get_str(wm, "username", username, sizeof(username));


  int player_id_int = atoi(player_id);

  char *error_msg = NULL;
  if (!db_on_player_join(lobby_id, player_id_int, &error_msg)) {
    char error_response[512];
    if (error_msg) {
      snprintf(error_response, sizeof(error_response),
              "{\"type\":\"join_error\",\"message\":%s}", error_msg);
      free(error_msg);
    } else {
      snprintf(error_response, sizeof(error_response),
              "{\"type\":\"error\",\"message\":\"Errore join\"}");
    }
    mg_ws_send(c, error_response, strlen(error_response), WEBSOCKET_OP_TEXT);
    return;
  }

  if (strncmp(st->lobby_id, lobby_id, sizeof(st->lobby_id)) != 0) {
    snprintf(st->lobby_id, sizeof(st->lobby_id), "%s", lobby_id);
  }


  struct ws_client *exists = NULL;
  for (struct ws_client *it = r->clients; it; it = it->next) {
    if (it->c == c) {
      exists = it;
      break;
    }
  }

  if (!exists) {
    struct ws_client *cl = (struct ws_client *) calloc(1, sizeof(*cl));
    if (!cl) {
      mg_ws_send(c, "{\"type\":\"error\",\"message\":\"Errore memoria server\"}", 50, WEBSOCKET_OP_TEXT);
      return;
    }
    cl->c = c;
    snprintf(cl->player_id, sizeof(cl->player_id), "%s", player_id);
    snprintf(cl->username, sizeof(cl->username), "%s", username);
    cl->next = NULL;

    if (!r->clients) {
      r->clients = cl;
    } else {
      struct ws_client *tail = r->clients;
      while (tail->next) tail = tail->next;
      tail->next = cl;
    }
    r->players_count++;

    // se non c'è ancora un creatore, il primo join diventa creator
    if (!r->creator_id[0]) {
      snprintf(r->creator_id, sizeof(r->creator_id), "%s", player_id);
    }
  } else {
    snprintf(exists->player_id, sizeof(exists->player_id), "%s", player_id);
    snprintf(exists->username, sizeof(exists->username), "%s", username);
  }

  snprintf(st->player_id, sizeof(st->player_id), "%s", player_id);
  snprintf(st->username, sizeof(st->username), "%s", username);
  char success_msg[256];
  snprintf(success_msg, sizeof(success_msg),
      "{\"type\":\"join_success\",\"playerId\":\"%s\",\"username\":\"%s\",\"lobbyId\":\"%s\"}",
      player_id, username, lobby_id);
  mg_ws_send(c, success_msg, strlen(success_msg), WEBSOCKET_OP_TEXT);
  char broadcast_msg[256];
  snprintf(broadcast_msg, sizeof(broadcast_msg),
      "{\"type\":\"player_joined\",\"playerId\":\"%s\",\"username\":\"%s\",\"players\":%d}",
      player_id, username, r->players_count);
  for (struct ws_client *cl = r->clients; cl; cl = cl->next) {
    if (cl->c != c) {
      mg_ws_send(cl->c, broadcast_msg, strlen(broadcast_msg), WEBSOCKET_OP_TEXT);
    }
  }
  if (r->players_count >= r->max_players) {
    room_broadcast(r, "{\"type\":\"lobby_full\"}");
  }
}

// ======= Turni =======
static void get_next_player_id(struct lobby_room *r, const char *current_id,
                               char *out, size_t outlen) {
  if (!r || !r->clients || !out || outlen == 0) { if (out) out[0] = 0; return; }

  struct ws_client *cur = find_client_by_id(r, current_id ? current_id : "");

  // Se non trovo il corrente, prendo semplicemente la testa
  struct ws_client *next = NULL;
  if (!cur) {
    next = r->clients;
  } else {
    next = cur->next ? cur->next : r->clients;  // ciclo
  }

  snprintf(out, outlen, "%s", next ? next->player_id : "");
}

static void advance_turn_and_broadcast(struct lobby_room *r) {
  if (!r || !r->clients) {
    if (r && r->creator_id[0] && r->game_active) {
      int creator_id = atoi(r->creator_id);
      db_on_game_end(r->id, creator_id);
      r->current_player_id[0] = 0;
      r->game_active = false;
      promote_spectators_if_slots(r);
      room_broadcast(r, "{\"type\":\"game_ended\",\"message\":\"La partita è terminata automaticamente (nessun giocatore)\"}");
    }
    return;
  }

  // ✅ FIX: Controlla solo i giocatori ancora connessi
  bool all_played = true;
  int connected_count = 0;
  for (struct ws_client *cl = r->clients; cl; cl = cl->next) {
    connected_count++;
    if (!cl->has_played) {
      all_played = false;
      break;
    }
  }

  // ✅ Se tutti i giocatori connessi hanno giocato, termina la partita
  if (all_played && connected_count > 0) {
    if (r->creator_id[0]) {
      int creator_id = atoi(r->creator_id);
      db_on_game_end(r->id, creator_id);
      promote_spectators_if_slots(r);
    }
    r->current_player_id[0] = 0;
    r->game_active = false;

    // Reset has_played per permettere una nuova partita
    for (struct ws_client *cl = r->clients; cl; cl = cl->next) {
      cl->has_played = false;
    }

    char msg[4600];
    snprintf(msg, sizeof(msg),
             "{\"type\":\"game_ended\",\"message\":\"La partita è terminata: tutti hanno giocato!\",\"text\":\"%s\"}",
             r->full_story);  // ✅ USA la storia completa
    room_broadcast(r, msg);
    return;
  }

  // ✅ Trova il prossimo giocatore che non ha ancora giocato
  char next_id[32] = {0};
  struct ws_client *start = find_client_by_id(r, r->current_player_id);
  struct ws_client *iter = start ? start->next : r->clients;
  if (!iter) iter = r->clients;

  // Cicla fino a trovare un giocatore che non ha giocato
  int attempts = 0;
  while (iter && attempts < 10) {  // Max 10 tentativi per evitare loop infiniti
    if (!iter->has_played) {
      snprintf(next_id, sizeof(next_id), "%s", iter->player_id);
      break;
    }
    iter = iter->next ? iter->next : r->clients;
    attempts++;

    // Evita loop infinito se siamo tornati al punto di partenza
    if (iter == start) break;
  }

  if (!next_id[0]) {
    // Nessun prossimo giocatore trovato, termina
    if (r->creator_id[0]) {
      int creator_id = atoi(r->creator_id);
      db_on_game_end(r->id, creator_id);
    }
    r->current_player_id[0] = 0;
    r->game_active = false;
    promote_spectators_if_slots(r);
    char msg[4600];
    snprintf(msg, sizeof(msg),
             "{\"type\":\"game_ended\",\"message\":\"La partita è terminata: nessun prossimo giocatore!\",\"text\":\"%s\"}",
             r->full_story);
    room_broadcast(r, msg);
    return;
  }

  snprintf(r->current_player_id, sizeof(r->current_player_id), "%s", next_id);

  const struct ws_client *next_cl = find_client_by_id(r, next_id);
  const char *next_username = next_cl ? next_cl->username : "";

  char msg[4600];
  snprintf(msg, sizeof(msg),
           "{\"type\":\"turn_changed\",\"currentPlayerId\":\"%s\",\"username\":\"%s\",\"lastMessage\":\"%s\"}",
           next_id, next_username, r->full_story);  // ✅ Invia la storia completa
  room_broadcast(r, msg);
}

static void handle_action_leave(struct mg_connection *c, struct conn_state *st) {
  if (!*st->lobby_id) return;

  struct lobby_room *r = find_room(st->lobby_id);
  if (!r) return;

  int player_id_int = atoi(st->player_id);
  LeaveResult result = db_on_player_leave(st->lobby_id, player_id_int);

  if (!result.success) {
    char error_response[512];
    if (result.error_msg) {
      snprintf(error_response, sizeof(error_response),
              "{\"type\":\"leave_error\",\"message\":%s}", result.error_msg);
      free(result.error_msg);
    } else {
      snprintf(error_response, sizeof(error_response),
              "{\"type\":\"error\",\"message\":\"Errore leave\"}");
    }
    mg_ws_send(c, error_response, strlen(error_response), WEBSOCKET_OP_TEXT);
    return;
  }

  if (result.lobby_deleted) {
    room_broadcast(r, "{\"type\":\"lobby_deleted\",\"message\":\"Il creatore ha chiuso la lobby\"}");

    struct ws_client *cl = r->clients;
    while (cl) {
      struct ws_client *next = cl->next;
      if (cl->c && !cl->c->is_closing) {
        cl->c->is_closing = 1;
      }
      free(cl);
      cl = next;
    }

    struct lobby_room **pp = &s_ws.rooms;
    while (*pp) {
      if (*pp == r) {
        *pp = r->next;
        free(r);
        break;
      }
      pp = &(*pp)->next;
    }
  } else {
    // ✅ FIX: Controlla se era il turno del giocatore che sta uscendo
    bool was_current_turn = (r->current_player_id[0] &&
                            strncmp(r->current_player_id, st->player_id, sizeof(r->current_player_id)) == 0);

    bool was_player = (find_client_by_id(r, st->player_id) != NULL);
    // ✅ IMPORTANTE: Rimuovi il client PRIMA di calcolare il prossimo
    remove_client(r, c, st->player_id);
    if (!was_player) {
      remove_spectator(r, c, st->player_id);
    }

    // Broadcast che il giocatore è uscito
    char msg[5000];
    snprintf(msg, sizeof(msg),
            "{\"type\":\"player_left\",\"playerId\":\"%s\",\"players\":%d,\"spectators\":%d,\"lastMessage\":\"%s\"}",
            st->player_id, r->players_count, r->spectators_count, r->full_story);  // ✅ USA full_story
    room_broadcast(r, msg);

    // ✅ Se era il suo turno E il gioco è attivo, passa al prossimo
    if (was_current_turn && r->game_active) {
      // Il turno passa automaticamente al prossimo giocatore
      advance_turn_and_broadcast(r);
    }

    if (is_creator(r, st->player_id)) {
      r->creator_id[0] = 0;
    }

    delete_room_if_empty(r);
  }

  st->lobby_id[0] = 0;
  st->player_id[0] = 0;
  st->username[0] = 0;
}

static void handle_action_chat(struct mg_connection *c, struct mg_ws_message *wm, struct conn_state *st) {
  if (!*st->lobby_id) return;
  struct lobby_room *r = find_room(st->lobby_id);
  if (!r) return;

  // Verifica che sia il turno del giocatore
  if (!r->current_player_id[0] ||
      strncmp(r->current_player_id, st->player_id, sizeof(r->current_player_id)) != 0) {
    const char *err = "{\"type\":\"error\",\"message\":\"Non è il tuo turno di scrivere\"}";
    mg_ws_send(c, err, strlen(err), WEBSOCKET_OP_TEXT);
    return;
  }

  struct ws_client *writer = NULL;
  for (struct ws_client *cl = r->clients; cl; cl = cl->next) {
    if (cl->c == c) {
      writer = cl;
      break;
    }
  }

  if (!writer) return;

  if (writer->has_played) {
    const char *err = "{\"type\":\"error\",\"message\":\"Hai già scritto la tua parola\"}";
    mg_ws_send(c, err, strlen(err), WEBSOCKET_OP_TEXT);
    return;
  }

  // ✅ Leggi il messaggio
  char text[512] = {0};
  json_get_str(wm, "text", text, sizeof(text));

  // ✅ Aggiorna last_message
  snprintf(r->last_message, sizeof(r->last_message), "%s", text);

  // ✅ NUOVO: Aggiungi alla storia completa
  if (r->full_story[0]) {
    // Se c'è già testo, aggiungi uno spazio
    size_t current_len = strlen(r->full_story);
    if (current_len + 1 + strlen(text) < sizeof(r->full_story)) {
      snprintf(r->full_story + current_len, sizeof(r->full_story) - current_len, " %s", text);
    }
  } else {
    // Primo messaggio
    snprintf(r->full_story, sizeof(r->full_story), "%s", text);
  }

  // Marca che questo giocatore ha scritto
  writer->has_played = true;

  // ✅ Calcola il prossimo giocatore PRIMA di cambiare il turno
  char next_id[32] = {0};
  get_next_player_id(r, r->current_player_id, next_id, sizeof(next_id));

  // Broadcast del messaggio di chat
  char msg[900];
  int n = snprintf(msg, sizeof(msg),
           "{\"type\":\"chat\",\"playerId\":\"%s\",\"username\":\"%s\",\"text\":\"%s\",\"nextPlayerId\":\"%s\"}",
           st->player_id, st->username, text, next_id);
  if (n < 0) return;
  if ((size_t)n >= sizeof(msg)) msg[sizeof(msg)-1] = '\0';

  room_broadcast(r, msg);

  // ✅ Avanza il turno DOPO aver inviato il messaggio di chat
  advance_turn_and_broadcast(r);
}

static void handle_action_state(struct mg_connection *c, struct conn_state *st) {
  if (!*st->lobby_id) return;
  struct lobby_room *r = find_room(st->lobby_id);
  if (!r) return;

  char buf[2048];
  size_t off = 0;
  off += snprintf(buf + off, sizeof(buf) - off,
                  "{\"type\":\"state\",\"lobbyId\":\"%s\",\"players\":%d,\"maxPlayers\":%d,\"list\":[",
                  r->id, r->players_count, r->max_players);

  for (struct ws_client *cl = r->clients; cl; cl = cl->next) {
    off += snprintf(buf + off, sizeof(buf) - off,
                    "{\"playerId\":\"%s\",\"username\":\"%s\"}%s",
                    cl->player_id, cl->username, cl->next ? "," : "");
    if (off >= sizeof(buf)) break;
  }
  off += snprintf(buf + off, sizeof(buf) - off, "],\"spectators\":%d,\"spectatorsList\":[", r->spectators_count);
  for (struct ws_client *sp = r->spectators; sp; sp = sp->next) {
    off += snprintf(buf + off, sizeof(buf) - off,
                    "{\"playerId\":\"%s\",\"username\":\"%s\"}%s",
                    sp->player_id, sp->username, sp->next ? "," : "");
    if (off >= sizeof(buf)) break;
  }
  off += snprintf(buf + off, sizeof(buf) - off, "]}");
  mg_ws_send(c, buf, off, WEBSOCKET_OP_TEXT);
}

static void handle_action_setmax(struct mg_connection *c, struct mg_ws_message *wm, struct conn_state *st) {
  if (!*st->lobby_id) return;
  struct lobby_room *r = find_room(st->lobby_id);
  if (!r) return;

  // naive parse "max":<int>
  const char *p = (const char *) wm->data.buf;
  size_t n = wm->data.len;
  const char *needle = "\"max\":";
  const char *at = find_substr(p, n, needle);
  if (!at) return;
  int max = r->max_players;
  sscanf(at + (int)strlen(needle), "%d", &max);
  if (max < 1) max = 1;
  r->max_players = max;

  char msg[128];
  snprintf(msg, sizeof(msg), "{\"type\":\"max_players\",\"value\":%d}", r->max_players);
  room_broadcast(r, msg);

  if (r->players_count >= r->max_players) {
    db_on_lobby_full(r->id, r->players_count);
    room_broadcast(r, "{\"type\":\"lobby_full\"}");
  }
}

static void handle_action_start(struct mg_connection *c, struct conn_state *st) {
  if (!*st->lobby_id) return;
  struct lobby_room *r = find_room(st->lobby_id);
  if (!r) return;

  if (!is_creator(r, st->player_id)) {
    const char *err = "{\"type\":\"error\",\"message\":\"Solo il creatore può avviare la partita\"}";
    mg_ws_send(c, err, strlen(err), WEBSOCKET_OP_TEXT);
    return;
  }

  if (r->players_count < 4) {
    char err[128];
    snprintf(err, sizeof(err),
             "{\"type\":\"error\",\"message\":\"Servono almeno 4 giocatori per iniziare\"}");
    mg_ws_send(c, err, strlen(err), WEBSOCKET_OP_TEXT);
    return;
  }

  int creator_id = atoi(st->player_id);
  if (!db_on_game_start(st->lobby_id, creator_id)) {
    const char *err = "{\"type\":\"error\",\"message\":\"Errore nell'avvio della partita\"}";
    mg_ws_send(c, err, strlen(err), WEBSOCKET_OP_TEXT);
    return;
  }

  // ✅ Reset dello stato del gioco
  for (struct ws_client *cl = r->clients; cl; cl = cl->next) {
    cl->has_played = false;
  }
  r->last_message[0] = 0;
  r->full_story[0] = 0;
  r->game_active = true;  // ✅ Attiva il gioco

  snprintf(r->current_player_id, sizeof(r->current_player_id), "%s", st->player_id);

  char msg[256];
  snprintf(msg, sizeof(msg),
           "{\"type\":\"game_started\",\"currentPlayerId\":\"%s\",\"username\":\"%s\"}",
           r->current_player_id, st->username);
  room_broadcast(r, msg);
}

static void handle_action_endgame(struct mg_connection *c, struct conn_state *st) {
  if (!*st->lobby_id) return;
  struct lobby_room *r = find_room(st->lobby_id);
  if (!r) return;

  if (!is_creator(r, st->player_id)) {
    const char *err = "{\"type\":\"error\",\"message\":\"Solo il creatore può terminare la partita\"}";
    mg_ws_send(c, err, strlen(err), WEBSOCKET_OP_TEXT);
    return;
  }

  int creator_id = atoi(st->player_id);
  if (!db_on_game_end(st->lobby_id, creator_id)) {
    const char *err = "{\"type\":\"error\",\"message\":\"Errore nella terminazione della partita\"}";
    mg_ws_send(c, err, strlen(err), WEBSOCKET_OP_TEXT);
    return;
  }

  r->current_player_id[0] = 0;
  r->game_active = false;  // ✅ Disattiva il gioco

  for (struct ws_client *cl = r->clients; cl; cl = cl->next) {
    cl->has_played = false;
  }

  char msg[4600];
  promote_spectators_if_slots(r);
  snprintf(msg, sizeof(msg),
           "{\"type\":\"game_ended\",\"message\":\"La partita è terminata\",\"text\":\"%s\"}",
           r->full_story);  // ✅ USA full_story
  room_broadcast(r, msg);
}

// ======= Handler WS =======

static void ws_handler(struct mg_connection *c, int ev, void *ev_data) {
  switch (ev) {
    case MG_EV_WS_OPEN: {
      struct conn_state *st = (struct conn_state *) c->fn_data;
      const char *lobby_id = (st && *st->lobby_id) ? st->lobby_id : "default";
      on_ws_open(c, lobby_id);
      break;
    }
    case MG_EV_WS_MSG: {
      struct mg_ws_message *wm = (struct mg_ws_message *) ev_data;
      struct conn_state *st = (struct conn_state *) c->fn_data;

      // difesa: payload troppo grande
      if (wm->data.len > 4096) {
        const char *err = "{\"type\":\"error\",\"message\":\"payload too large\"}";
        mg_ws_send(c, err, strlen(err), WEBSOCKET_OP_TEXT);
        break;
      }

      if (json_has(wm, "action", "join"))        handle_action_join(c, wm, st);
      else if (json_has(wm, "action", "leave"))  handle_action_leave(c, st);
      else if (json_has(wm, "action", "chat"))   handle_action_chat(c, wm, st);
      else if (json_has(wm, "action", "state"))  handle_action_state(c, st);
      else if (json_has(wm, "action", "setmax")) handle_action_setmax(c, wm, st);
      else if (json_has(wm, "action", "start")) handle_action_start(c, st);
      else if (json_has(wm, "action", "end")) handle_action_endgame(c, st);
      else if (json_has(wm, "action", "join_spectator")) handle_action_join_spectator(c, wm, st);
      else {
        const char *err = "{\"type\":\"error\",\"message\":\"Unknown action\"}";
        mg_ws_send(c, err, strlen(err), WEBSOCKET_OP_TEXT);
      }
      break;
    }
    case MG_EV_CLOSE: {
      if (c->is_websocket) on_ws_close(c);
      break;
    }
  }
}

// ======= HTTP upgrader =======

static bool parse_lobby_id(const struct mg_http_message *hm, char *out, size_t maxlen) {
  const char *uri = hm->uri.buf;
  size_t ulen = hm->uri.len;

  // Path: /ws/lobby/<id>
  const char *marker = "/ws/lobby/";
  const char *slash = find_substr(uri, ulen, marker);
  if (slash) {
    const char *id = slash + strlen(marker);
    const char *end = id;
    const char *uend = uri + ulen;
    while (end < uend && *end != '?' && *end != '/') end++;
    size_t len = (size_t)(end - id);
    if (len >= maxlen) len = maxlen - 1;
    memcpy(out, id, len);
    out[len] = 0;
    if (len > 0) return true;
  }

  // Query: ?id=...
  char idbuf[64] = {0};
  if (mg_http_get_var(&hm->query, "id", idbuf, sizeof(idbuf)) > 0) {
    snprintf(out, maxlen, "%s", idbuf);
    return true;
  }
  return false;
}

bool ws_lobby_handle_http(struct mg_connection *c, struct mg_http_message *hm) {
  const char *uri = hm->uri.buf;
  size_t ulen = hm->uri.len;
  const char *prefix = "/ws/lobby";

  bool match = (ulen >= strlen(prefix) && memcmp(uri, prefix, strlen(prefix)) == 0);
  if (!match) return false;

  char lobby_id[LOBBY_ID_MAXLEN] = {0};
  if (!parse_lobby_id(hm, lobby_id, sizeof(lobby_id))) snprintf(lobby_id, sizeof(lobby_id), "default");

  // Prepara stato PRIMA dell'upgrade
  struct conn_state *st = (struct conn_state *) calloc(1, sizeof(*st));
  snprintf(st->lobby_id, sizeof(st->lobby_id), "%s", lobby_id);
  c->fn_data = st;

  c->fn = ws_handler;           // <-- QUI il fix
  mg_ws_upgrade(c, hm, NULL);

  return true;
}

// ======= API =======

void ws_lobby_init(struct mg_mgr *mgr) {
  s_ws.mgr = mgr;
  s_ws.rooms = NULL;
}

// ======= Stub DB =======

bool db_on_player_join(const char *lobby_id, int player_id, char **error_msg) {
  if (!lobby_id || player_id <= 0) {
    if (error_msg) *error_msg = strdup("Parametri non validi");
    return false;
  }

  char *db_result = joinLobby(lobby_id, player_id);
  if (!db_result) {
    if (error_msg) *error_msg = strdup("Errore interno server");
    return false;
  }

  // Verifica se il risultato indica un errore
  if (strstr(db_result, "\"result\":false") != NULL) {
    if (error_msg) {
      *error_msg = db_result;
    } else {
      free(db_result);
    }
    return false;
  }

  free(db_result);
  return true;
}

static LeaveResult db_on_player_leave(const char *lobby_id, int player_id) {
  LeaveResult result = {false, false, NULL};
  if (!lobby_id || player_id <= 0) {
    result.error_msg = strdup("Parametri non validi");
    return result;
  }

  char *db_result = leaveLobby(lobby_id, player_id);
  if (!db_result) {
    result.error_msg = strdup("Errore interno server");
    return result;
  }

  // Verifica se il risultato indica un errore
  if (strstr(db_result, "\"result\":false") != NULL) {
    result.error_msg = db_result;
    return result;
  }

  // Controlla se la lobby è stata eliminata
  result.lobby_deleted = (strstr(db_result, "\"lobbyDeleted\":true") != NULL);
  result.success = true;
  free(db_result);
  return result;
}

bool db_on_game_start(const char *lobby_id, int creator_id) {
  if (!lobby_id) return false;

  char *result = startGame(lobby_id, creator_id);
  if (!result) return false;

  // Verifica se il risultato indica successo
  bool success = (strstr(result, "\"result\":true") != NULL);
  free(result);

  return success;
}

bool db_on_game_end(const char *lobby_id, int creator_id) {
  if (!lobby_id) return false;

  char *result = endGame(lobby_id, creator_id);
  if (!result) return false;

  bool success = (strstr(result, "\"result\":true") != NULL);
  free(result);

  return success;
}

void db_on_lobby_full(const char *lobby_id, int players_count) {
  printf("[DB] lobby FULL id=%s players=%d\n", lobby_id, players_count);
}