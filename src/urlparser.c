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

#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <errno.h>

#include "mem.h"
#include "urlparser.h"

static int astrncpy(char **dst, char *ptr, unsigned int len)
{
  if (!len) {
    *dst = NULL;
    return 1;
  }
  *dst = xcalloc(1, len + 1);
  strncpy(*dst, ptr, len);

  return 1;
}

void free_url(struct url *url)
{
  if (url->scheme) {
    free(url->scheme);
  }
  if (url->username) {
    free(url->username);
  }
  if (url->password) {
    free(url->password);
  }
  if (url->hostname) {
    free(url->hostname);
  }
  if (url->path) {
    free(url->path);
  }
  if (url->query) {
    free(url->query);
  }
  if (url->fragment) {
    free(url->fragment);
  }
  free(url);
}

struct url *parse_url(const char *str)
{
  char *ptr, *tmp, *tmp2, *buf, *end;
  struct url *url;
  int len;

  url = (struct url *) xcalloc(1, sizeof(struct url));
  ptr = (char *) str;

  /* parse scheme */
  if ((tmp = strchr(ptr, ':')) != NULL) {
    len = tmp - ptr;
    if (strncmp(ptr + len, "://", 3) != 0) {
      goto clean;
    }
    astrncpy(&(url->scheme), ptr, len);
    ptr = tmp + 3;
  } else {
    goto clean;
  }

  /* parse credentials */
  if ((tmp = strchr(ptr, '@')) != NULL) {
    if ((tmp2 = strchr(ptr, ':')) != 0 && tmp2 < tmp) {
      len = tmp - tmp2;
      astrncpy(&(url->password), tmp2 + 1, len - 1);
    } else {
      len = 0;
    }

    len = tmp - ptr - len;
    astrncpy(&(url->username), ptr, len);
    ptr += tmp - ptr + 1;
  }

  /* parse host/port */
  tmp = strchrnul(ptr, '/');
  if ((tmp2 = strchr(ptr, ':')) != 0 && tmp2 < tmp) {
    len = tmp - tmp2;
    astrncpy(&buf, tmp2 + 1, len - 1);

    url->port = strtol(buf, &end, 10);
    free(buf);

    if (errno == ERANGE && (url->port == LONG_MAX || url->port == LONG_MIN)) {
      goto clean;
    }
  } else {
    len = 0;
  }

  len = tmp - ptr - len;
  astrncpy(&(url->hostname), ptr, len);
  ptr += tmp - ptr;

  /* path */
  buf = ptr;

  /* query string and fragment */
  if ((tmp = strchr(ptr, '?')) != NULL) {
    end = tmp;
    ptr += tmp - ptr;

    tmp2 = strchrnul(ptr, '#');
    astrncpy(&(url->query), tmp + 1, tmp2 - tmp - 1);
    astrncpy(&(url->fragment), tmp2 + 1, strlen(tmp2));
  } else if ((tmp = strchr(ptr, '#')) != NULL) {
    end = tmp;
    ptr += tmp - ptr;

    astrncpy(&(url->fragment), ptr + 1, strlen(ptr));
  } else {
    end = buf + strlen(buf);
  }

  astrncpy(&(url->path), buf, end - buf);

  return url;

clean:
  free_url(url);
  return NULL;
}
