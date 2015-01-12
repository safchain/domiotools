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
#include <mosquitto.h>
#include <assert.h>
#include <stdarg.h>

#include "mock.h"
#include "mqtt.h"

struct mosquitto {
  int id;
  void *obj;
  void (*on_connect)(struct mosquitto *mosq, void *obj, int rc);
  void (*on_disconnect)(struct mosquitto *mosq, void *obj, int rc);
};

static int rc_success = MOSQ_ERR_SUCCESS;
static int rc_error = MOSQ_ERR_UNKNOWN;
int verbose = 0;

int mosquitto_lib_init(void)
{
}

int mosquitto_lib_cleanup(void)
{
}

const char *mosquitto_strerror(int mosq_errno)
{
  return "";
}

int mosquitto_connect_async(struct mosquitto *mosq, const char *host, int port,
        int keepalive)
{
  mock_called("mosquitto_connect_async");

  mosq->on_connect(mosq, mosq->obj, MOSQ_ERR_SUCCESS);

  return *((int *) mock_returns("mosquitto_connect_async"));
}

int mosquitto_loop(struct mosquitto *mosq, int timeout, int max_packets)
{
  int rc = *((int *) mock_returns("mosquitto_loop", mosq));

  mock_called("mosquitto_loop");

  return rc;
}

int mosquitto_max_inflight_messages_set(struct mosquitto *mosq,
        unsigned int max_inflight_messages)
{
  return MOSQ_ERR_SUCCESS;
}

int mosquitto_publish(struct mosquitto *mosq, int *mid, const char *topic,
        int payloadlen, const void *payload, int qos, bool retain)
{
  mock_called("mosquitto_publish");

  return MOSQ_ERR_SUCCESS;
}

int mosquitto_reconnect_delay_set(struct mosquitto *mosq,
        unsigned int reconnect_delay, unsigned int reconnect_delay_max,
        bool reconnect_exponential_backoff)
{
  return MOSQ_ERR_SUCCESS;
}

int mosquitto_username_pw_set(struct mosquitto *mosq, const char *username,
        const char *password)
{
  return MOSQ_ERR_SUCCESS;
}

struct mosquitto *mosquitto_new(const char *id, bool clean_session, void *obj)
{
  return (struct mosquitto *)malloc(sizeof(struct mosquitto));
}

void mosquitto_connect_callback_set(struct mosquitto *mosq,
        void (*on_connect)(struct mosquitto *, void *, int))
{
  mosq->on_connect = on_connect;
}

void mosquitto_destroy(struct mosquitto *mosq)
{
}

void mosquitto_disconnect_callback_set(struct mosquitto *mosq,
        void (*on_disconnect)(struct mosquitto *, void *, int))
{
  mosq->on_disconnect = on_disconnect;
}

void mosquitto_message_retry_set(struct mosquitto *mosq,
        unsigned int message_retry)
{
}

void mosquitto_publish_callback_set(struct mosquitto *mosq,
        void (*on_publish)(struct mosquitto *, void *, int))
{
}

void mosquitto_user_data_set(struct mosquitto *mosq, void *obj)
{
  mosq->obj = obj;
}

void test_mqtt_setup()
{
  mock_init();
  mqtt_init();
}

void test_mqtt_teardown()
{
  mqtt_destroy();
  mock_destroy();
}

START_TEST(test_mqtt_publish_bad_url)
{
  int rc;

  rc = mqtt_publish("mqtt:/locahost:1843/test", "test");
  ck_assert_int_eq(rc, MQTT_BAD_URL);

  rc = mqtt_publish("ftp://locahost:1843/test", "test");
  ck_assert_int_eq(rc, MQTT_BAD_URL);

  rc = mqtt_publish("mqtt://locahost:1843", "test");
  ck_assert_int_eq(rc, MQTT_BAD_URL);
}
END_TEST

START_TEST(test_mqtt_publish_connection_error)
{
  struct mosquitto mosq;
  int rc;

  mock_will_return("mosquitto_connect_async", &rc_error, MOCK_RETURNED_ALWAYS);

  rc = mqtt_publish("mqtt://locahost:1843/test", "test");
  ck_assert_int_eq(rc, MQTT_CONNECTION_ERROR);
}
END_TEST

START_TEST(test_mqtt_publish_success)
{
  struct mosquitto mosq;
  int rc;

  mock_will_return("mosquitto_connect_async", &rc_success, MOCK_RETURNED_ALWAYS);
  mock_will_return("mosquitto_loop", &rc_success, MOCK_RETURNED_ALWAYS);

  rc = mqtt_publish("mqtt://locahost:1843/test", "test");
  ck_assert_int_eq(rc, MQTT_SUCCESS);

  rc = mock_wait_to_be_called("mosquitto_publish", 2);
  ck_assert_int_eq(rc, 1);
}
END_TEST

