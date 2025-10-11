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

typedef struct {
  int id;
  int playerId;
  char lobbyId[7];
  char status[21]; // 'active', 'waiting', 'left'
  int position;
  char nickname[51];
} LobbyPlayer;

char* getAllLobbies();

char* getLobbyById(const char* id);

char* getPlayerInfoById(int playerId);

Lobby* createLobby(int idCreator, bool isPrivate, LobbyRotation rotation);

char* createLobbyEndpoint(const char* requestBody);

char* deleteLobby(const char* lobbyId, int creatorId);

char* joinLobby(const char* lobbyId, int playerId);

char* getLobbyPlayers(const char* lobbyId);

char* leaveLobby(const char* lobbyId, int playerId);

char* startGame(const char* lobbyId, int creatorId);

char* endGame(const char* lobbyId, int creatorId);

char* getLobbyRotation(const char* lobbyId);

#endif