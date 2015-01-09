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
#include <check.h>

#include "hl.h"

START_TEST(test_list)
{
  LIST *list;
  char *string1 = "string1";
  char *string2 = "string2";
  char *ptr;

  list = hl_list_alloc();
  ck_assert(list != NULL);

  hl_list_push(list, string1, strlen(string1) + 1);
  ck_assert_int_eq(1, hl_list_count(list));
  ck_assert_str_eq(string1, hl_list_get(list, 0));
  ck_assert_str_eq(string1, hl_list_get_last(list));

  hl_list_push(list, string2, strlen(string2) + 1);
  ck_assert_int_eq(2, hl_list_count(list));
  ck_assert_str_eq(string2, hl_list_get(list, 1));
  ck_assert_str_eq(string2, hl_list_get_last(list));

  hl_list_pop(list);
  ck_assert_int_eq(1, hl_list_count(list));
  ck_assert_str_eq(string1, hl_list_get_last(list));

  hl_list_pop(list);
  ck_assert_int_eq(0, hl_list_count(list));

  ptr = hl_list_get_last(list);
  ck_assert(ptr == NULL);

  hl_list_free(list);
}
END_TEST

START_TEST(test_hash)
{
  HCODE *hash;
  LIST *list;
  char *string1 = "string1";
  char *string2 = "string2";
  char *value;
  int rc;

  hash = hl_hash_alloc(30);
  ck_assert(hash != NULL);

  hl_hash_put(hash, string1, string1, strlen(string1) + 1);
  value = hl_hash_get(hash, string1);
  ck_assert_str_eq(string1, value);

  hl_hash_put(hash, string2, string2, strlen(string2) + 1);
  value = hl_hash_get(hash, string2);
  ck_assert_str_eq(string2, value);

  /* check the keys */
  list = hl_hash_keys(hash);
  ck_assert_int_eq(2, hl_list_count(list));
  hl_list_free(list);

  /* check the values */
  list = hl_hash_values(hash);
  ck_assert_int_eq(2, hl_list_count(list));
  hl_list_free(list);

  /* check override the value of a specific key */
  hl_hash_put(hash, string2, string1, strlen(string1) + 1);
  value = hl_hash_get(hash, string2);
  ck_assert_str_eq(string1, value);

  /* check the deletion */
  rc = hl_hash_del(hash, string1);
  ck_assert_int_eq(rc, 1);
  value = hl_hash_get(hash, string1);
  ck_assert(value == NULL);

  hl_hash_free(hash);

  /* test collisions */
  hash = hl_hash_alloc(1);

  hl_hash_put(hash, string1, string1, strlen(string1) + 1);
  hl_hash_put(hash, string2, string2, strlen(string2) + 1);

  list = hl_hash_keys(hash);
  ck_assert_int_eq(2, hl_list_count(list));
  hl_list_free(list);

  list = hl_hash_values(hash);
  ck_assert_int_eq(2, hl_list_count(list));
  hl_list_free(list);

  hl_hash_free(hash);
}
END_TEST

Suite *hl_suite(void)
{
  Suite *s;
  TCase *tc_hl;

  s = suite_create("hl");
  tc_hl = tcase_create("hl");

  tcase_add_test(tc_hl, test_list);
  tcase_add_test(tc_hl, test_hash);

  suite_add_tcase(s, tc_hl);

  return s;
}