START_TEST(test_mqtt_two_publishes_one_connect)
{
  struct mosquitto mosq;
  int rc;

  mock_will_return("mosquitto_connect_async", &rc_success, MOCK_RETURNED_ALWAYS);
  mock_will_return("mosquitto_loop", &rc_success, MOCK_RETURNED_ALWAYS);

  rc = mqtt_publish("mqtt://locahost:1843/test1", "test1");
  ck_assert_int_eq(rc, MQTT_SUCCESS);

  rc = mock_wait_to_be_called("mosquitto_publish", 2);
  ck_assert_int_eq(rc, 1);

  mock_reset_calls();

  rc = mqtt_publish("mqtt://locahost:1843/test2", "test2");
  ck_assert_int_eq(rc, MQTT_SUCCESS);

  rc = mock_wait_to_be_called("mosquitto_publish", 2);
  ck_assert_int_eq(rc, 1);

  /* since we are using the same host, we should not see a new connection */
  rc = mock_calls("mosquitto_connect_async");
  ck_assert_int_eq(rc, 0);
}
END_TEST

START_TEST(test_mqtt_two_publishes_two_connect)
{
  struct mosquitto mosq;
  int rc;

  mock_will_return("mosquitto_connect_async", &rc_success, MOCK_RETURNED_ALWAYS);
  mock_will_return("mosquitto_loop", &rc_success, MOCK_RETURNED_ALWAYS);

  rc = mqtt_publish("mqtt://locahost1:1843/test1", "test1");
  ck_assert_int_eq(rc, MQTT_SUCCESS);

  rc = mock_wait_to_be_called("mosquitto_publish", 2);
  ck_assert_int_eq(rc, 1);

  mock_reset_calls();

  rc = mqtt_publish("mqtt://locahost2:1843/test2", "test2");
  ck_assert_int_eq(rc, MQTT_SUCCESS);

  rc = mock_wait_to_be_called("mosquitto_publish", 2);
  ck_assert_int_eq(rc, 1);

  /* we are using a different host so a new connection should be initiated */
  rc = mock_calls("mosquitto_connect_async");
  ck_assert_int_eq(rc, 1);
}
END_TEST

static void *mosquitto_loop_with_disconnect(va_list *pa)
{
  struct mosquitto *mosq = (struct mosquitto *) va_arg(*pa, void *);
  int loop = mock_calls("mosquitto_loop");

  /* only one loop, after that, we initiate a disconnection */
  if (loop == 0) {
    return &rc_success;
  } else if (loop == 1) {
    mosq->on_disconnect(mosq, mosq->obj, MOSQ_ERR_SUCCESS);
    mock_called("mosquitto_disconnect");
  } else if (mock_calls("mosquitto_reconnect") == 1) {
    mosq->on_connect(mosq, mosq->obj, MOSQ_ERR_SUCCESS);
  }

  return &rc_success;
}

START_TEST(test_mqtt_disconnect)
{
  struct mosquitto mosq;
  int rc;

  mock_will_return("mosquitto_connect_async", &rc_success, MOCK_RETURNED_ALWAYS);
  mock_will_return("mosquitto_loop", mosquitto_loop_with_disconnect, MOCK_RETURNED_FNC);

  rc = mqtt_publish("mqtt://locahost:1843/test", "test1");
  ck_assert_int_eq(rc, MQTT_SUCCESS);

  rc = mock_wait_to_be_called("mosquitto_publish", 2);
  ck_assert_int_eq(rc, 1);

  rc = mock_wait_to_be_called("mosquitto_disconnect", 2);
  ck_assert_int_eq(rc, 1);

  mock_reset_calls_for("mosquitto_publish");

  /* we should be disconnected now */
  rc = mqtt_publish("mqtt://locahost:1843/test", "test2");
  ck_assert_int_eq(rc, MQTT_SUCCESS);

  /* check that since we are disconnected there is no publish */
  rc = mock_calls("mosquitto_publish");
  ck_assert_int_eq(rc, 0);

  /* initiate a reconnection */
  mock_called("mosquitto_reconnect");

  /* after the reconnection the remaining message should be published */
  rc = mock_wait_to_be_called("mosquitto_publish", 2);
  ck_assert_int_eq(rc, 1);
}
END_TEST

Suite *mqtt_suite(void)
{
  Suite *s;
  TCase *tc_mqtt;
  int rc;

  s = suite_create("mqtt");
  tc_mqtt = tcase_create("mqtt");

  tcase_add_checked_fixture(tc_mqtt, test_mqtt_setup, test_mqtt_teardown);
  tcase_add_test(tc_mqtt, test_mqtt_publish_bad_url);
  tcase_add_test(tc_mqtt, test_mqtt_publish_connection_error);
  tcase_add_test(tc_mqtt, test_mqtt_publish_success);
  tcase_add_test(tc_mqtt, test_mqtt_two_publishes_one_connect);
  tcase_add_test(tc_mqtt, test_mqtt_two_publishes_two_connect);
  tcase_add_test(tc_mqtt, test_mqtt_disconnect);

  suite_add_tcase(s, tc_mqtt);

  return s;
}

int main(void)
{
  Suite *s;
  SRunner *sr;
  int number_failed;

  sr = srunner_create(mqtt_suite ());
  srunner_run_all(sr, CK_VERBOSE);

  number_failed = srunner_ntests_failed(sr);
  srunner_free(sr);

  return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
