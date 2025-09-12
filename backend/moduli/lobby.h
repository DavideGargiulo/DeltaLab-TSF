/**
 * @file lobby.h
 * @brief Sistema di gestione delle lobby per giochi multiplayer.
 *
 * Questo modulo fornisce le strutture dati e le funzioni per gestire
 * le stanze di gioco, i giocatori e le code di attesa in modo thread-safe.
 */

#ifndef LOBBY_H
#define LOBBY_H

#include <pthread.h>
#include <stdbool.h>
#include "list.h"
#include "../utils.h"

#define ROOM_NAME_MAX_LEN 32 ///< Lunghezza massima del nome della stanza

/**
 * @brief Stati possibili di una stanza di gioco.
 */
typedef enum {
  ROOM_WAITING,   ///< In attesa del numero minimo di giocatori per iniziare.
  ROOM_PLAYING,   ///< Partita in corso. I nuovi giocatori vengono messi in coda.
  ROOM_FINISHED   ///< Turno finito, in attesa di mostrare i risultati o iniziare un nuovo turno.
} RoomStatus;

/**
 * @brief Direzione di rotazione dei turni di gioco.
 */
typedef enum {
  DIR_CLOCKWISE,
  DIR_COUNTERCLOCKWISE
} TurnDirection;

/**
 * @brief Struttura che rappresenta un giocatore connesso.
 */
typedef struct {
  int userId;       ///< ID univoco dell'utente (dal database).
  int clientSocket; ///< File descriptor del socket di connessione del client.
} Player;

/**
 * @brief Struttura che rappresenta una stanza di gioco (lobby).
 */
typedef struct {
  char id[ROOM_ID_LEN + 1];           ///< Corrisponde a 'id' (CHAR(6))
  int utenti_connessi;                ///< Corrisponde a 'utenti_connessi' (INT)
  TurnDirection rotazione;            ///< Corrisponde a 'rotazione' (VARCHAR)
  char id_accountCreatore[50]; ///< Corrisponde a 'id_accountCreatore' (CHAR(6))
  bool is_private;                    ///< Corrisponde a 'is_private' (BOOLEAN)
  RoomStatus status;                  ///< Corrisponde a 'status' (VARCHAR)
  
  // Campi gestiti solo a livello di applicazione
  pthread_mutex_t lock; 
  List* currentPlayers;  ///< Lista di giocatori attivi nella stanza.
  List* waitingPlayers;   ///< Lista di giocatori in attesa di entrare.
} Room;

/**
 * @brief Gestore che supervisiona tutte le stanze di gioco.
 */
typedef struct {
  List *rooms;            ///< Lista di tutte le stanze di gioco attive.
  pthread_mutex_t lock;   ///< Mutex per proteggere la lista delle stanze (aggiunta/rimozione).
} RoomManager;

/**
 * @brief Struttura dati pubblica e sicura per rappresentare una stanza in elenchi pubblici.
 */
typedef struct {
  char id[ROOM_ID_LEN + 1];      ///< ID della stanza.
  unsigned int currentPlayers;  ///< Numero attuale di giocatori.
  unsigned int maxPlayers;      ///< Numero massimo di giocatori.
  RoomStatus status;             ///< Stato della stanza.
} PublicRoomInfo;

/**
 * @brief Crea e inizializza il gestore di tutte le stanze.
 * @return Un puntatore al RoomManager creato, o NULL in caso di fallimento critico.
 */
RoomManager* createRoomManager();

/**
 * @brief Distrugge il gestore delle stanze e libera tutta la memoria associata.
 * @param[in] manager Il gestore da distruggere.
 */
void destroyRoomManager(RoomManager* manager);

/**
 * @brief Ottiene una lista di copie delle informazioni pubbliche di tutte le stanze.
 * @warning Il chiamante è responsabile di deallocare la lista restituita
 * e ogni elemento PublicRoomInfo al suo interno usando list_destroy(lista, free).
 * @param[in] manager Il gestore delle stanze.
 * @return Una nuova lista contenente le informazioni, o NULL in caso di errore.
 */
List* getAllRooms();

/**
 * @brief Crea una nuova stanza di gioco e la aggiunge al manager.
 * @param[in] manager Gestore delle stanze in cui aggiungere la nuova stanza.
 * @param[in] name Nome pubblico della stanza.
 * @param[in] min_players Numero minimo di giocatori per iniziare.
 * @param[in] max_players Numero massimo di giocatori consentiti.
 * @param[in] direction Direzione di rotazione dei turni (orario/antiorario).
 * @return Un puntatore alla stanza creata e aggiunta, o NULL in caso di errore.
 */
Room* createRoom(RoomManager* manager, unsigned int minPlayers, unsigned int maxPlayers, TurnDirection direction, Player* creator);

/**
 * @brief Trova una stanza esistente tramite il suo ID univoco.
 * @param[in] manager Il gestore delle stanze.
 * @param[in] id L'ID stringa della stanza da cercare.
 * @return Un puntatore alla stanza, o NULL se non trovata.
 */
Room* findRoomById(RoomManager* manager, const char* id);

/**
 * @brief Gestisce l'ingresso di un giocatore in una stanza.
 * Aggiunge il giocatore alla lista attiva o alla coda di attesa a seconda dello stato e
 * della capienza della stanza. La memoria del puntatore Player viene gestita da questa funzione.
 * @param[in] room La stanza a cui il giocatore vuole unirsi.
 * @param[in] player Il giocatore da aggiungere.
 * @return 0 in caso di successo, -1 in caso di errore.
 */
int handlePlayerJoin(Room* room, Player* player);

/**
 * @brief Gestisce l'uscita di un giocatore da una stanza.
 * Rimuove il giocatore dalla lista dei partecipanti attivi. Se applicabile,
 * promuove il primo giocatore dalla coda di attesa.
 * @param[in] room La stanza da cui il giocatore esce.
 * @param[in] player_id L'ID del giocatore da rimuovere.
 * @return 0 in caso di successo, -1 se il giocatore non viene trovato.
 */
int handlePlayerLeave(RoomManager* manager, Room* room, int playerID);

#endif