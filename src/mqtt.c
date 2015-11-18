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
#include <arpa/inet.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>
#include <string.h>
#include <mosquitto.h>
#include <pthread.h>
#include <errno.h>
#include <limits.h>

#include "mem.h"
#include "hl.h"
#include "urlparser.h"
#include "mqtt.h"
#include "logging.h"
#include "str.h"

extern int verbose;
extern int debug;
extern struct dlog *DLOG;

#define MAX_QUEUE_MESSAGES  10

struct mqtt_broker {
  char *hostname;
  struct mosquitto *mosq;
  LIST *queue;
  int connected;
  pthread_t thread;
  pthread_mutex_t queue_mutex;
  int running;
  pthread_rwlock_t running_rwlock;
};

struct mqtt_message {
  char *topic;
  char *payload;
  unsigned int payload_len;
};

struct mqtt_subscriber {
  LIST *topic;
  void *obj;
  void (*callback)(void *obj, const char *topic, const void *payload,
          int payloadlen);
};

static HMAP *mqtt_brokers;
static HMAP *mqtt_topics;
static LIST *mqtt_subscribers;

static struct mqtt_broker *mqtt_broker_alloc(struct mosquitto *mosq,
        struct url *url)
{
  struct mqtt_broker *broker;

  broker = (struct mqtt_broker *)xcalloc(1, sizeof(struct mqtt_broker));

  broker->queue = hl_list_alloc();

  broker->hostname = strdup(url->hostname);
  if (broker->hostname == NULL) {
    alloc_error();
  }
  broker->mosq = mosq;
  pthread_mutex_init(&(broker->queue_mutex), NULL);

  return broker;
}

static void mqtt_broker_free(struct mqtt_broker *broker) {
  hl_list_free(broker->queue);
  free(broker->hostname);
  free(broker);
}

static int mqtt_publish_message(struct mqtt_broker *broker)
{
  struct mqtt_message *message;
  int rc;

  rc = pthread_mutex_lock(&(broker->queue_mutex));
  if (rc != 0) {
    dlog(DLOG, DLOG_ERR, "Unable to acquire queue lock when publishing");
    return 0;
  }

  message = hl_list_get_last(broker->queue);
  if (message == NULL) {
    goto clean;
  }
  rc = mosquitto_publish(broker->mosq, NULL, message->topic,
          message->payload_len, message->payload, 2, 0);
  if (rc != MOSQ_ERR_SUCCESS) {
    dlog(DLOG, DLOG_ERR, "Unable to publish to mqtt topic: %s, %s",
            message->topic, mosquitto_strerror(rc));
    goto clean;
  }
  free(message->topic);
  free(message->payload);

  hl_list_pop(broker->queue);

  pthread_mutex_unlock(&(broker->queue_mutex));

  return 1;

clean:
  pthread_mutex_unlock(&(broker->queue_mutex));

  return 0;
}

static void mqtt_connect_callback(struct mosquitto *mosq, void *obj, int rc)
{
  struct mqtt_broker *broker = (struct mqtt_broker *) obj;

  if (rc != MOSQ_ERR_SUCCESS) {
    dlog(DLOG, DLOG_ERR, "mqtt connection failed, %s", mosquitto_strerror(rc));
    return;
  }
  broker->connected = 1;
}

static void mqtt_disconnect_callback(struct mosquitto *mosq, void *obj, int rc)
{
  struct mqtt_broker *broker = (struct mqtt_broker *) obj;

  if (rc != MOSQ_ERR_SUCCESS) {
    dlog(DLOG, DLOG_ERR, "mqtt connection failed, %s", mosquitto_strerror(rc));
    return;
  }
  broker->connected = 0;
}

/*
static void mqtt_publish_callback(struct mosquitto *mosq, void *obj, int rc)
{
}
*/

