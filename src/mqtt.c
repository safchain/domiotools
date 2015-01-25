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

#include "hl.h"
#include "urlparser.h"
#include "mqtt.h"

extern int verbose;
extern int debug;

#define MAX_QUEUE_MESSAGES  10

struct mqtt_broker {
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

static HMAP *mqtt_brokers;

static struct mqtt_broker *mqtt_broker_alloc(struct mosquitto *mosq)
{
  struct mqtt_broker *broker;

  broker = (struct mqtt_broker *)calloc(1, sizeof(struct mqtt_broker));
  if (broker == NULL) {
    return NULL;
  }
  broker->queue = hl_list_alloc();
  if (broker->queue == NULL) {
    goto clean;
  }
  broker->mosq = mosq;
  pthread_mutex_init(&(broker->queue_mutex), NULL);

  return broker;

clean:
  if (broker->queue != NULL) {
    hl_list_free(broker->queue);
  }
  free(broker);

  return NULL;
}

static void mqtt_broker_free(struct mqtt_broker *broker) {
  if (broker->queue != NULL) {
    hl_list_free(broker->queue);
  }
  free(broker);
}

static int mqtt_publish_message(struct mqtt_broker *broker)
{
  struct mqtt_message *message;
  int rc;

  rc = pthread_mutex_lock(&(broker->queue_mutex));
  if (rc != 0) {
    fprintf(stderr, "Unable to acquire queue lock when publishing\n");
    return 0;
  }

  message = hl_list_get_last(broker->queue);
  if (message == NULL) {
    goto clean;
  }
  rc = mosquitto_publish(broker->mosq, NULL, message->topic, message->payload_len,
          message->payload, 2, 0);
  if (rc != MOSQ_ERR_SUCCESS) {
    fprintf(stderr, "Unable to publish to mqtt topic: %s, %s\n", message->topic,
            mosquitto_strerror(rc));
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
    fprintf(stderr, "mqtt connection failed, %s\n", mosquitto_strerror(rc));
    return;
  }
  broker->connected = 1;
}

static void mqtt_disconnect_callback(struct mosquitto *mosq, void *obj, int rc)
{
  struct mqtt_broker *broker = (struct mqtt_broker *) obj;

  if (rc != MOSQ_ERR_SUCCESS) {
    fprintf(stderr, "mqtt connection failed, %s\n", mosquitto_strerror(rc));
    return;
  }
  broker->connected = 0;
}

static void mqtt_publish_callback(struct mosquitto *mosq, void *obj, int rc)
{
}

static void *mqtt_loop(void *ptr)
{
  struct mqtt_broker *broker = (struct mqtt_broker *) ptr;
  int rc, running;

  rc = pthread_rwlock_init(&(broker->running_rwlock), NULL);
  if (rc != 0) {
    fprintf(stderr, "Unable to init a pthread lock\n");
    return NULL;
  }
  broker->running = 1;

  while(1) {
    rc = pthread_rwlock_rdlock(&(broker->running_rwlock));
    if (rc != 0) {
      fprintf(stderr, "Unable to get a running pthread lock\n");
      continue;
    }
    running = broker->running;
    rc = pthread_rwlock_unlock(&(broker->running_rwlock));
    if (rc != 0) {
      fprintf(stderr, "Unable to get a running pthread lock\n");
    }

    if (! running) {
      break;
    }

    rc = mosquitto_loop(broker->mosq, 200, 1);
    if (rc != MOSQ_ERR_SUCCESS) {
      fprintf(stderr, "Error during the mqtt loop processing, %s\n",
              mosquitto_strerror(rc));
      continue;
    }
    if (broker->connected) {
      mqtt_publish_message(broker);
    }
  }

  return NULL;
}

static struct mqtt_broker *mqtt_connect(struct url *url)
{
  struct mqtt_broker *broker = NULL;
  struct mosquitto *mosq;
  int keepalive = 60, rc;

  mosq = mosquitto_new(NULL, 1, NULL);
  if (mosq == NULL) {
    fprintf(stderr, "Failed during mosquitto initialization\n");
    return NULL;
  }

  if (url->username != NULL) {
    rc = mosquitto_username_pw_set(mosq, url->username, url->password);
    if (rc != MOSQ_ERR_SUCCESS) {
        fprintf(stderr, "Unable to specify username/password, %s\n",
                mosquitto_strerror(rc));
        goto clean;
    }
  }

