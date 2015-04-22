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
#include <signal.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include "utils.h"

START_TEST(test_mkpath)
{
  char *path = "/tmp/aaa/bbb";
  struct stat st;
  int rc;

  rmdir(path);
  rmdir("/tmp/aaa");

  rc = stat(path, &st);
  ck_assert_int_eq(-1, rc);

  rc = mkpath(path, 0755);
  ck_assert_int_ne(-1, rc);

  rc = stat(path, &st);
  ck_assert_int_ne(-1, rc);

  rmdir(path);
  rmdir("/tmp/aaa");
}
END_TEST

void test_utils_setup()
{
}

void test_utils_teardown()
{
}

Suite *utils_suite(void)
{
  Suite *s;
  TCase *tc_utils;

  s = suite_create("utils");
  tc_utils = tcase_create("utils");

  tcase_add_checked_fixture(tc_utils, test_utils_setup, test_utils_teardown);
  tcase_add_test(tc_utils, test_mkpath);
  suite_add_tcase(s, tc_utils);

  return s;
}

int main(void)
{
  SRunner *sr;
  int number_failed;

  sr = srunner_create(utils_suite());
  srunner_run_all(sr, CK_VERBOSE);

  number_failed = srunner_ntests_failed(sr);
  srunner_free(sr);

  return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
