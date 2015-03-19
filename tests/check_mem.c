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

#include "mem.h"

START_TEST(test_xmalloc_fatal)
{
  xmalloc(10000000000000000);
}
END_TEST

START_TEST(test_xcalloc_fatal)
{
  xcalloc(2, 10000000000000000);
}
END_TEST

START_TEST(test_xmalloc_fatal_no_abort)
{
  mem_disable_abort_on_error();

  void *ptr = xmalloc(10000000000000000);
  ck_assert(ptr == NULL);
}
END_TEST

START_TEST(test_xcalloc_fatal_no_abort)
{
  mem_disable_abort_on_error();

  void *ptr = xcalloc(2, 10000000000000000);
  ck_assert(ptr == NULL);
}
END_TEST

void test_mem_setup()
{
  mem_enable_abort_on_error();
}

void test_mem_teardown()
{
}

Suite *mem_suite(void)
{
  Suite *s;
  TCase *tc_mem;

  s = suite_create("mem");
  tc_mem = tcase_create("mem");

  tcase_add_checked_fixture(tc_mem, test_mem_setup, test_mem_teardown);
  tcase_add_test_raise_signal(tc_mem, test_xmalloc_fatal, SIGABRT);
  tcase_add_test_raise_signal(tc_mem, test_xcalloc_fatal, SIGABRT);
  tcase_add_test(tc_mem, test_xmalloc_fatal_no_abort);
  tcase_add_test(tc_mem, test_xcalloc_fatal_no_abort);
  suite_add_tcase(s, tc_mem);

  return s;
}

int main(void)
{
  SRunner *sr;
  int number_failed;

  sr = srunner_create(mem_suite());
  srunner_run_all(sr, CK_VERBOSE);

  number_failed = srunner_ntests_failed(sr);
  srunner_free(sr);

  return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
