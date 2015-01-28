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

#include <stdlib.h>
#include <stdio.h>

#include "mem.h"

void _alloc_error(const char *file, int line)
{
  fprintf(stderr, "Out of Memory in %s at line %d\n", file, line);
  abort();
}

void *_xmalloc (size_t n, const char *file, int line)
{
  void *p = malloc (n);
  if (p == NULL) {
    alloc_error();
  }
  return p;
}

void *_xcalloc(size_t nmemb, size_t size, const char *file, int line)
{
  void *p = calloc(nmemb, size);
  if (p == NULL) {
    alloc_error();
  }
  return p;
}

