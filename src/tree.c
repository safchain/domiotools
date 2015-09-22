/*
 * Copyright (C) 2015 Sylvain Afchain
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

#include "mem.h"
#include "tree.h"

TREE_II *tree_ii_alloc()
{
  return xcalloc(1, sizeof(TREE_II));
}

static void tree_ii_insert_node(struct tree_ii_node_s **sub_tree,
        unsigned int key, unsigned int value)
{
  struct tree_ii_node_s *node;

  if (*sub_tree == NULL) {
    node = xcalloc(1, sizeof(struct tree_ii_node_s));
    node->key = key;
    node->value = value;

    *sub_tree = node;
    return;
  }

  if ((*sub_tree)->key == key) {
    (*sub_tree)->value = value;
  }

  node = xcalloc(1, sizeof(struct tree_ii_node_s));
  node->key = key;
  node->value = value;

  if (node->key < (*sub_tree)->key) {
    tree_ii_insert_node(&(*sub_tree)->left, key, value);
  } else {
    tree_ii_insert_node(&(*sub_tree)->right, key, value);
  }
}

void tree_ii_insert(TREE_II *tree, unsigned int key, unsigned int value)
{
  tree_ii_insert_node(&tree->nodes, key, value);
}

static unsigned int tree_ii_lookup_node(struct tree_ii_node_s *node,
        unsigned int key, int *err)
{
  if (node->key == key) {
    return node->value;
  }

  if (node->key > key && node->left != NULL) {
    return tree_ii_lookup_node(node->left, key, err);
  } else if (node->right != NULL) {
    return tree_ii_lookup_node(node->right, key, err);
  }

  *err = 1;
  return 0;
}

unsigned int tree_ii_lookup(TREE_II *tree, unsigned int key, int *err)
{
  if (tree->nodes == NULL) {
    *err = 1;
    return 0;
  }

  *err = 0;
  return tree_ii_lookup_node(tree->nodes, key, err);
}