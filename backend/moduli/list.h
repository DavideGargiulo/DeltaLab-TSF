/**
 * @file list.h
 * @brief Implementazione di una lista generica doppiamente concatenata.
 *
 * Questo modulo fornisce una struttura dati per una lista doppiamente concatenata
 * che può contenere puntatori a qualsiasi tipo di dato (void*).
 */

#ifndef LIST_H
#define LIST_H

#include <stddef.h>

/**
 * @brief Rappresenta un singolo nodo della lista.
 * Contiene un puntatore ai dati e i puntatori al nodo successivo e precedente.
 */
typedef struct ListNode {
  void *data;             ///< Puntatore generico ai dati contenuti nel nodo.
  struct ListNode *next;  ///< Puntatore al nodo successivo nella lista.
  struct ListNode *prev;  ///< Puntatore al nodo precedente nella lista.
} ListNode;

/**
 * @brief Rappresenta la lista doppiamente concatenata.
 * Mantiene puntatori alla testa e alla coda per operazioni O(1) e
 * un conteggio degli elementi.
 */
typedef struct List {
  ListNode *head; ///< Puntatore al primo nodo della lista.
  ListNode *tail; ///< Puntatore all'ultimo nodo della lista.
  size_t count;   ///< Numero totale di elementi nella lista.
} List;

/**
 * @brief Macro per iterare in modo sicuro e pulito su ogni nodo di una lista.
 * @param list Un puntatore alla lista su cui iterare.
 * @param current_node Il nome della variabile (ListNode*) che rappresenterà il nodo corrente.
 * @example
 * LIST_FOREACH(my_list, node) {
 * MyDataType *data = (MyDataType*)node->data;
 * // ... usa i dati ...
 * }
 */
#define LIST_FOREACH(list, current_node) \
  for (ListNode *current_node = (list)->head; current_node != NULL; current_node = current_node->next)

/**
 * @brief Alloca e inizializza una nuova lista vuota.
 * @return Un puntatore alla lista creata, o NULL in caso di fallimento di malloc.
 */
List* listCreate();

/**
 * @brief Dealloca una lista e tutti i suoi nodi.
 * Opzionalmente, può deallocare anche i dati contenuti in ogni nodo
 * se viene fornita una funzione di deallocazione.
 * @param[in] list La lista da distruggere.
 * @param[in] free_data Un puntatore a una funzione che sa come deallocare
 * i dati contenuti in ogni nodo (es. `free`). Se NULL,
 * i dati non verranno deallocati.
 */
void listDestroy(List *list, void (*free_data)(void*));

/**
 * @brief Aggiunge un elemento in fondo alla lista (operazione di enqueue).
 * L'operazione ha una complessità temporale di O(1).
 * @param[in] list La lista a cui aggiungere l'elemento.
 * @param[in] data Un puntatore ai dati da aggiungere.
 * @return 0 in caso di successo, -1 in caso di fallimento (es. malloc).
 */
int listPushBack(List *list, void *data);

/**
 * @brief Rimuove e restituisce l'elemento in testa alla lista (operazione di dequeue).
 * L'operazione ha una complessità temporale di O(1).
 * @param[in] list La lista da cui rimuovere l'elemento.
 * @return Un puntatore ai dati dell'elemento rimosso, o NULL se la lista è vuota.
 */
void* listPopFront(List *list);

#endif