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

#include <wiringPi.h>
#include <arpa/inet.h>
#include <libconfig.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>
#include <string.h>
#include <mosquitto.h>
#include <pthread.h>
#include <errno.h>
#include <limits.h>

#include "common.h"
#include "srts.h"
#include "hl.h"
#include "urlparser.h"

extern int verbose;
extern int debug;

#define MAX_QUEUE_MESSAGES  10

enum {
  SRTS = 1,
  HOMEASY
};

struct mqtt_broker {
  struct mosquitto *mosq;
  LIST *queue;
  int connected;
  pthread_mutex_t queue_mutex;
};

struct mqtt_message {
    char *topic;
    char *payload;
    unsigned int payload_len;
};

static HCODE *mqtt_brokers;

static config_t cfg;
static int publisher_types;

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

static int mqtt_publish(struct mqtt_broker *broker)
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
  int rc;

  while(1) {
    rc = mosquitto_loop(broker->mosq, 200, 1);
    if (rc != MOSQ_ERR_SUCCESS) {
      fprintf(stderr, "Error during the mqtt loop processing, %s\n",
              mosquitto_strerror(rc));
      continue;
    }
    if (broker->connected) {
      mqtt_publish(broker);
    }
  }

  return NULL;
}

static struct mqtt_broker *mqtt_connect(struct url *url)
{
  struct mqtt_broker *broker = NULL;
  struct mosquitto *mosq;
  pthread_t thread;
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

