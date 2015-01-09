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

#ifndef __HCODE_H
#define __HCODE_H

#include "list.h"

typedef struct hcode_s HCODE;
typedef struct hnode_s HNODE;
typedef struct hl_hash_iterator_s HCODE_ITERATOR;

struct hcode_s {
  HNODE *nodes;
  unsigned int hsize;
};

struct hnode_s {
  char *key;
  void *value;
  HNODE *next;
};

struct hl_hash_iterator_s {
  HCODE *hcode;
  unsigned int index;
  HNODE *current;
};

/* prototypes */
char *hl_hash_get_error(void);
HCODE *hl_hash_alloc();
HNODE *hl_hash_put(HCODE *, const char *, const void *, unsigned int);
void *hl_hash_get(HCODE *, const char *);
LIST *hl_hash_keys(HCODE *);
LIST *hl_hash_values(HCODE *);
void hl_hash_reset(HCODE *);
void hl_hash_free(HCODE *);
void hl_hash_free_node(HNODE *);
HNODE *hl_hash_iterate(HCODE_ITERATOR *);
void hl_hash_init_iterator(HCODE *, HCODE_ITERATOR *);
int hl_hash_del(HCODE *, const char *);

#endif
