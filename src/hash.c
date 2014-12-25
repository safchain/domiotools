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
#include <stdio.h>
#include <string.h>

#include "hash.h"
#include "list.h"

#define HSIZE 255

static void _free0(void **ptr)
{
  free(*ptr);
  *ptr = NULL;
}

HCODE *hl_hash_alloc(unsigned int size)
{
  HCODE *hcode;

  if (!size) {
    size = HSIZE;
  }

  if ((hcode = malloc(sizeof(HCODE))) == NULL) {
    return NULL;
  }

  if ((hcode->nodes = calloc(size, sizeof(HNODE))) == NULL) {
    free(hcode);
    return NULL;
  }
  hcode->hsize = size;

  return hcode;
}

static inline unsigned int hl_hash_get_key(HCODE *hcode, const char *key)
{
  unsigned int hash = 0;
  const char *c = key;

  while(*c != '\0') {
    hash += *c++;
    hash += (hash << 10);
    hash ^= (hash >> 6);
  }

  hash += (hash << 3);
  hash ^= (hash >> 11);
  hash += (hash << 15);

  return hash % hcode->hsize;
}

HNODE *hl_hash_put(HCODE *hcode, const char *key, const void *value,
                   unsigned int size)
{
  HNODE *hnode_ptr = hcode->nodes;
  unsigned int index = 0;

  index = hl_hash_get_key(hcode, key);

  hnode_ptr = (HNODE *) (hnode_ptr + index);
  do {
    /* new key / value */
    if (hnode_ptr->key == NULL) {
      if ((hnode_ptr->key = strdup(key)) == NULL) {
        return NULL;
      }

      if ((hnode_ptr->value = malloc(size)) == NULL) {
        goto error;
      }
      memcpy(hnode_ptr->value, value, size);

      break;
    }

    /* old key / new value ? */
    if (strcmp(key, hnode_ptr->key) == 0) {
      free(hnode_ptr->value);
      if ((hnode_ptr->value = malloc(size)) == NULL) {
        goto error;
      }
      memcpy(hnode_ptr->value, value, size);

      break;
    }

    /* new node ? */
    if (hnode_ptr->next == NULL) {
      if ((hnode_ptr->next = calloc(1, sizeof(HNODE))) == NULL) {
        goto error;
      }
    }

    hnode_ptr = hnode_ptr->next;
  } while (hnode_ptr != NULL);

  return hnode_ptr;

error:
  free(hnode_ptr->key);

  return hnode_ptr;
}

void *hl_hash_get(HCODE *hcode, const char *key)
{
  HNODE *hnode_ptr = hcode->nodes;
  unsigned int index = 0;

  index = hl_hash_get_key(hcode, key);

  hnode_ptr = (HNODE *) (hnode_ptr + index);
  do {
    if (hnode_ptr->key != NULL) {
      if (strcmp(key, hnode_ptr->key) == 0) {
        return hnode_ptr->value;
      }
    }

    hnode_ptr = hnode_ptr->next;
  } while (hnode_ptr != NULL);

  return NULL;
}

LIST *hl_hash_keys(HCODE *hcode)
{
  HNODE *hnode_ptr = hcode->nodes;
  LIST *list;
  unsigned int i;

  if ((list = hl_list_alloc()) == NULL) {
    return NULL;
  }

  for (i = 0; i != hcode->hsize; i++) {
    hnode_ptr = (HNODE *) (hcode->nodes + i);

    do {
      if (hnode_ptr->key != NULL) {
        hl_list_push(list, hnode_ptr->key, strlen(hnode_ptr->key) + 1);
      }
      hnode_ptr = hnode_ptr->next;
    } while (hnode_ptr != NULL);
  }

  return list;
}

LIST *hl_hash_values(HCODE *hcode)
{
  HNODE *hnode_ptr = hcode->nodes;
  LIST *list;
  unsigned int i;

  if ((list = hl_list_alloc()) == NULL) {
    return NULL;
  }

  for (i = 0; i != hcode->hsize; i++) {
    hnode_ptr = (HNODE *) (hcode->nodes + i);

    do {
      if (hnode_ptr->key != NULL) {
        hl_list_push(list, hnode_ptr->value, sizeof(void *));
      }
      hnode_ptr = hnode_ptr->next;
    } while (hnode_ptr != NULL);
  }

  return list;
}

void hl_hash_reset(HCODE *hcode)
{
  HNODE *hnodes_ptr;
  HNODE *hnode_ptr;
  HNODE *next_ptr;
  unsigned int size = hcode->hsize;
  unsigned int i;

  hnodes_ptr = hcode->nodes;

  for (i = 0; i != size; i++) {
    hnode_ptr = ((HNODE *) hnodes_ptr) + i;

    if (hnode_ptr->key == NULL) {
      continue;
    }

    _free0((void *) &hnode_ptr->key);
    free(hnode_ptr->value);

    hnode_ptr = hnode_ptr->next;
    while (hnode_ptr != NULL) {
      _free0((void *) &hnode_ptr->key);
      free(hnode_ptr->value);

      next_ptr = hnode_ptr->next;
      free(hnode_ptr);

      hnode_ptr = next_ptr;
    }
  }
}

void hl_hash_init_iterator(HCODE *hcode, HCODE_ITERATOR *iterator)
{
  iterator->hcode = hcode;
  iterator->index = 0;
  iterator->current = NULL;
}

inline HNODE *hl_hash_iterate(HCODE_ITERATOR *iterator)
{
  HNODE *hnodes_ptr;
  HNODE *hnode_ptr;
  unsigned int size;

  size = iterator->hcode->hsize;
  hnodes_ptr = iterator->hcode->nodes;
  while (iterator->index != size) {
    if (iterator->current != NULL) {
      hnode_ptr = iterator->current;
      iterator->current = NULL;
    } else {
      hnode_ptr = (HNODE *) (hnodes_ptr + iterator->index++);
    }

    if (hnode_ptr->key != NULL) {
      iterator->current = hnode_ptr->next;
      return hnode_ptr;
    }
  }

  return NULL;
}

int hl_hash_del(char *key)
{
  return 0;
}

void hl_hash_free_node(HNODE *hnode)
{
  _free0((void *) &hnode->key);
  free(hnode->value);
}

void hl_hash_free(HCODE *hcode)
{
  hl_hash_reset(hcode);
  free(hcode->nodes);
  free(hcode);
}