static int mqtt_topics_match(struct mqtt_subscriber *subscriber,
        const struct mosquitto_message *message)
{
  LIST *topic;
  char *str1, *str2;
  unsigned int len1, len2, i1, i2;
  int rc = 0;

  topic = str_split(message->topic, "/");

  len1 = hl_list_len(subscriber->topic);
  len2 = hl_list_len(topic);

  if (len2 < len1) {
    return 0;
  }

  for (i1 = i2 = 0; i2 != len2; i2++) {
    if (i1 >= len1) {
      goto clean;
    }

    str1 = hl_list_get(subscriber->topic, i1);
    str2 = hl_list_get(topic, i2);

    if (strcmp(str1, "#") == 0) {
      rc = 1;
      goto clean;
    }

    if (strcmp(str1, "+") == 0) {
      i1++;
      continue;
    }

    if (strcmp(str1, str2) != 0) {
      goto clean;
    }
    i1++;
  }
  rc = 1;

clean:
  hl_list_free(topic);

  return rc;
}

static void mqtt_message_callback(struct mosquitto *mosq, void *obj,
        const struct mosquitto_message *message)
{
  struct mqtt_subscriber *subscriber;
  LIST_ITERATOR iterator;
  int match;

  hl_list_init_iterator(mqtt_subscribers, &iterator);
  while((subscriber = hl_list_iterate(&iterator)) != NULL) {
    match = mqtt_topics_match(subscriber, message);
    if (match) {
      subscriber->callback(subscriber->obj, message->topic, message->payload,
          message->payloadlen);
    }
  }
}

/*
static void mqtt_subscribe_callback(struct mosquitto *mosq, void *obj, int mid,
        int qos_count, const int *granted_qos)
{
}
*/

static void *mqtt_loop(void *ptr)
{
  struct mqtt_broker *broker = (struct mqtt_broker *) ptr;
  int rc, running;

  rc = pthread_rwlock_init(&(broker->running_rwlock), NULL);
  if (rc != 0) {
    dlog(DLOG, DLOG_ERR, "Unable to init a pthread lock");
    return NULL;
  }

  rc = pthread_rwlock_rdlock(&(broker->running_rwlock));
  if (rc != 0) {
    dlog(DLOG, DLOG_ERR, "Unable to get a running pthread lock");
    goto clean;
  }
  running = broker->running;
  rc = pthread_rwlock_unlock(&(broker->running_rwlock));
  if (rc != 0) {
    dlog(DLOG, DLOG_ERR, "Unable to get a running pthread lock");
  }

  if (running == -1) {
    goto clean;
  }

  broker->running = 1;
  while (1) {
    rc = pthread_rwlock_rdlock(&(broker->running_rwlock));
    if (rc != 0) {
      dlog(DLOG, DLOG_ERR, "Unable to get a running pthread lock");
      continue;
    }
    running = broker->running;
    rc = pthread_rwlock_unlock(&(broker->running_rwlock));
    if (rc != 0) {
      dlog(DLOG, DLOG_ERR, "Unable to get a running pthread lock");
    }

    if (running != 1) {
      break;
    }

    rc = mosquitto_loop(broker->mosq, 200, 1);
    if (rc != MOSQ_ERR_SUCCESS) {
      dlog(DLOG, DLOG_ERR, "Error during the mqtt loop processing, %s",
              mosquitto_strerror(rc));
      continue;
    }
    if (broker->connected) {
      mqtt_publish_message(broker);
    }
  }

clean:

  pthread_exit(NULL);

  return NULL;
}

static struct mqtt_broker *mqtt_connect(struct url *url)
{
  struct mqtt_broker *broker = NULL;
  struct mosquitto *mosq;
  int keepalive = 60, rc;

  mosq = mosquitto_new(NULL, 1, NULL);
  if (mosq == NULL) {
    dlog(DLOG, DLOG_EMERG, "Failed during mosquitto initialization");
    return NULL;
  }

  if (url->username != NULL) {
    rc = mosquitto_username_pw_set(mosq, url->username, url->password);
    if (rc != MOSQ_ERR_SUCCESS) {
      dlog(DLOG, DLOG_ERR, "Unable to specify username/password, %s",
              mosquitto_strerror(rc));
      goto clean;
    }
  }

