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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <errno.h>
#include <assert.h>
#include <pthread.h>
#include <time.h>
#include <unistd.h>
#include <stdarg.h>

#include "hl.h"
#include "mock.h"

struct returned {
  void *value;
  int type;
};

static HCODE *returns = NULL;
static HCODE *calls = NULL;
static pthread_mutex_t call_mutex;

void mock_init()
{
  returns = hl_hash_alloc(32);
  assert(returns != NULL);

  calls = hl_hash_alloc(32);
  assert(calls != NULL);
}

void mock_destroy()
{
  hl_hash_free(returns);
  returns = NULL;

  hl_hash_free(calls);
  calls = NULL;
}

void mock_will_return(const char *fnc, void *value, int type)
{
  struct returned *r = hl_hash_get(returns, fnc);

  if (r == NULL) {
    r = (struct returned *)malloc(sizeof(struct returned));
    assert(r != NULL);
  }

  r->value = value;
  r->type = type;

  hl_hash_put(returns, fnc, r, sizeof(struct returned));

  free(r);
}

void *mock_returns(const char *fnc, ...)
{
  struct returned *r = hl_hash_get(returns, fnc);
  void *value;
  va_list pa;

  if (r == NULL) {
    return NULL;
  }
  value = r->value;

  if (r->type == MOCK_RETURNED_ONCE) {
    hl_hash_del(returns, fnc);
  } else if (r->type == MOCK_RETURNED_FNC) {
    va_start(pa, fnc);
    value = ((void*(*)(va_list *pa))r->value)(&pa);
    va_end(pa);

    return value;
  }

  return value;
}

void mock_called(const char *fnc)
{
  int *value, v = 1;
  int rc;

  rc = pthread_mutex_lock(&call_mutex);
  assert(rc == 0);

  value = hl_hash_get(calls, fnc);

  if (value == NULL) {
    value = &v;
    hl_hash_put(calls, fnc, value, sizeof(int));
  }
  *value = *value + 1;

  rc = pthread_mutex_unlock(&call_mutex);
  assert(rc == 0);
}

void mock_reset_calls_for(const char *fnc)
{
  int value = 0;
  int rc;

  rc = pthread_mutex_lock(&call_mutex);
  assert(rc == 0);

  hl_hash_put(calls, fnc, &value, sizeof(int));

  rc = pthread_mutex_unlock(&call_mutex);
  assert(rc == 0);
}

int mock_calls(const char *fnc)
{
  int value = 0, *v;
  int rc;

  rc = pthread_mutex_lock(&call_mutex);
  assert(rc == 0);

  v = (int *)hl_hash_get(calls, fnc);
  if (v != NULL) {
    value = *v;
  }

  rc = pthread_mutex_unlock(&call_mutex);
  assert(rc == 0);

  return value;
}

void mock_reset_calls()
{
  hl_hash_reset(calls);
}

int mock_wait_for_call_num_higher_than(const char *fnc, int limit, int timeout)
{
  time_t start, now;
  int called;

  start = now = time(NULL);
  while(start + timeout > now) {
    called = mock_calls(fnc);
    if (called > limit) {
      return 1;
    }
    usleep(100);
    now = time(NULL);
  }

  return 0;
}

int mock_wait_to_be_called(const char *fnc, int timeout)
{
  return mock_wait_for_call_num_higher_than(fnc, 0, timeout);
}

