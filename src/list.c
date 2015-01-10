/*
 * Copyright (C) 2014 Sylvain Afchain
 *
 * This program is free software; you can redistribute it and/or modify it under the terms of the
 * GNU General Public License as published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without
 * even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along with this program; if
 * not, write to the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
 * 02110-1301, USA.
 */

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "list.h"

LIST *hl_list_alloc()
{
  LIST *list;

  if ((list = calloc(1, sizeof(LIST))) == NULL) {
    return NULL;
  }

  return list;
}

int hl_list_unshift(LIST *list, void *value, unsigned int size)
{
  LIST_NODE *node_ptr;

  if ((node_ptr = malloc(sizeof(LIST_NODE) + size)) == NULL) {
    return -1;
  }
  node_ptr->prev = NULL;

  if (list->nodes != NULL) {
    list->nodes->prev = node_ptr;
    node_ptr->next = list->nodes;
    list->nodes = node_ptr;
  } else {
    list->nodes = node_ptr;
    node_ptr->next = NULL;
    list->last = node_ptr;
  }

  memcpy((char *) node_ptr + sizeof(LIST_NODE), value, size);

  return 0;
}

int hl_list_push(LIST *list, void *value, unsigned int size)
{
  LIST_NODE *node_ptr;

  if ((node_ptr = malloc(sizeof(LIST_NODE) + size)) == NULL) {
    return -1;
  }
  node_ptr->next = NULL;

  if (list->nodes == NULL) {
    list->nodes = node_ptr;
    node_ptr->prev = NULL;
  } else {
    list->last->next = node_ptr;
    node_ptr->prev = list->last;
  }
  list->last = node_ptr;

  memcpy((char *) node_ptr + sizeof(LIST_NODE), value, size);

  return 0;
}

static void hl_list_remove(LIST *list, LIST_NODE *node_ptr)
{
  if (list->nodes == node_ptr) {
     list->nodes = node_ptr->next;
     if (list->nodes != NULL) {
       list->nodes->prev = NULL;
     } else {
       list->last = NULL;
     }
  } else if (list->last == node_ptr) {
     list->last = node_ptr->prev;
     list->last->next = NULL;
  } else {
    node_ptr->prev->next = node_ptr->next;
    node_ptr->next->prev = node_ptr->prev;
  }
  free(node_ptr);
}

void hl_list_pop(LIST *list)
{
  hl_list_remove(list, list->last);
}

void *hl_list_get_last(LIST *list)
{
  if (list->last != NULL) {
    return ((char *) list->last + sizeof(LIST_NODE));
  }
  return NULL;
}

void *hl_list_get(LIST *list, unsigned int index)
{
  LIST_NODE *node_ptr = list->nodes;
  unsigned int i = 0;

  while (node_ptr != NULL) {
    if (i++ == index) {
      return ((char *) node_ptr + sizeof(LIST_NODE));
    }
    node_ptr = node_ptr->next;
  }

  return NULL;
}

void hl_list_init_iterator(LIST *list, LIST_ITERATOR *iterator)
{
  iterator->current = list->nodes;
}

inline void *hl_list_iterate(LIST_ITERATOR *iterator)
{
  void *value;

  if (iterator == NULL || iterator->current == NULL) {
    return NULL;
  }

  value = ((char *) iterator->current + sizeof(LIST_NODE));
  iterator->current = iterator->current->next;

  return value;
}

inline unsigned int hl_list_count(LIST *list)
{
  LIST_NODE *node_ptr = list->nodes;
  unsigned int i = 0;

  while (node_ptr != NULL) {
    node_ptr = node_ptr->next;
    i++;
  }

  return i;
}

inline void hl_list_reset(LIST *list)
{
  LIST_NODE *next_ptr;

  while (list->nodes != NULL) {
    next_ptr = list->nodes->next;
    free(list->nodes);
    list->nodes = next_ptr;
  }
  list->last = NULL;
}

void hl_list_free(LIST *list)
{
  hl_list_reset(list);
  free(list);
}
