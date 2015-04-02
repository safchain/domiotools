/*
 * Copyright (C) 2015 Sylvain Afchain
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

#ifndef __TREE_H
#define __TREE_H

typedef struct tree_ii_s TREE_II;

struct tree_ii_node_s {
  unsigned int key;
  unsigned int value;

  struct tree_ii_node_s *left;
  struct tree_ii_node_s *right;
};

struct tree_ii_s {
  struct tree_ii_node_s *nodes;
};

/* prototypes */
TREE_II *tree_ii_alloc();
void tree_ii_insert(TREE_II *tree, unsigned int key, unsigned int value);
unsigned int tree_ii_lookup(TREE_II *tree, unsigned int key, int *err);

#endif
