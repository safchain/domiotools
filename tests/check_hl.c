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

START_TEST(test_list_push_pop)
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

START_TEST(test_list_push_unshift)
{
  LIST *list;
  char *string1 = "string1";
  char *string2 = "string2";

  list = hl_list_alloc();
  ck_assert(list != NULL);

  hl_list_push(list, string1, strlen(string1) + 1);
  ck_assert_int_eq(1, hl_list_count(list));
  ck_assert_str_eq(string1, hl_list_get(list, 0));
  ck_assert_str_eq(string1, hl_list_get_last(list));

  hl_list_unshift(list, string2, strlen(string2) + 1);
  ck_assert_int_eq(2, hl_list_count(list));
  ck_assert_str_eq(string2, hl_list_get(list, 0));
  ck_assert_str_eq(string1, hl_list_get_last(list));

  hl_list_free(list);
}
END_TEST

START_TEST(test_list_unshift)
{
  LIST *list;
  char *string1 = "string1";
  char *string2 = "string2";

  list = hl_list_alloc();
  ck_assert(list != NULL);

  hl_list_unshift(list, string1, strlen(string1) + 1);
  ck_assert_int_eq(1, hl_list_count(list));
  ck_assert_str_eq(string1, hl_list_get(list, 0));
  ck_assert_str_eq(string1, hl_list_get_last(list));

  hl_list_unshift(list, string2, strlen(string2) + 1);
  ck_assert_int_eq(2, hl_list_count(list));
  ck_assert_str_eq(string2, hl_list_get(list, 0));
  ck_assert_str_eq(string1, hl_list_get_last(list));

  hl_list_free(list);
}
END_TEST

START_TEST(test_hmap_put_get)
{
  HMAP *hmap;
  LIST *list;
  char *string1 = "string1";
  char *string2 = "string2";
  char *value;

  hmap = hl_hmap_alloc(30);
  ck_assert(hmap != NULL);

  hl_hmap_put(hmap, string1, string1, strlen(string1) + 1);
  value = hl_hmap_get(hmap, string1);
  ck_assert_str_eq(string1, value);

  hl_hmap_put(hmap, string2, string2, strlen(string2) + 1);
  value = hl_hmap_get(hmap, string2);
  ck_assert_str_eq(string2, value);

  /* check the keys */
  list = hl_hmap_keys(hmap);
  ck_assert_int_eq(2, hl_list_count(list));
  hl_list_free(list);

  /* check the values */
  list = hl_hmap_values(hmap);
  ck_assert_int_eq(2, hl_list_count(list));
  hl_list_free(list);

  hl_hmap_free(hmap);
}
END_TEST

START_TEST(test_hmap_override)
{
  HMAP *hmap;
  char *string1 = "string1";
  char *string2 = "string2";
  char *value;

  hmap = hl_hmap_alloc(30);
  ck_assert(hmap != NULL);

  hl_hmap_put(hmap, string1, string1, strlen(string1) + 1);
  value = hl_hmap_get(hmap, string1);
  ck_assert_str_eq(string1, value);

  hl_hmap_put(hmap, string1, string2, strlen(string2) + 1);
  value = hl_hmap_get(hmap, string1);
  ck_assert_str_eq(string2, value);

  hl_hmap_free(hmap);
}
END_TEST

START_TEST(test_hmap_delete)
{
  HMAP *hmap;
  char *string1 = "string1";
  char *value;
  int rc;

  hmap = hl_hmap_alloc(30);
  ck_assert(hmap != NULL);

  hl_hmap_put(hmap, string1, string1, strlen(string1) + 1);
  value = hl_hmap_get(hmap, string1);
  ck_assert_str_eq(string1, value);

  rc = hl_hmap_del(hmap, string1);
  ck_assert_int_eq(rc, 1);
  value = hl_hmap_get(hmap, string1);
  ck_assert(value == NULL);

  hl_hmap_free(hmap);
}
END_TEST

START_TEST(test_hmap_collision)
{
  HMAP *hmap;
  LIST *list;
  char *string1 = "string1";
  char *string2 = "string2";

  hmap = hl_hmap_alloc(1);
  ck_assert(hmap != NULL);

  hl_hmap_put(hmap, string1, string1, strlen(string1) + 1);
  hl_hmap_put(hmap, string2, string2, strlen(string2) + 1);

  list = hl_hmap_keys(hmap);
  ck_assert_int_eq(2, hl_list_count(list));
  hl_list_free(list);

  list = hl_hmap_values(hmap);
  ck_assert_int_eq(2, hl_list_count(list));
  hl_list_free(list);

  hl_hmap_free(hmap);
}
END_TEST

START_TEST(test_tree_ii_insert)
{
  TREE_II *tree;
  int value, err;

  tree = tree_ii_alloc();
  ck_assert(tree != NULL);

  tree_ii_insert(tree, 122, 9);
  tree_ii_insert(tree, 123, 10);
  tree_ii_insert(tree, 124, 11);

  value = tree_ii_lookup(tree, 122, &err);
  ck_assert_int_eq(0, err);
  ck_assert_int_eq(9, value);

  value = tree_ii_lookup(tree, 123, &err);
  ck_assert_int_eq(0, err);
  ck_assert_int_eq(10, value);

  value = tree_ii_lookup(tree, 124, &err);
  ck_assert_int_eq(0, err);
  ck_assert_int_eq(11, value);

  tree_ii_insert(tree, 124, 12);
  value = tree_ii_lookup(tree, 124, &err);
  ck_assert_int_eq(0, err);
  ck_assert_int_eq(12, value);

  tree_ii_lookup(tree, 125, &err);
  ck_assert_int_eq(1, err);
}
END_TEST

Suite *hl_suite(void)
{
  Suite *s;
  TCase *tc_hl;

  s = suite_create("hl");
  tc_hl = tcase_create("hl");

  tcase_add_test(tc_hl, test_list_push_pop);
  tcase_add_test(tc_hl, test_list_push_unshift);
  tcase_add_test(tc_hl, test_list_unshift);

  tcase_add_test(tc_hl, test_hmap_put_get);
  tcase_add_test(tc_hl, test_hmap_override);
  tcase_add_test(tc_hl, test_hmap_delete);
  tcase_add_test(tc_hl, test_hmap_collision);

  tcase_add_test(tc_hl, test_tree_ii_insert);
  suite_add_tcase(s, tc_hl);

  return s;
}

int main(void)
{
  SRunner *sr;
  int number_failed;

  sr = srunner_create(hl_suite());
  srunner_run_all(sr, CK_VERBOSE);

  number_failed = srunner_ntests_failed(sr);
  srunner_free(sr);

  return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