  rc = pthread_create(&thread, NULL, mqtt_loop, broker);
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

static int publish_to_mqtt(const char *output, const char *value)
{
  struct mqtt_broker *broker, **broker_ptr;
  struct mqtt_message message;
  struct url *url = NULL;
  int rc;

  if (verbose) {
    printf("Sending mqtt notification to %s with %s as value\n", output, value);
  }

  url = parse_url(output);
  if (url == NULL) {
      fprintf(stderr, "Unable to parse the output mqtt url\n");
      return 0;
  }

  broker_ptr = hl_hash_get(mqtt_brokers, url->hostname);
  if (broker_ptr == NULL) {
      broker = mqtt_connect(url);
      if (broker == NULL) {
          goto clean;
      }
      broker_ptr = &broker;
      hl_hash_put(mqtt_brokers, url->hostname, broker_ptr,
              sizeof(struct mqtt_broker *));
  } else {
    broker = *broker_ptr;
  }

  memset(&message, 0, sizeof(struct mqtt_message));
  message.topic = strdup(url->path);
  message.payload = strdup(value);
  if (message.topic == NULL || message.payload == NULL) {
    fprintf(stderr, "Memory allocation error during mqtt message allocation\n");
    goto clean;
  }
  message.payload_len = strlen(value);

  /* enqueue a new message */
  rc = pthread_mutex_lock(&(broker->queue_mutex));
  if (rc != 0) {
    fprintf(stderr, "Unable to acquire queue lock when publishing\n");
    goto clean;
  }
  hl_list_push(broker->queue, &message, sizeof(struct mqtt_message));
  pthread_mutex_unlock(&(broker->queue_mutex));

  free(url);

  return 1;

clean:
    free(url);
    if (message.topic != NULL) {
      free(message.topic);
    }
    if (message.payload != NULL) {
      free(message.payload);
    }

    return 0;
}

static const char *translate_value(config_setting_t *config, const char *value)
{
  config_setting_t *translations;
  const char *translation;

  translations = config_setting_get_member (config, "translations");
  if (translations == NULL) {
    return value;
  }

  if (config_setting_lookup_string (translations, value,
              (const char **) &translation) == CONFIG_TRUE) {
    return translation;
  }

  return value;
}

static int publish(config_setting_t *config, const char *value)
{
  const char *output;
  int rc;

  if (config_setting_lookup_string(config, "output",
              (const char **) &output) != CONFIG_TRUE) {
    fprintf(stderr, "No output defined for the publisher line: %d\n",
            config_setting_source_line(config));
    return -1;
  }

  if (strstr(output, "mqtt://") == output) {
    rc = publish_to_mqtt(output, translate_value(config, value));
  } else {
    fprintf(stderr, "Output unknown for the publisher line: %d\n",
            config_setting_source_line(config));
    return -1;
  }

  return rc;
}

static int srts_lookup_for_publisher(struct srts_payload *payload)
{
  config_setting_t *hs, *h;
  int address, value, i = 0;
  const char *type, *ctrl;

  hs = config_lookup(&cfg, "config.publishers");
  if (hs == NULL) {
    fprintf(stderr, "No publisher defined!\n");
    return 0;
  }

  do {
    h = config_setting_get_elem(hs, i);
    if (h != NULL) {
      if (config_setting_lookup_string(h, "type", &type) != CONFIG_TRUE) {
        fprintf(stderr, "No address defined for the publisher line: %d\n",
                config_setting_source_line(h));
        continue;
      }
      if (strcmp(type, "srts") != 0) {
        continue;
      }

      if (config_setting_lookup_int(h, "address", &value) != CONFIG_TRUE) {
        fprintf(stderr, "No address defined for the publisher line: %d\n",
                config_setting_source_line(h));
        continue;
      }
      address = srts_get_address(payload);
      if (address == value) {
        ctrl = srts_get_ctrl_str(payload);
        if (ctrl == NULL) {
          fprintf(stderr, "Srts ctrl unknown: %d\n", payload->ctrl);
          return 0;
        }

        return publish(h, ctrl);
      }
    }
    i++;
  } while (h != NULL);

  return 1;
}

int srts_handler(int type, int duration)
{
  struct srts_payload payload;
  static int last_key = 0;
  int rc;

  rc = srts_receive(type, duration, &payload);
  if (rc == 1) {
    if (last_key && payload.key == last_key) {
      return 0;
    }
    last_key = payload.key;
    if (verbose) {
      printf("Message correctly received\n");
      if (debug) {
        srts_print_payload(&payload);
      }
    }
    rc = srts_lookup_for_publisher(&payload);
  }
  return rc;
}

void handle_interrupt()
{
  static unsigned int last_change = 0;
  static unsigned int total_duration = 0;
  unsigned int duration = 0;
  long time;
  int type;

  type = digitalRead(2);
  if (type == LOW) {
    type = HIGH;
  } else {
    type = LOW;
  }

  time = micros();
  if (last_change) {
    duration = time - last_change;
  } else {
    last_change = time;
    return;
  }
  last_change = time;

  /* simple noise reduction */
  total_duration += duration;
  if (total_duration > 50) {

    /* ask for all defined publisher */
    if (publisher_types & SRTS) {
      srts_handler(type, total_duration);
    }
    total_duration = 0;
  }
}

static int add_publisher_type(char *type)
{
  if (strcmp(type, "srts") == 0) {
    publisher_types |= SRTS;
  } else if (strcmp(type, "homeasy") == 0) {
    publisher_types |= HOMEASY;
  } else {
    fprintf(stderr, "Publisher type unknown: %s\n", type);
    return 0;
  }

  return 1;
}

static int read_publisher_types()
{
  config_setting_t *hs, *h;
  unsigned int i = 0;
  char *type;

  hs = config_lookup(&cfg, "config.publishers");
  if (hs == NULL) {
    fprintf(stderr, "No publisher defined!\n");
    return 0;
  }

  do {
    h = config_setting_get_elem(hs, i);
    if (h != NULL) {
      if (!config_setting_lookup_string(h, "type", (const char **) &type)) {
        fprintf(stderr, "No type defined for the publisher line: %d\n",
                config_setting_source_line(h));
      } else {
        if (!add_publisher_type(type)) {
          return 0;
        }
      }
    }
    i++;
  } while (h != NULL);

  return 1;
}

static int read_config(char *filename)
{
  config_init(&cfg);

  if (!config_read_file(&cfg, filename)) {
    printf("\n%s:%d - %s", config_error_file(&cfg),
           config_error_line(&cfg), config_error_text(&cfg));
    config_destroy(&cfg);
    return 0;
  }

  if (!read_publisher_types()) {
    return 0;
  }

  return 1;
}

int main(int argc, char **argv)
{
  int gpio = 2;

  if (setuid(0)) {
    perror("setuid");
    return -1;
  }

  if (!read_config("rules.cfg")) {
    return -1;
  }

  if (wiringPiSetup() == -1) {
    fprintf(stderr, "Wiring Pi not installed");
    return -1;
  }

  srand(time(NULL));

  mosquitto_lib_init();

  mqtt_brokers = hl_hash_alloc(32);
  if (!mqtt_brokers) {
    fprintf(stderr,
            "Unable to allocate a hash table to store the mqtt brokers\n");
    return -1;
  }

  verbose = 1;
  debug = 1;

  piHiPri(99);
  pinMode(gpio, INPUT);
  wiringPiISR(gpio, INT_EDGE_BOTH, handle_interrupt);

  while (1) {
    sleep(1);
  }

  return 0;
}
