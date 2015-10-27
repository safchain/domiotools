/*
 * Copyright (C) 2014 Sylvain Afchain
 *
 * This program is free software; you can redistribute it and/or modify it under the terms of the
 * GNU General Public License as published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without
 / even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along with this program; if
 * not, write to the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
 * 02110-1301, USA.
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "mem.h"
#include "hmap.h"
#include "list.h"

#define HSIZE 255

static inline void _free0(void **ptr)
{
  free(*ptr);
  *ptr = NULL;
}

HMAP *hl_hmap_alloc(unsigned int size)
{
  HMAP *hmap;

  if (!size) {
    size = HSIZE;
  }

  hmap = xmalloc(sizeof(HMAP));
  hmap->nodes = xcalloc(size, sizeof(HNODE));
  hmap->hsize = size;

  return hmap;
}

static inline unsigned int hl_hmap_get_key(HMAP *hmap, const char *key)
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

  return hash % hmap->hsize;
}

HNODE *hl_hmap_put(HMAP *hmap, const char *key, const void *value,
                   unsigned int size)
{
  HNODE *hnode_ptr = hmap->nodes;
  unsigned int index = 0;

  index = hl_hmap_get_key(hmap, key);

  hnode_ptr = (HNODE *) (hnode_ptr + index);
  do {
    /* new key / value */
    if (hnode_ptr->key == NULL) {
      if ((hnode_ptr->key = strdup(key)) == NULL) {
        alloc_error();
      }

      hnode_ptr->value = xmalloc(size);
      memcpy(hnode_ptr->value, value, size);

      break;
    }

    /* old key / new value ? */
    if (strcmp(key, hnode_ptr->key) == 0) {
      free(hnode_ptr->value);
      hnode_ptr->value = xmalloc(size);
      memcpy(hnode_ptr->value, value, size);

      break;
    }

    /* new node ? */
    if (hnode_ptr->next == NULL) {
      hnode_ptr->next = xcalloc(1, sizeof(HNODE));
    }

    hnode_ptr = hnode_ptr->next;
  } while (hnode_ptr != NULL);

  return hnode_ptr;
}

static HNODE *hl_hmap_get_node(HMAP *hmap, const char *key)
{
  HNODE *hnode_ptr = hmap->nodes;
  unsigned int index = 0;

  index = hl_hmap_get_key(hmap, key);

  hnode_ptr = (HNODE *) (hnode_ptr + index);
  do {
    if (hnode_ptr->key != NULL) {
      if (strcmp(key, hnode_ptr->key) == 0) {
        return hnode_ptr;
      }
    }

    hnode_ptr = hnode_ptr->next;
  } while (hnode_ptr != NULL);

  return NULL;
}


void *hl_hmap_get(HMAP *hmap, const char *key)
{
  HNODE *hnode_ptr = hl_hmap_get_node(hmap, key);
  if (hnode_ptr != NULL) {
    return hnode_ptr->value;
  }

  return NULL;
}

LIST *hl_hmap_keys(HMAP *hmap)
{
  HNODE *hnode_ptr;
  LIST *list;
  unsigned int i;

  list = hl_list_alloc();

  for (i = 0; i != hmap->hsize; i++) {
    hnode_ptr = (HNODE *) (hmap->nodes + i);

    do {
      if (hnode_ptr->key != NULL) {
        hl_list_push(list, hnode_ptr->key, strlen(hnode_ptr->key) + 1);
      }
      hnode_ptr = hnode_ptr->next;
    } while (hnode_ptr != NULL);
  }

  return list;
}

LIST *hl_hmap_values(HMAP *hmap)
{
  HNODE *hnode_ptr;
  LIST *list;
  unsigned int i;

  list = hl_list_alloc();

  for (i = 0; i != hmap->hsize; i++) {
    hnode_ptr = (HNODE *) (hmap->nodes + i);

    do {
      if (hnode_ptr->key != NULL) {
        hl_list_push(list, hnode_ptr->value, sizeof(void *));
      }
      hnode_ptr = hnode_ptr->next;
    } while (hnode_ptr != NULL);
  }

  return list;
}

void hl_hmap_reset(HMAP *hmap)
{
  HNODE *hnodes_ptr;
  HNODE *hnode_ptr;
  HNODE *next_ptr;
  unsigned int size = hmap->hsize;
  unsigned int i;

  hnodes_ptr = hmap->nodes;

  for (i = 0; i != size; i++) {
    hnode_ptr = ((HNODE *) hnodes_ptr) + i;

    if (hnode_ptr->key == NULL) {
      continue;
    }

    _free0((void *) &hnode_ptr->key);
    free(hnode_ptr->value);

    next_ptr = hnode_ptr->next;
    hnode_ptr->next = NULL;

    hnode_ptr = next_ptr;
    while (hnode_ptr != NULL) {
      free(hnode_ptr->key);
      free(hnode_ptr->value);

      next_ptr = hnode_ptr->next;
      free(hnode_ptr);

      hnode_ptr = next_ptr;
    }
  }
}

void hl_hmap_init_iterator(HMAP *hmap, HMAP_ITERATOR *iterator)
{
  iterator->hmap = hmap;
  iterator->index = 0;
  iterator->current = NULL;
}

inline HNODE *hl_hmap_iterate(HMAP_ITERATOR *iterator)
{
  HNODE *hnodes_ptr;
  HNODE *hnode_ptr;
  unsigned int size;

  size = iterator->hmap->hsize;
  hnodes_ptr = iterator->hmap->nodes;
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

void hl_hmap_free_node(HNODE *hnode)
{
  _free0((void *) &hnode->key);
  free(hnode->value);
}

int hl_hmap_del(HMAP *hmap, const char *key)
{
  HNODE *hnode_ptr = hl_hmap_get_node(hmap, key);
  if (hnode_ptr != NULL) {
    hl_hmap_free_node(hnode_ptr);
    return 1;
  }

  return 0;
}

void hl_hmap_free(HMAP *hmap)
{
  hl_hmap_reset(hmap);
  free(hmap->nodes);
  free(hmap);
}
