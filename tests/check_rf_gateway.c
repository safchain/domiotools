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
#include <assert.h>
#include <stdarg.h>

#include "mock.h"
#include "rf_gateway.h"

int srts_handler(int type, int duration)
{

}

void test_rf_setup()
{
}

void test_rf_teardown()
{
}

START_TEST(test_srts_publish)
{
}
END_TEST

Suite *rf_suite(void)
{
  Suite *s;
  TCase *tc_rf;
  int rc;

  s = suite_create("rf_gateway");
  tc_rf = tcase_create("rf_gateway");

  tcase_add_checked_fixture(tc_rf, test_rf_setup, test_rf_teardown);
  tcase_add_test(tc_rf, test_srts_publish);

  suite_add_tcase(s, tc_rf);

  return s;
}
