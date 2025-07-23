/**
 * @file list.c
 * @brief Implementazione delle funzioni per la lista generica doppiamente concatenata.
 */

#include <stdlib.h>
#include <stdio.h>
#include "list.h"

/**
 * @brief Alloca e inizializza una nuova lista vuota.
 */
List* listCreate() {
  List *list = malloc(sizeof(List));
  if (!list) {
    perror("listCreate: malloc fallito per la struttura List");
    return NULL;
  }
  list->head = NULL;
  list->tail = NULL;
  list->count = 0;
  return list;
}

/**
 * @brief Dealloca una lista, i suoi nodi e opzionalmente i dati contenuti.
 */
void listDestroy(List *list, void (*free_data)(void*)) {
  if (!list) {
    return;
  }

  ListNode *current = list->head;
  while (current != NULL) {
    ListNode *next = current->next;
    if (free_data && current->data) {
      free_data(current->data);
    }
    free(current);
    current = next;
  }
  free(list);
}

/**
 * @brief Aggiunge un elemento in fondo alla lista (complessità O(1)).
 */
int listPushBack(List *list, void *data) {
  if (!list) {
    return -1;
  }

  ListNode *newNode = malloc(sizeof(ListNode));
  if (!newNode) {
    perror("listPushBack: malloc fallito per ListNode");
    return -1;
  }
  newNode->data = data;
  newNode->next = NULL;

  if (list->tail == NULL) {
    newNode->prev = NULL;
    list->head = newNode;
    list->tail = newNode;
  } else {
    newNode->prev = list->tail;
    list->tail->next = newNode;
    list->tail = newNode;
  }

  list->count++;
  return 0;
}

/**
 * @brief Rimuove e restituisce l'elemento in testa alla lista (complessità O(1)).
 */
void* listPopFront(List *list) {
  if (!list || !list->head) {
    return NULL;
  }

  ListNode *nodeToRemove = list->head;
  void *data = nodeToRemove->data;

  list->head = nodeToRemove->next;
  
  if (list->head) {
    list->head->prev = NULL;
  } else {
    list->tail = NULL;
  }

  list->count--;
  free(nodeToRemove);
  return data;
}