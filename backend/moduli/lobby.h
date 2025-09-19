#ifndef LOBBY_H
#define LOBBY_H

#include <pthread.h>
#include <stdbool.h>
#include "../utils.h"

typedef enum {
  CLOCKWISE,
  COUNTERCLOCKWISE
} LobbyRotation;

typedef enum {
  WAITING,
  IN_GAME,
  FINISHED
} LobbyStatus;

typedef struct {
  char id[7]; // 6 caratteri + terminatore
  LobbyRotation rotation;
  int idCreator; // ID del giocatore che ha creato la lobby
  bool isPrivate;
  LobbyStatus status;
  pthread_mutex_t mutex; // Mutex per la sincronizzazioneù
} Lobby;

char* getAllLobbies();

char* getLobbyById(const char* id);

char* getPlayerInfoById(int playerId);

Lobby* createLobby(int idCreator, bool isPrivate, LobbyRotation rotation);

char* createLobbyEndpoint(const char* requestBody);

char* deleteLobby(const char* lobbyId, int creatorId);

// bool removePlayerFromLobby(Lobby* lobby, const char* playerId);

#endif