  mosquitto_max_inflight_messages_set(mosq, 5);
  mosquitto_reconnect_delay_set(mosq, 1, 1000, 0);
  mosquitto_message_retry_set(mosq, 1);
  mosquitto_connect_callback_set(mosq, mqtt_connect_callback);
  mosquitto_disconnect_callback_set(mosq, mqtt_disconnect_callback);
  //mosquitto_publish_callback_set(mosq, mqtt_publish_callback);
  //mosquitto_subscribe_callback_set(mosq, mqtt_subscribe_callback);
  mosquitto_message_callback_set(mosq, mqtt_message_callback);

  broker = mqtt_broker_alloc(mosq, url);
  mosquitto_user_data_set(mosq, broker);

  rc = mosquitto_connect_async(mosq, url->hostname, url->port, keepalive);
  if (rc != MOSQ_ERR_SUCCESS) {
    dlog(DLOG, DLOG_ERR, "Unable to initiate a new mqtt connection to "
            "%s:%d, %s", url->hostname, url->port, mosquitto_strerror(rc));
    goto clean;
  }

  rc = pthread_create(&(broker->thread), NULL, mqtt_loop, broker);
  if (rc < 0) {
    dlog(DLOG, DLOG_ERR, "Error during the mqtt thread creation");
    goto clean;
  }

  return broker;

clean:
  mosquitto_destroy(mosq);
  if (broker != NULL) {
    mqtt_broker_free(broker);
  }

  return NULL;
}

static struct mqtt_broker *mqtt_broker_connect(struct url *url, int *rc)
{
  struct mqtt_broker *broker, **broker_ptr;

  if (mqtt_brokers == NULL) {
    dlog(DLOG, DLOG_ERR, "mqtt not initialized");
    *rc = MQTT_CONNECTION_ERROR;
    return NULL;
  }

  broker_ptr = hl_hmap_get(mqtt_brokers, url->hostname);
  if (broker_ptr == NULL) {
    broker = mqtt_connect(url);
    if (broker == NULL) {
      *rc = MQTT_CONNECTION_ERROR;
      return NULL;
    }
    broker_ptr = &broker;
    hl_hmap_put(mqtt_brokers, url->hostname, broker_ptr,
                sizeof(struct mqtt_broker *));
  } else {
    broker = *broker_ptr;
  }
  *rc = MQTT_SUCCESS;

  return broker;
}

static int check_mqtt_url(const char *str, struct url *url)
{
  if (strcmp(url->scheme, "mqtt") != 0 ||
      url->hostname == NULL ||
      url->path == NULL) {
    dlog(DLOG, DLOG_ERR, "Bad mqtt url: %s", str);
    return 0;
  }

  return 1;
}

int mqtt_publish(const char *output, const char *value)
{
  struct mqtt_broker *broker;
  struct mqtt_message message;
  struct url *url;
  int rc = MQTT_SUCCESS;

  dlog(DLOG, DLOG_INFO, "Sending mqtt notification to %s with %s as value",
          output, value);

  url = parse_url(output);
  if (url == NULL) {
    dlog(DLOG, DLOG_ERR, "Unable to parse the output mqtt url");
    return MQTT_BAD_URL;
  }
  memset(&message, 0, sizeof(struct mqtt_message));

  if (!check_mqtt_url(output, url)) {
    rc = MQTT_BAD_URL;
    goto clean;
  }

  broker = mqtt_broker_connect(url, &rc);
  if (broker == NULL) {
    goto clean;
  }

  message.topic = strdup(url->path);
  message.payload = strdup(value);
  if (message.topic == NULL || message.payload == NULL) {
    alloc_error();
  }
  message.payload_len = strlen(value);

  rc = pthread_mutex_lock(&(broker->queue_mutex));
  if (rc != 0) {
    dlog(DLOG, DLOG_ERR, "Unable to acquire queue lock when publishing");
    rc = MQTT_MESSAGE_ERROR;
    goto clean;
  }
  hl_list_unshift(broker->queue, &message, sizeof(struct mqtt_message));
  pthread_mutex_unlock(&(broker->queue_mutex));

  free_url(url);

  return 1;

clean:
  free_url(url);
  if (message.topic != NULL) {
    free(message.topic);
  }
  if (message.payload != NULL) {
    free(message.payload);
  }

  return rc;
}

