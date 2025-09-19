#ifndef LOBBY_H
#define LOBBY_H

#include <pthread.h>
#include <stdbool.h>
#include "list.h"
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
  List *playersConnected; // Lista di Player
  LobbyRotation rotation;
  int idCreator; // ID del giocatore che ha creato la lobby
  bool isPrivate;
  LobbyStatus status;
  pthread_mutex_t mutex; // Mutex per la sincronizzazione

  List *queueWaitingPlayers; // Lista di Player in attesa di entrare
} Lobby;

char* getAllLobbies();

char* getLobbyById(const char* id);

char* getPlayerInfoById(int playerId);

Lobby* createLobby(int idCreator, bool isPrivate, LobbyRotation rotation);

char* createLobbyEndpoint(const char* requestBody);

char* deleteLobby(const char* lobbyId, int creatorId);

void destroyLobby(Lobby *lobby);

// bool removePlayerFromLobby(Lobby* lobby, const char* playerId);

#endif