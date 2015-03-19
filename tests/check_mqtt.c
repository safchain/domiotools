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
#include <cmocker.h>

#include "mqtt.h"
#include "logging.h"

struct mosquitto {
  int id;
  void *obj;
  void (*on_connect)(struct mosquitto *mosq, void *obj, int rc);
  void (*on_disconnect)(struct mosquitto *mosq, void *obj, int rc);
  void (*on_message)(struct mosquitto *mosq, void *obj,
          const struct mosquitto_message *message);
};

static int rc_success = MOSQ_ERR_SUCCESS;
static int rc_error = MOSQ_ERR_UNKNOWN;

struct dlog *DLOG;
int verbose = 0;

int mosquitto_lib_init(void)
{
  return MOSQ_ERR_SUCCESS;
}

int mosquitto_lib_cleanup(void)
{
  return MOSQ_ERR_SUCCESS;
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
  struct mosquitto *mosq = (struct mosquitto *)malloc(sizeof(struct mosquitto));

  mock_called_with("mosquitto_new", mosq);

  return mosq;
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

void mosquitto_subscribe_callback_set(struct mosquitto *mosq,
        void (*on_subscribe)(struct mosquitto *, void *, int, int, const int *))
{
}

void mosquitto_message_callback_set(struct mosquitto *mosq,
        void (*on_message)(struct mosquitto *, void *,
            const struct mosquitto_message *))
{
  mosq->on_message = on_message;
}

void mosquitto_user_data_set(struct mosquitto *mosq, void *obj)
{
  mosq->obj = obj;
}

int mosquitto_subscribe(struct  mosquitto *mosq, int *mid,
        const char *sub, int qos)
{
  int rc = *((int *) mock_returns("mosquitto_subscribe"));

  mock_called_with("mosquitto_subscribe", (void *) sub);

  return rc;
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
  int rc;

  mock_will_return("mosquitto_connect_async", &rc_error,
          MOCK_RETURNED_ALWAYS);

  rc = mqtt_publish("mqtt://locahost:1843/test", "test");
  ck_assert_int_eq(rc, MQTT_CONNECTION_ERROR);
}
END_TEST

START_TEST(test_mqtt_publish_success)
{
  int rc;

  mock_will_return("mosquitto_connect_async", &rc_success,
          MOCK_RETURNED_ALWAYS);
  mock_will_return("mosquitto_loop", &rc_success, MOCK_RETURNED_ALWAYS);

  rc = mqtt_publish("mqtt://user1:pass1@locahost:1843/test", "test");
  ck_assert_int_eq(rc, MQTT_SUCCESS);

  rc = mock_wait_to_be_called("mosquitto_publish", 2);
  ck_assert_int_eq(rc, 1);
}
END_TEST

START_TEST(test_mqtt_two_publishes_one_connect)
{
  int rc;

  mock_will_return("mosquitto_connect_async", &rc_success,
          MOCK_RETURNED_ALWAYS);
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
  int rc;

  mock_will_return("mosquitto_connect_async", &rc_success,
          MOCK_RETURNED_ALWAYS);
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

START_TEST(test_mqtt_disconnect)
{
  struct mosquitto *mosq;
  int rc, loop;

  mock_will_return("mosquitto_connect_async", &rc_success,
          MOCK_RETURNED_ALWAYS);
  mock_will_return("mosquitto_loop", &rc_success, MOCK_RETURNED_ALWAYS);

  rc = mqtt_publish("mqtt://locahost:1843/test", "test1");
  ck_assert_int_eq(rc, MQTT_SUCCESS);

  ck_assert_int_eq(1, mock_calls("mosquitto_new"));
  mosq = (struct mosquitto *) mock_call("mosquitto_new", 0);

  rc = mock_wait_to_be_called("mosquitto_publish", 2);
  ck_assert_int_eq(rc, 1);

  mosq->on_disconnect(mosq, mosq->obj, MOSQ_ERR_SUCCESS);

  mock_reset_calls_for("mosquitto_publish");

  /* we should be disconnected now */
  rc = mqtt_publish("mqtt://locahost:1843/test", "test2");
  ck_assert_int_eq(rc, MQTT_SUCCESS);

  loop = mock_calls("mosquitto_loop");
  mock_wait_for_call_num_higher_than("mosquitto_loop", loop + 2, 2);

  /* check that since we are disconnected there is no publish */
  rc = mock_calls("mosquitto_publish");
  ck_assert_int_eq(rc, 0);

  mosq->on_connect(mosq, mosq->obj, MOSQ_ERR_SUCCESS);

  /* after the reconnection the remaining message should be published */
  rc = mock_wait_to_be_called("mosquitto_publish", 2);
  ck_assert_int_eq(rc, 1);
}
END_TEST

START_TEST(test_mqtt_subscribe)
{
  int rc;

  mock_will_return("mosquitto_subscribe", &rc_success, MOCK_RETURNED_ALWAYS);
  mock_will_return("mosquitto_connect_async", &rc_success,
          MOCK_RETURNED_ALWAYS);
  mock_will_return("mosquitto_loop", &rc_success, MOCK_RETURNED_ALWAYS);

  rc = mqtt_subscribe("mqtt://locahost:1843/test", NULL, NULL);
  ck_assert_int_eq(rc, MQTT_SUCCESS);
  ck_assert_int_eq(1, mock_calls("mosquitto_subscribe"));

  mock_reset_calls_for("mosquitto_subscribe");

  rc = mqtt_subscribe("mqtt://locahost:1843/test", NULL, NULL);
  ck_assert_int_eq(rc, MQTT_SUCCESS);
  ck_assert_int_eq(0, mock_calls("mosquitto_subscribe"));

  rc = mqtt_subscribe("mqtt://locahost:1843/test2", NULL, NULL);
  ck_assert_int_eq(rc, MQTT_SUCCESS);
  ck_assert_int_eq(1, mock_calls("mosquitto_subscribe"));
}
END_TEST

static void mqtt_message_callback(void *obj, const void *payload,
        int payloadlen)
{
  mock_called_with("mqtt_message_callback_obj", obj);
  mock_called_with("mqtt_message_callback_payload", (void *)payload);
}

START_TEST(test_mqtt_on_message)
{
  struct mosquitto *mosq;
  struct mosquitto_message message;
  char *obj1 = "1", *obj2 = "2", *obj3 = "3";
  char *topic = "/test", *payload = "UP";
  int rc;

  mock_will_return("mosquitto_subscribe", &rc_success, MOCK_RETURNED_ALWAYS);
  mock_will_return("mosquitto_connect_async", &rc_success,
          MOCK_RETURNED_ALWAYS);
  mock_will_return("mosquitto_loop", &rc_success, MOCK_RETURNED_ALWAYS);

  mock_will_return("on_message_topic", topic, MOCK_RETURNED_ONCE);
  mock_will_return("on_message_payload", payload, MOCK_RETURNED_ONCE);

  rc = mqtt_subscribe("mqtt://locahost:1843/test", obj1,
          mqtt_message_callback);
  ck_assert_int_eq(rc, MQTT_SUCCESS);
  rc = mqtt_subscribe("mqtt://locahost:1843/test", obj2,
          mqtt_message_callback);
  ck_assert_int_eq(rc, MQTT_SUCCESS);

  ck_assert_int_eq(1, mock_calls("mosquitto_new"));
  ck_assert_int_eq(1, mock_calls("mosquitto_subscribe"));

  mosq = (struct mosquitto *) mock_call("mosquitto_new", 0);

  message.topic = (char *) topic;
  message.payload = (char *) payload;
  message.payloadlen = strlen(message.payload);

  mosq->on_message(mosq, mosq->obj, &message);
  ck_assert_int_eq(2, mock_calls("mqtt_message_callback_obj"));

  ck_assert(obj1 == mock_call("mqtt_message_callback_obj", 0));
  ck_assert(strncmp(payload, mock_call("mqtt_message_callback_payload", 0),
              message.payloadlen) == 0);
  ck_assert(obj2 == mock_call("mqtt_message_callback_obj", 1));

  mock_reset_calls();

  rc = mqtt_subscribe("mqtt://locahost:1843/test3", obj3,
          mqtt_message_callback);
  ck_assert_int_eq(rc, MQTT_SUCCESS);

  mosq->on_message(mosq, mosq->obj, &message);
  ck_assert_int_eq(2, mock_calls("mqtt_message_callback_obj"));
}
END_TEST

START_TEST(test_mqtt_broker_connect_before_init)
{
  struct mqtt_broker *broker;
  int rc;

  mqtt_destroy();

  rc = mqtt_publish("mqtt://locahost:1843/test", "test");
  ck_assert_int_eq(rc, MQTT_CONNECTION_ERROR);
}
END_TEST

Suite *mqtt_suite(void)
{
  Suite *s;
  TCase *tc_mqtt;

  s = suite_create("mqtt");
  tc_mqtt = tcase_create("mqtt");

  tcase_add_checked_fixture(tc_mqtt, test_mqtt_setup, test_mqtt_teardown);
  tcase_add_test(tc_mqtt, test_mqtt_publish_bad_url);
  tcase_add_test(tc_mqtt, test_mqtt_publish_connection_error);
  tcase_add_test(tc_mqtt, test_mqtt_publish_success);
  tcase_add_test(tc_mqtt, test_mqtt_two_publishes_one_connect);
  tcase_add_test(tc_mqtt, test_mqtt_two_publishes_two_connect);
  tcase_add_test(tc_mqtt, test_mqtt_disconnect);
  tcase_add_test(tc_mqtt, test_mqtt_subscribe);
  tcase_add_test(tc_mqtt, test_mqtt_on_message);
  tcase_add_test(tc_mqtt, test_mqtt_broker_connect_before_init);

  suite_add_tcase(s, tc_mqtt);

  return s;
}

int main(void)
{
  SRunner *sr;
  int number_failed;

  DLOG = dlog_init(DLOG_NULL, DLOG_INFO, NULL);
  assert(DLOG != NULL);

  sr = srunner_create(mqtt_suite ());
  srunner_run_all(sr, CK_VERBOSE);

  number_failed = srunner_ntests_failed(sr);
  srunner_free(sr);

  return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
