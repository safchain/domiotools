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
#include "srts.h"
#include "mqtt.h"

int debug;
int verbose;

const char *ctrl = "UP";

int mqtt_publish(const char *output, const char *value)
{
  if (strcmp(value, ctrl) == 0) {
    mock_called("mqtt_publish");
  }

  return MQTT_SUCCESS;
}

void srts_print_payload(struct srts_payload *payload)
{
}

int srts_get_address(struct srts_payload *payload)
{
  return *((int *) mock_returns("srts_get_address"));
}

const char *srts_get_ctrl_str(struct srts_payload *payload)
{
  return ctrl;
}

int srts_receive(int type, int duration, struct srts_payload *payload)
{
  int *key = (int *) mock_returns("srts_receive_key");
  if (key != NULL) {
    payload->key = *key;
  }

  mock_called("srts_receive");

  return 1;
}

void test_rf_setup()
{
  mock_init();
}

void test_rf_teardown()
{
  mock_destroy();
}

START_TEST(test_srts_publish)
{
  char *conf = "config:{"
    "publishers:({"
        "type: \"srts\";"
        "address: 3333;"
        "output: \"mqtt://localhost:1883/3333\";})"
    "}";
  int rc, key, address;

  rc = rf_gw_read_config(conf, 0);
  ck_assert_int_eq(rc, 1);

  key = 1;
  address = 1111;
  mock_will_return("srts_receive_key", &key, MOCK_RETURNED_ONCE);
  mock_will_return("srts_get_address", &address, MOCK_RETURNED_ONCE);

  rf_gw_handle_interrupt(0, 50);
  rf_gw_handle_interrupt(1, 1500);

  rc = mock_calls("srts_receive");
  ck_assert_int_eq(rc, 1);

  rc = mock_calls("mqtt_publish");
  ck_assert_int_eq(rc, 0);

  key = 2;
  address = 3333;
  mock_will_return("srts_receive_key", &key, MOCK_RETURNED_ONCE);
  mock_will_return("srts_get_address", &address, MOCK_RETURNED_ONCE);

  rf_gw_handle_interrupt(1, 2000);

  rc = mock_calls("srts_receive");
  ck_assert_int_eq(rc, 2);

  rc = mock_calls("mqtt_publish");
  ck_assert_int_eq(rc, 1);
}
END_TEST

START_TEST(test_srts_publish_same_twice)
{
  char *conf = "config:{"
    "publishers:({"
        "type: \"srts\";"
        "address: 3333;"
        "output: \"mqtt://localhost:1883/3333\";})"
    "}";
  int rc, key, address;

  rc = rf_gw_read_config(conf, 0);
  ck_assert_int_eq(rc, 1);

  key = 1;
  mock_will_return("srts_receive_key", &key, MOCK_RETURNED_ALWAYS);

  address = 3333;
  mock_will_return("srts_get_address", &address, MOCK_RETURNED_ONCE);

  rf_gw_handle_interrupt(0, 50);
  rf_gw_handle_interrupt(1, 1500);

  rc = mock_calls("srts_receive");
  ck_assert_int_eq(rc, 1);

  rc = mock_calls("mqtt_publish");
  ck_assert_int_eq(rc, 1);

  address = 3333;
  mock_will_return("srts_get_address", &address, MOCK_RETURNED_ONCE);

  rf_gw_handle_interrupt(1, 2000);

  rc = mock_calls("srts_receive");
  ck_assert_int_eq(rc, 2);

  rc = mock_calls("mqtt_publish");
  ck_assert_int_eq(rc, 1);
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
  tcase_add_test(tc_rf, test_srts_publish_same_twice);

  suite_add_tcase(s, tc_rf);

  return s;
}

int main(void)
{
  Suite *s;
  SRunner *sr;
  int number_failed;

  sr = srunner_create(rf_suite ());
  srunner_run_all(sr, CK_VERBOSE);

  number_failed = srunner_ntests_failed(sr);
  srunner_free(sr);

  return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}