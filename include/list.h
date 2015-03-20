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

#ifndef __LIST_H
#define __LIST_H

typedef struct list_s LIST;
typedef struct list_node_s LIST_NODE;
typedef struct list_iterator_s LIST_ITERATOR;

struct list_s {
  LIST_NODE *nodes;
  LIST_NODE *last;
};

/* append value just behind the next pointer, then only one malloc/free for each node */
struct list_node_s {
  LIST_NODE *prev;
  LIST_NODE *next;
};

struct list_iterator_s {
  LIST_NODE *current;
};

/* prototypes */
LIST *hl_list_alloc();
void *hl_list_unshift(LIST *, void *, unsigned int);
void *hl_list_push(LIST *, void *, unsigned int);
void hl_list_pop(LIST *);
void *hl_list_get_last(LIST *);
void *hl_list_get(LIST *, unsigned int);
void hl_list_init_iterator(LIST *, LIST_ITERATOR *);
void *hl_list_iterate(LIST_ITERATOR *);
unsigned int hl_list_count(LIST *);
void hl_list_reset(LIST *);
void hl_list_free(LIST *);

#endif
