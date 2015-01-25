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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#ifndef __HMAP_H
#define __HMAP_H

#include "list.h"

typedef struct hmap_s HMAP;
typedef struct hnode_s HNODE;
typedef struct hl_hmap_iterator_s HMAP_ITERATOR;

struct hmap_s {
  HNODE *nodes;
  unsigned int hsize;
};

struct hnode_s {
  char *key;
  void *value;
  HNODE *next;
};

struct hl_hmap_iterator_s {
  HMAP *hmap;
  unsigned int index;
  HNODE *current;
};

/* prototypes */
char *hl_hmap_get_error(void);
HMAP *hl_hmap_alloc();
HNODE *hl_hmap_put(HMAP *, const char *, const void *, unsigned int);
void *hl_hmap_get(HMAP *, const char *);
LIST *hl_hmap_keys(HMAP *);
LIST *hl_hmap_values(HMAP *);
void hl_hmap_reset(HMAP *);
void hl_hmap_free(HMAP *);
void hl_hmap_free_node(HNODE *);
HNODE *hl_hmap_iterate(HMAP_ITERATOR *);
void hl_hmap_init_iterator(HMAP *, HMAP_ITERATOR *);
int hl_hmap_del(HMAP *, const char *);

#endif
