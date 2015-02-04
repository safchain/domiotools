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

Suite *urlparser_suite(void)
{
  Suite *s;
  TCase *tc_url;

  s = suite_create("url");
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

  return s;
}

int main(void)
{
  SRunner *sr;
  int number_failed;

  sr = srunner_create(urlparser_suite());
  srunner_run_all(sr, CK_VERBOSE);

  number_failed = srunner_ntests_failed(sr);
  srunner_free(sr);

  return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
