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
#include <cmocker.h>
#include <unistd.h>
#include <sys/time.h>

#include "mem.h"
#include "homeasy.h"
#include "logging.h"

struct pulse {
  int value;
  int duration;
};

struct dlog *DLOG;

void digitalWrite(int pin, int value)
{
  struct pulse *pulse = xmalloc(sizeof(struct pulse));
  pulse->value = value;

  mock_called_with("digitalWrite", pulse);
}

void delayMicroseconds(unsigned int howLong)
{
  struct pulse *pulse;
  int rc;

  rc = mock_calls("digitalWrite");
  ck_assert_int_ne(0, rc);

  pulse = mock_call("digitalWrite", rc - 1);
  pulse->duration = howLong;
}

void delay(unsigned int howLong)
{
  delayMicroseconds(howLong * 1000);
}

void random_signal()
{
  struct homeasy_payload payload;
  int i, value;

  for (i = 0; i != 33; i++) {
    value = 1 - value;
    homeasy_receive(value, 300, &payload);
  }
}

static int receive_pulses(struct homeasy_payload *payload)
{
  struct pulse *pulse;
  int i, pulses, rc = 0;

  pulses = mock_calls("digitalWrite");
  if (!pulses) {
    return 0;
  }

  for (i = 0; i < pulses; i++) {
    pulse = (struct pulse *) mock_call("digitalWrite", i);
    rc = homeasy_receive(pulse->value, pulse->duration, payload);
    if (rc != 0) {
      return rc;
    }
  }

  return rc;
}

static void free_pulses()
{
  struct pulse *pulse;
  int i, pulses;

  pulses = mock_calls("digitalWrite");
  if (!pulses) {
    return;
  }
  for (i = 0; i < pulses; i++) {
    pulse = (struct pulse*) mock_call("digitalWrite", i);
    free(pulse);
  }
}

START_TEST(test_homeasy_transmit_receive)
{
  struct homeasy_payload payload;
  int rc;

  memset(&payload, 0, sizeof(struct homeasy_payload));

  random_signal();
  homeasy_transmit(2, 123, 6, HOMEASY_ON, 1, 0);

  rc = receive_pulses(&payload);
  ck_assert_int_eq(1, rc);

  ck_assert_int_eq(123, payload.address);
  ck_assert_int_eq(6, payload.receiver);
  ck_assert_int_eq(1, payload.group);
  ck_assert_str_eq("ON", homeasy_get_ctrl_str(&payload));

  free_pulses();
}
END_TEST

START_TEST(test_homeasy_transmit_two_receives)
{
  struct homeasy_payload payload;
  int rc;

  memset(&payload, 0, sizeof(struct homeasy_payload));

  random_signal();
  homeasy_transmit(2, 123, 6, HOMEASY_ON, 1, 0);

  rc = receive_pulses(&payload);
  ck_assert_int_eq(1, rc);

  ck_assert_int_eq(123, payload.address);
  ck_assert_int_eq(6, payload.receiver);
  ck_assert_int_eq(1, payload.group);
  ck_assert_str_eq("ON", homeasy_get_ctrl_str(&payload));

  free_pulses();
  mock_reset_calls();

  random_signal();
  homeasy_transmit(2, 125, 7, HOMEASY_OFF, 0, 0);

  rc = receive_pulses(&payload);
  ck_assert_int_eq(1, rc);

  ck_assert_int_eq(125, payload.address);
  ck_assert_int_eq(7, payload.receiver);
  ck_assert_int_eq(0, payload.group);
  ck_assert_str_eq("OFF", homeasy_get_ctrl_str(&payload));

  free_pulses();
}
END_TEST

void test_homeasy_setup()
{
  mock_init();
}

void test_homeasy_teardown()
{
  mock_destroy();
}

Suite *homeasy_suite(void)
{
  Suite *s;
  TCase *tc_homeasy;

  s = suite_create("homeasy");
  tc_homeasy = tcase_create("homeasy");

  tcase_add_checked_fixture(tc_homeasy, test_homeasy_setup,
          test_homeasy_teardown);
  tcase_add_test(tc_homeasy, test_homeasy_transmit_receive);
  tcase_add_test(tc_homeasy, test_homeasy_transmit_two_receives);

  suite_add_tcase(s, tc_homeasy);

  return s;
}

int main(void)
{
  SRunner *sr;
  int number_failed;

  DLOG = dlog_init(DLOG_NULL, DLOG_DEBUG, NULL);
  assert(DLOG != NULL);

  sr = srunner_create(homeasy_suite ());
  srunner_run_all(sr, CK_VERBOSE);

  number_failed = srunner_ntests_failed(sr);
  srunner_free(sr);

  return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
