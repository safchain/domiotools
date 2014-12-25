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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <errno.h>
#include <check.h>

#include "urlparser.h"
#include "hl.h"

START_TEST(test_urlparser_full)
{
  struct url *u;

  u = parse_url("http://uu:pp@foo.org:8080/bbb?query#frag");
  ck_assert(u != NULL);
  ck_assert_str_eq(u->scheme, "http");
  ck_assert_str_eq(u->username, "uu");
  ck_assert_str_eq(u->password, "pp");
  ck_assert_str_eq(u->hostname, "foo.org");
  ck_assert_int_eq(u->port, 8080);
  ck_assert_str_eq(u->path, "/bbb");
  ck_assert_str_eq(u->query, "query");
  ck_assert_str_eq(u->fragment, "frag");

  free_url(u);
}
END_TEST

START_TEST(test_urlparser_no_query)
{
  struct url *u;

  u = parse_url("http://uu:pp@foo.org:8080/bbb#frag");
  ck_assert(u != NULL);
  ck_assert_str_eq(u->scheme, "http");
  ck_assert_str_eq(u->username, "uu");
  ck_assert_str_eq(u->password, "pp");
  ck_assert_str_eq(u->hostname, "foo.org");
  ck_assert_int_eq(u->port, 8080);
  ck_assert_str_eq(u->path, "/bbb");
  ck_assert(u->query == NULL);
  ck_assert_str_eq(u->fragment, "frag");

  free_url(u);
}
END_TEST

START_TEST(test_urlparser_no_fragment)
{
  struct url *u;

  u = parse_url("http://uu:pp@foo.org:8080/bbb?query");
  ck_assert(u != NULL);
  ck_assert_str_eq(u->scheme, "http");
  ck_assert_str_eq(u->username, "uu");
  ck_assert_str_eq(u->password, "pp");
  ck_assert_str_eq(u->hostname, "foo.org");
  ck_assert_int_eq(u->port, 8080);
  ck_assert_str_eq(u->path, "/bbb");
  ck_assert_str_eq(u->query, "query");
  ck_assert(u->fragment == NULL);

  free_url(u);
}
END_TEST

START_TEST(test_urlparser_no_password)
{
  struct url *u;

  u = parse_url("http://uu@foo.org:8080/bbb");
  ck_assert(u != NULL);
  ck_assert_str_eq(u->scheme, "http");
  ck_assert_str_eq(u->username, "uu");
  ck_assert(u->password == NULL);
  ck_assert_str_eq(u->hostname, "foo.org");
  ck_assert_int_eq(u->port, 8080);
  ck_assert_str_eq(u->path, "/bbb");

  free_url(u);
}
END_TEST

START_TEST(test_urlparser_no_credentials)
{
  struct url *u;

  u = parse_url("http://foo.org:8080/bbb");
  ck_assert(u != NULL);
  ck_assert_str_eq(u->scheme, "http");
  ck_assert(u->username == NULL);
  ck_assert(u->password == NULL);
  ck_assert_str_eq(u->hostname, "foo.org");
  ck_assert_int_eq(u->port, 8080);
  ck_assert_str_eq(u->path, "/bbb");

  free_url(u);
}
END_TEST

START_TEST(test_urlparser_no_port)
{
  struct url *u;

  u = parse_url("http://foo.org/bbb");
  ck_assert(u != NULL);
  ck_assert_str_eq(u->scheme, "http");
  ck_assert(u->username == NULL);
  ck_assert(u->password == NULL);
  ck_assert_str_eq(u->hostname, "foo.org");
  ck_assert_int_eq(u->port, 0);
  ck_assert_str_eq(u->path, "/bbb");

  free_url(u);
}
END_TEST

START_TEST(test_urlparser_no_path)
{
  struct url *u;

  u = parse_url("http://foo.org");
  ck_assert(u != NULL);
  ck_assert_str_eq(u->scheme, "http");
  ck_assert(u->username == NULL);
  ck_assert(u->password == NULL);
  ck_assert_str_eq(u->hostname, "foo.org");
  ck_assert_int_eq(u->port, 0);
  ck_assert(u->path == NULL);

  free_url(u);
}
END_TEST

START_TEST(test_urlparser_no_host)
{
  struct url *u;

  u = parse_url("file:///foo");
  ck_assert(u != NULL);
  ck_assert_str_eq(u->scheme, "file");
  ck_assert(u->username == NULL);
  ck_assert(u->password == NULL);
  ck_assert(u->hostname == NULL);
  ck_assert_int_eq(u->port, 0);
  ck_assert_str_eq(u->path, "/foo");

  free_url(u);
}
END_TEST

START_TEST(test_urlparser_no_host_fragment)
{
  struct url *u;

  u = parse_url("file:///foo#frag");
  ck_assert(u != NULL);
  ck_assert_str_eq(u->scheme, "file");
  ck_assert(u->username == NULL);
  ck_assert(u->password == NULL);
  ck_assert(u->hostname == NULL);
  ck_assert_int_eq(u->port, 0);
  ck_assert_str_eq(u->path, "/foo");
  ck_assert_str_eq(u->fragment, "frag");

  free_url(u);
}
END_TEST

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

  hash = hl_hash_alloc(30);
  ck_assert(hash != NULL);

  hl_hash_put(hash, string1, string1, strlen(string1) + 1);
  value = hl_hash_get(hash, string1);
  ck_assert_str_eq(string1, value);

  hl_hash_put(hash, string2, string2, strlen(string2) + 1);
  value = hl_hash_get(hash, string2);
  ck_assert_str_eq(string2, value);

  list = hl_hash_keys(hash);
  ck_assert_int_eq(2, hl_list_count(list));
  hl_list_free(list);

  list = hl_hash_values(hash);
  ck_assert_int_eq(2, hl_list_count(list));
  hl_list_free(list);

  hl_hash_put(hash, string2, string1, strlen(string1) + 1);
  value = hl_hash_get(hash, string1);
  ck_assert_str_eq(string1, value);

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

Suite *domiotools_suite(void)
{
  Suite *s;
  TCase *tc_url, *tc_hl;

  s = suite_create("Domiotools");

  tc_url = tcase_create("url");
  tcase_add_test(tc_url, test_urlparser_full);
  tcase_add_test(tc_url, test_urlparser_no_password);
  tcase_add_test(tc_url, test_urlparser_no_credentials);
  tcase_add_test(tc_url, test_urlparser_no_port);
  tcase_add_test(tc_url, test_urlparser_no_path);
  tcase_add_test(tc_url, test_urlparser_no_host);
  tcase_add_test(tc_url, test_urlparser_no_query);
  tcase_add_test(tc_url, test_urlparser_no_fragment);
  tcase_add_test(tc_url, test_urlparser_no_host_fragment);
  suite_add_tcase(s, tc_url);

  tc_hl = tcase_create("HashList");
  tcase_add_test(tc_hl, test_list);
  tcase_add_test(tc_hl, test_hash);
  suite_add_tcase(s, tc_hl);

  return s;
}

int main(void)
{
  Suite *s;
  SRunner *sr;
  int number_failed;

  s = domiotools_suite();
  sr = srunner_create(s);

  srunner_run_all(sr, CK_NORMAL);

  number_failed = srunner_ntests_failed(sr);
  srunner_free(sr);
  return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