  mosquitto_max_inflight_messages_set(mosq, 5);
  mosquitto_reconnect_delay_set(mosq, 1, 1000, 0);
  mosquitto_message_retry_set(mosq, 1);
  mosquitto_connect_callback_set(mosq, mqtt_connect_callback);
  mosquitto_disconnect_callback_set(mosq, mqtt_disconnect_callback);
  mosquitto_publish_callback_set(mosq, mqtt_publish_callback);

  broker = mqtt_broker_alloc(mosq);
  if (broker == NULL) {
    fprintf(stderr, "Memory alloaction error when creating the mqtt broker\n");
    goto clean;
  }
  mosquitto_user_data_set(mosq, broker);

  rc = mosquitto_connect_async(mosq, url->hostname, url->port, keepalive);
  if (rc != MOSQ_ERR_SUCCESS) {
    fprintf(stderr, "Unable to initiate a new mqtt connection to %s:%d, %s\n",
            url->hostname, url->port, mosquitto_strerror(rc));
    goto clean;
  }

  rc = pthread_create(&(broker->thread), NULL, mqtt_loop, broker);
  if (rc < 0) {
    fprintf(stderr, "Error during the mqtt thread creation\n");
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

int mqtt_publish(const char *output, const char *value)
{
  struct mqtt_broker *broker, **broker_ptr;
  struct mqtt_message message;
  struct url *url = NULL;
  int rc = MQTT_SUCCESS;

  if (verbose) {
    printf("Sending mqtt notification to %s with %s as value\n", output, value);
  }

  if (mqtt_brokers == NULL) {
    fprintf(stderr, "mqtt not initialized\n");
    return MQTT_NOT_INITIALIZED;
  }

  url = parse_url(output);
  if (url == NULL) {
      fprintf(stderr, "Unable to parse the output mqtt url\n");
      return MQTT_BAD_URL;
  }

  memset(&message, 0, sizeof(struct mqtt_message));

  if (strcmp(url->scheme, "mqtt") != 0 ||
      url->hostname == NULL ||
      url->path == NULL) {
    fprintf(stderr, "Bad mqtt url: %s\n", output);
    rc = MQTT_BAD_URL;
    goto clean;
  }

  broker_ptr = hl_hmap_get(mqtt_brokers, url->hostname);
  if (broker_ptr == NULL) {
      broker = mqtt_connect(url);
      if (broker == NULL) {
          rc = MQTT_CONNECTION_ERROR;
          goto clean;
      }
      broker_ptr = &broker;
      hl_hmap_put(mqtt_brokers, url->hostname, broker_ptr,
              sizeof(struct mqtt_broker *));
  } else {
    broker = *broker_ptr;
  }

  message.topic = strdup(url->path);
  message.payload = strdup(value);
  if (message.topic == NULL || message.payload == NULL) {
    fprintf(stderr, "Memory allocation error during mqtt message allocation\n");
    rc = MQTT_MESSAGE_ERROR;
    goto clean;
  }
  message.payload_len = strlen(value);

  /* enqueue a new message */
  rc = pthread_mutex_lock(&(broker->queue_mutex));
  if (rc != 0) {
    fprintf(stderr, "Unable to acquire queue lock when publishing\n");
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

int mqtt_subscribe(const char *input, int type, int address)
{
  return 0;
}

int mqtt_init() {
  mqtt_brokers = hl_hmap_alloc(32);
  if (mqtt_brokers == NULL) {
    fprintf(stderr, "Memory allocation error during mqtt initialization\n");
    return 0;
  }
  mosquitto_lib_init();

  return 1;
}

void mqtt_destroy() {
  struct mqtt_broker *broker;
  HMAP_ITERATOR iterator;
  HNODE *hnode;
  int rc;

  if (mqtt_brokers == NULL) {
    return;
  }

  hl_hmap_init_iterator(mqtt_brokers, &iterator);
  while((hnode = hl_hmap_iterate(&iterator)) != NULL) {
    broker = *((struct mqtt_broker **) hnode->value);
    rc = pthread_rwlock_wrlock(&(broker->running_rwlock));
    if (rc == 0) {
      broker->running = 0;
      pthread_rwlock_unlock(&(broker->running_rwlock));
    } else {
      /* fallback do it without a lock */
      fprintf(stderr, "Unable to get a pthread lock\n");
      broker->running = 0;
    }

    pthread_join(broker->thread, NULL);
  }

  hl_hmap_free(mqtt_brokers);
  mqtt_brokers = NULL;

  mosquitto_lib_cleanup();
}
