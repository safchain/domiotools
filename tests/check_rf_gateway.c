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

#include "mem.h"
#include "rf_gateway.h"
#include "srts.h"
#include "mqtt.h"
#include "logging.h"

int debug;
int verbose;
struct dlog *DLOG;

int mqtt_publish(const char *output, const char *value)
{
  mock_called_with("mqtt_publish", (char *) value);

  return MQTT_SUCCESS;
}

int mqtt_subscribe(const char *input, void *obj,
        void (*callback)(void *data, const void *payload, int payloadlen))
{
  mock_called("mqtt_subscribe");
  mock_called_with("mqtt_subscribe:obj", obj);
  mock_called_with("mqtt_subscribe:callback", callback);

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
  return "UP";
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

void srts_transmit_persist(int gpio, char key, unsigned short address,
        unsigned char ctrl, int repeat, const char *persistence_path)
{
  int *a = xmalloc(sizeof(int));
  int *c = xmalloc(sizeof(int));

  *a = address;
  *c = ctrl;

  mock_called("srts_transmit_persist");
  mock_called_with("srts_transmit_persist:address", a);
  mock_called_with("srts_transmit_persist:ctrl", c);
  mock_called_with("srts_transmit_persist:path", (void *) persistence_path);
}

int srts_get_ctrl_int(const char *ctrl)
{
  if (strcasecmp(ctrl, "UP") == 0) {
    return UP;
  }

  return UNKNOWN;
}

void test_rf_setup()
{
  mock_init();
}

void test_rf_teardown()
{
  mock_destroy();
}

START_TEST(test_config_publishers_no_type_error)
{
  char *conf = "config:{"
    "globals:{"
        "gpio: 2;"
    "};"
    "publishers:({"
        "address: 3333;"
        "output: \"mqtt://localhost:1883/3333\";})"
    "}";
  int rc;

  rc = rf_gw_read_config(conf, 0);
  ck_assert_int_eq(0, rc);
}
END_TEST

START_TEST(test_config_publishers_no_output_error)
{
  char *conf = "config:{"
    "globals:{"
        "gpio: 2;"
    "};"
    "publishers:({"
        "type: \"srts\";"
        "address: 3333;})"
    "}";
  int rc;

  rc = rf_gw_read_config(conf, 0);
  ck_assert_int_eq(0, rc);
}
END_TEST

START_TEST(test_config_publishers_no_address_error)
{
  char *conf = "config:{"
    "globals:{"
        "gpio: 2;"
    "};"
    "publishers:({"
        "type: \"srts\";"
        "output: \"mqtt://localhost:1883/3333\";})"
    "}";
  int rc;

  rc = rf_gw_read_config(conf, 0);
  ck_assert_int_eq(0, rc);
}
END_TEST

START_TEST(test_config_publishers_success)
{
  char *conf = "config:{"
    "globals:{"
        "gpio: 2;"
    "};"
    "publishers:({"
        "type: \"srts\";"
        "address: 3333;"
        "output: \"mqtt://localhost:1883/3333\";})"
    "}";
  int rc;

  rc = rf_gw_read_config(conf, 0);
  ck_assert_int_eq(1, rc);
}
END_TEST

START_TEST(test_config_subscribers_no_type_error)
{
  char *conf = "config:{"
    "globals:{"
        "gpio: 2;"
    "};"
    "subscribers:({"
        "address: 3333;"
        "input: \"mqtt://localhost:1883/3333\";})"
    "}";
  int rc;

  rc = rf_gw_read_config(conf, 0);
  ck_assert_int_eq(0, rc);
  ck_assert_int_eq(0, mock_calls("mqtt_subscribe"));
}
END_TEST

START_TEST(test_config_subscribers_no_output_error)
{
  char *conf = "config:{"
    "globals:{"
        "gpio: 2;"
    "};"
    "subscribers:({"
        "type: \"srts\";"
        "address: 3333;})"
    "}";
  int rc;

  rc = rf_gw_read_config(conf, 0);
  ck_assert_int_eq(0, rc);
  ck_assert_int_eq(0, mock_calls("mqtt_subscribe"));
}
END_TEST

START_TEST(test_config_subscribers_no_address_error)
{
  char *conf = "config:{"
    "globals:{"
        "gpio: 2;"
    "};"
    "subscribers:({"
        "type: \"srts\";"
        "input: \"mqtt://localhost:1883/3333\";})"
    "}";
  int rc;

  rc = rf_gw_read_config(conf, 0);
  ck_assert_int_eq(0, rc);
  ck_assert_int_eq(0, mock_calls("mqtt_subscribe"));
}
END_TEST

START_TEST(test_config_subscribers_success)
{
  char *conf = "config:{"
    "globals:{"
        "gpio: 2;"
    "};"
    "subscribers:({"
        "type: \"srts\";"
        "address: 3333;"
        "input: \"mqtt://localhost:1883/3333\";})"
    "}";
  int rc;

  rc = rf_gw_read_config(conf, 0);
  ck_assert_int_eq(1, rc);
  ck_assert_int_eq(1, mock_calls("mqtt_subscribe"));
}
END_TEST

START_TEST(test_config_subscribers_callback)
{
  char *conf = "config:{"
    "globals:{"
        "gpio: 2;"
        "persistence_path: \"/tmp/persist\";"
    "};"
    "subscribers:({"
        "type: \"srts\";"
        "address: 3333;"
        "input: \"mqtt://localhost:1883/3333\";})"
    "}";
  void *obj;
  void (*callback)(void *data, const void *payload, int payloadlen);
  char *payload = "UP", *path;
  int rc, *value;

  rc = rf_gw_read_config(conf, 0);
  ck_assert_int_eq(1, rc);
  ck_assert_int_eq(1, mock_calls("mqtt_subscribe"));

  obj = mock_call("mqtt_subscribe:obj", 0);
  callback = mock_call("mqtt_subscribe:callback", 0);
  callback(obj, payload, strlen(payload));

  ck_assert_int_eq(1, mock_calls("srts_transmit_persist"));
  value = (int *) mock_call("srts_transmit_persist:address", 0);
  ck_assert_int_eq(3333, *value);
  free(value);

  value = (int *) mock_call("srts_transmit_persist:ctrl", 0);
  ck_assert_int_eq(UP, *value);
  free(value);

  path = (char *) mock_call("srts_transmit_persist:path", 0);
  ck_assert_str_eq("/tmp/persist", path);
}
END_TEST

START_TEST(test_srts_publish)
{
  char *conf = "config:{"
    "globals:{"
        "gpio: 2;"
    "};"
    "publishers:({"
        "type: \"srts\";"
        "address: 3333;"
        "output: \"mqtt://localhost:1883/3333\";})"
    "}";
  int rc, key, address;

  rc = rf_gw_read_config(conf, 0);
  ck_assert_int_eq(rc, 1);

  key = 88768;
  address = 1111;
  mock_will_return("srts_receive_key", &key, MOCK_RETURNED_ONCE);
  mock_will_return("srts_get_address", &address, MOCK_RETURNED_ONCE);

  rf_gw_handle_interrupt(0, 50);
  rf_gw_handle_interrupt(1, 1500);

  rc = mock_calls("srts_receive");
  ck_assert_int_eq(rc, 1);

  rc = mock_calls("mqtt_publish");
  ck_assert_int_eq(rc, 0);

  key = 9432;
  address = 3333;
  mock_will_return("srts_receive_key", &key, MOCK_RETURNED_ONCE);
  mock_will_return("srts_get_address", &address, MOCK_RETURNED_ONCE);

  rf_gw_handle_interrupt(1, 2000);

  rc = mock_calls("srts_receive");
  ck_assert_int_eq(rc, 2);

  rc = mock_calls("mqtt_publish");
  ck_assert_int_eq(rc, 1);
  ck_assert_str_eq("UP", mock_call("mqtt_publish", 0));
}
END_TEST

START_TEST(test_srts_publish_same_twice)
{
  char *conf = "config:{"
    "globals:{"
        "gpio: 2;"
    "};"
    "publishers:({"
        "type: \"srts\";"
        "address: 3333;"
        "output: \"mqtt://localhost:1883/3333\";})"
    "}";
  int rc, key, address;

  rc = rf_gw_read_config(conf, 0);
  ck_assert_int_eq(rc, 1);

  key = 3454;
  address = 3333;
  mock_will_return("srts_receive_key", &key, MOCK_RETURNED_ALWAYS);
  mock_will_return("srts_get_address", &address, MOCK_RETURNED_ONCE);

  rf_gw_handle_interrupt(0, 50);
  rf_gw_handle_interrupt(1, 1500);

  rc = mock_calls("srts_receive");
  ck_assert_int_eq(rc, 1);

  rc = mock_calls("mqtt_publish");
  ck_assert_int_eq(rc, 1);
  ck_assert_str_eq("UP", mock_call("mqtt_publish", 0));

  address = 3333;
  mock_will_return("srts_get_address", &address, MOCK_RETURNED_ONCE);

  rf_gw_handle_interrupt(1, 2000);

  rc = mock_calls("srts_receive");
  ck_assert_int_eq(rc, 2);

  rc = mock_calls("mqtt_publish");
  ck_assert_int_eq(rc, 1);
}
END_TEST

START_TEST(test_srts_publish_translations)
{
  char *conf = "config:{"
    "globals:{"
        "gpio: 2;"
    "};"
    "publishers:({"
        "type: \"srts\";"
        "address: 3333;"
        "output: \"mqtt://localhost:1883/3333\";"
        "translations: {"
                "UP: \"ON\";"
                "DOWN: \"OFF\";"
        "}})"
    "}";
  int rc, key, address;

  rc = rf_gw_read_config(conf, 0);
  ck_assert_int_eq(rc, 1);

  key = 88768;
  address = 1111;
  mock_will_return("srts_receive_key", &key, MOCK_RETURNED_ONCE);
  mock_will_return("srts_get_address", &address, MOCK_RETURNED_ONCE);

  rf_gw_handle_interrupt(0, 50);
  rf_gw_handle_interrupt(1, 1500);

  rc = mock_calls("srts_receive");
  ck_assert_int_eq(rc, 1);

  rc = mock_calls("mqtt_publish");
  ck_assert_int_eq(rc, 0);

  key = 9432;
  address = 3333;
  mock_will_return("srts_receive_key", &key, MOCK_RETURNED_ONCE);
  mock_will_return("srts_get_address", &address, MOCK_RETURNED_ONCE);

  rf_gw_handle_interrupt(1, 2000);

  rc = mock_calls("srts_receive");
  ck_assert_int_eq(rc, 2);

  rc = mock_calls("mqtt_publish");
  ck_assert_int_eq(rc, 1);
  ck_assert_str_eq("ON", mock_call("mqtt_publish", 0));
}
END_TEST

Suite *rf_suite(void)
{
  Suite *s;
  TCase *tc_rf;

  s = suite_create("rf_gateway");
  tc_rf = tcase_create("rf_gateway");

  tcase_add_checked_fixture(tc_rf, test_rf_setup, test_rf_teardown);
  tcase_add_test(tc_rf, test_config_publishers_no_type_error);
  tcase_add_test(tc_rf, test_config_publishers_no_output_error);
  tcase_add_test(tc_rf, test_config_publishers_no_address_error);
  tcase_add_test(tc_rf, test_config_publishers_success);
  tcase_add_test(tc_rf, test_config_subscribers_no_type_error);
  tcase_add_test(tc_rf, test_config_subscribers_no_output_error);
  tcase_add_test(tc_rf, test_config_subscribers_no_address_error);
  tcase_add_test(tc_rf, test_config_subscribers_success);
  tcase_add_test(tc_rf, test_config_subscribers_callback);
  tcase_add_test(tc_rf, test_srts_publish);
  tcase_add_test(tc_rf, test_srts_publish_same_twice);
  tcase_add_test(tc_rf, test_srts_publish_translations);
  suite_add_tcase(s, tc_rf);

  return s;
}

int main(void)
{
  SRunner *sr;
  int number_failed;

  DLOG = dlog_init(DLOG_NULL, DLOG_INFO, NULL);
  assert(DLOG != NULL);

  sr = srunner_create(rf_suite ());
  srunner_run_all(sr, CK_VERBOSE);

  number_failed = srunner_ntests_failed(sr);
  srunner_free(sr);

  return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