int mqtt_subscribe(const char *input, void *obj,
        void (*callback)(void *obj, const char *topic, const void *payload,
            int payloadlen))
{
  struct mqtt_subscriber subscriber;
  struct mqtt_broker *broker;
  struct url *url = NULL;
  char *topic = NULL;
  int len, rc = MQTT_SUCCESS;

  url = parse_url(input);
  if (url == NULL) {
    dlog(DLOG, DLOG_ERR, "Unable to parse the output mqtt url");
    return MQTT_BAD_URL;
  }

  if (!check_mqtt_url(input, url)) {
    rc = MQTT_BAD_URL;
    goto clean;
  }

  broker = mqtt_broker_connect(url, &rc);
  if (broker == NULL) {
    goto clean;
  }

  // convert fragment to topic wildcard
  if (url->fragment != NULL) {
    len = strlen(url->path);
    topic = xcalloc(1, len + 2);
    strcpy(topic, url->path);
    *(topic + len) = '#';
  } else {
    topic = strdup(url->path);
    if (topic == NULL) {
      alloc_error();
    }
  }

  if (hl_hmap_get(mqtt_topics, topic) == NULL) {
    rc = mosquitto_subscribe(broker->mosq, NULL, topic, 2);
    if (rc != MOSQ_ERR_SUCCESS) {
      rc = MQTT_CONNECTION_ERROR;
      goto clean;
    }
    hl_hmap_put(mqtt_topics, topic, topic, 1);
  }

  subscriber.topic = str_split(topic, "/");
  subscriber.obj = obj;
  subscriber.callback = callback;

  hl_list_push(mqtt_subscribers, &subscriber, sizeof(struct mqtt_subscriber));

  free_url(url);

  return MQTT_SUCCESS;

clean:
  free_url(url);
  free(topic);

  return rc;
}

int mqtt_init() {
  mqtt_brokers = hl_hmap_alloc(32);
  mqtt_topics = hl_hmap_alloc(32);
  mqtt_subscribers = hl_list_alloc();

  mosquitto_lib_init();

  return 1;
}

void mqtt_destroy() {
  struct mqtt_subscriber *subscriber;
  struct mqtt_broker *broker;
  HMAP_ITERATOR h_iter;
  LIST_ITERATOR l_iter;
  HNODE *hnode;
  int rc;

  if (mqtt_brokers == NULL) {
    return;
  }

  hl_hmap_init_iterator(mqtt_brokers, &h_iter);
  while((hnode = hl_hmap_iterate(&h_iter)) != NULL) {
    broker = *((struct mqtt_broker **) hnode->value);

    rc = pthread_rwlock_wrlock(&(broker->running_rwlock));
    if (rc == 0) {
      broker->running = -1;
      pthread_rwlock_unlock(&(broker->running_rwlock));
    } else {
      /* fallback do it without a lock */
      dlog(DLOG, DLOG_ERR, "Unable to get a pthread lock");
      broker->running = -1;
    }

    pthread_join(broker->thread, NULL);
    free(broker->hostname);
  }
  hl_hmap_free(mqtt_brokers);
  mqtt_brokers = NULL;

  hl_hmap_free(mqtt_topics);
  mqtt_topics = NULL;

  hl_list_init_iterator(mqtt_subscribers, &l_iter);
  while((subscriber = hl_list_iterate(&l_iter)) != NULL) {
    hl_list_free(subscriber->topic);
  }
  hl_list_free(mqtt_subscribers);
  mqtt_subscribers = NULL;

  mosquitto_lib_cleanup();
}
