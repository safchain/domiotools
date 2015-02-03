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
#include "srts.h"

struct pulse {
  int value;
  int time;
};

static unsigned long utime()
{
  struct timeval tv;
  gettimeofday(&tv, NULL);

  return 1000000 * tv.tv_sec + tv.tv_usec;
}

void digitalWrite(int pin, int value)
{
  struct pulse *pulse = xmalloc(sizeof(struct pulse));
  pulse->value = value;
  pulse->time = utime();

  mock_called_with("digitalWrite", pulse);
}

void delayMicroseconds(unsigned int howLong)
{
  usleep(howLong - 10);
}

void random_signal()
{
  struct srts_payload payload;
  int i, value;

  for (i = 0; i != 33; i++) {
    value = 1 - value;
    srts_receive(value, 300, &payload);
  }
}

static int receive_pulses(struct srts_payload *payload)
{
  struct pulse *pulse;
  unsigned long last_time = 0;
  int i, rc, pulses, value, duration;

  pulses = mock_calls("digitalWrite");
  if (!pulses) {
    return 0;
  }

  for (i = 0; i < pulses; i++) {
    pulse = (struct pulse *) mock_call("digitalWrite", i);
    if (last_time) {
      duration = pulse->time - last_time;
      rc = srts_receive(value, duration, payload);
      if (rc == -1) {
        return -1;
      }
    }
    value = pulse->value;
    last_time = pulse->time;
  }

  return rc;
}

static void free_pulses()
{
  struct pulse *pulse;
  int i, pulses;

  pulses = mock_calls("digitalWrite");
  for (i = 0; i < pulses; i++) {
    pulse = (struct pulse*) mock_call("digitalWrite", i);
    free(pulse);
  }
}

START_TEST(test_srts_transmit_receive)
{
  struct srts_payload payload;
  int rc;

  memset(&payload, 0, sizeof(struct srts_payload));

  random_signal();
  srts_transmit(2, 123, 456, UP, 789, 0);

  rc = receive_pulses(&payload);
  ck_assert_int_eq(1, rc);

  ck_assert_int_eq(123, payload.key);
  ck_assert_int_eq(456, srts_get_address(&payload));
  ck_assert_str_eq("UP", srts_get_ctrl_str(&payload));
  ck_assert_int_eq(789, payload.code);

  free_pulses();
}
END_TEST

START_TEST(test_srts_transmit_two_receives)
{
  struct srts_payload payload;
  int rc;

  memset(&payload, 0, sizeof(struct srts_payload));

  random_signal();
  srts_transmit(2, 123, 456, UP, 789, 0);

  rc = receive_pulses(&payload);
  ck_assert_int_eq(1, rc);

  ck_assert_int_eq(123, payload.key);
  ck_assert_int_eq(456, srts_get_address(&payload));
  ck_assert_str_eq("UP", srts_get_ctrl_str(&payload));
  ck_assert_int_eq(789, payload.code);

  free_pulses();
  mock_reset_calls();

  random_signal();
  srts_transmit(2, 123, 456, DOWN, 790, 0);

  rc = receive_pulses(&payload);
  ck_assert_int_eq(1, rc);

  ck_assert_int_eq(123, payload.key);
  ck_assert_int_eq(456, srts_get_address(&payload));
  ck_assert_str_eq("DOWN", srts_get_ctrl_str(&payload));
  ck_assert_int_eq(790, payload.code);

  free_pulses();
}
END_TEST

START_TEST(test_srts_transmit_receive_repeated)
{
  struct srts_payload payload;
  int rc;

  memset(&payload, 0, sizeof(struct srts_payload));

  random_signal();
  srts_transmit(2, 123, 456, UP, 789, 0);

  rc = receive_pulses(&payload);
  ck_assert_int_eq(1, rc);

  ck_assert_int_eq(123, payload.key);
  ck_assert_int_eq(456, srts_get_address(&payload));
  ck_assert_str_eq("UP", srts_get_ctrl_str(&payload));
  ck_assert_int_eq(789, payload.code);

  free_pulses();
  mock_reset_calls();

  random_signal();
  srts_transmit(2, 123, 456, UP, 789, 1);

  rc = receive_pulses(&payload);
  ck_assert_int_eq(1, rc);

  ck_assert_int_eq(123, payload.key);
  ck_assert_int_eq(456, srts_get_address(&payload));
  ck_assert_str_eq("UP", srts_get_ctrl_str(&payload));
  ck_assert_int_eq(789, payload.code);

  free_pulses();
}
END_TEST

START_TEST(test_srts_transmit_persist)
{
  struct srts_payload payload;;
  char *tmpname;
  int rc;

  tmpname = tmpnam(NULL);

  memset(&payload, 0, sizeof(struct srts_payload));

  random_signal();
  srts_transmit_persist(2, 123, 456, UP, 0, tmpname);

  rc = receive_pulses(&payload);
  ck_assert_int_eq(1, rc);

  ck_assert_int_eq(123, payload.key);
  ck_assert_int_eq(456, srts_get_address(&payload));
  ck_assert_str_eq("UP", srts_get_ctrl_str(&payload));
  ck_assert_int_eq(1, payload.code);

  free_pulses();
  mock_reset_calls();

  random_signal();
  srts_transmit_persist(2, 123, 456, UP, 0, tmpname);

  rc = receive_pulses(&payload);
  ck_assert_int_eq(1, rc);

  ck_assert_int_eq(123, payload.key);
  ck_assert_int_eq(456, srts_get_address(&payload));
  ck_assert_str_eq("UP", srts_get_ctrl_str(&payload));
  ck_assert_int_eq(2, payload.code);

  free_pulses();
}
END_TEST

void test_srts_setup()
{
  mock_init();
}

void test_srts_teardown()
{
  free_pulses();
  mock_destroy();
}

Suite *srts_suite(void)
{
  Suite *s;
  TCase *tc_srts;
  int rc;

  s = suite_create("srts");
  tc_srts = tcase_create("srts");

  tcase_add_checked_fixture(tc_srts, test_srts_setup, test_srts_teardown);
  tcase_add_test(tc_srts, test_srts_transmit_receive);
  tcase_add_test(tc_srts, test_srts_transmit_two_receives);
  tcase_add_test(tc_srts, test_srts_transmit_receive_repeated);
  tcase_add_test(tc_srts, test_srts_transmit_persist);

  suite_add_tcase(s, tc_srts);

  return s;
}

int main(void)
{
  Suite *s;
  SRunner *sr;
  int number_failed;

  sr = srunner_create(srts_suite ());
  srunner_run_all(sr, CK_VERBOSE);

  number_failed = srunner_ntests_failed(sr);
  srunner_free(sr);

  return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
