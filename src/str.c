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

#include "mem.h"
#include "list.h"

LIST *str_split(const char *str, const char *delim)
{
  LIST *result;
  char *tmp, *token;

  tmp = strdup(str);
  if (tmp == NULL) {
    alloc_error();
  }

  result = hl_list_alloc();

  token = strtok(tmp, delim);
  while (token) {
    hl_list_push(result, token, strlen(token) + 1);
    token = strtok(0, delim);
  }

  free(tmp);

  return result;
}
