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
#include <stdio.h>

#include "common.h"
#include "mem.h"
#include "srts.h"
#include "hl.h"
#include "urlparser.h"
#include "mqtt.h"
#include "logging.h"

#define SRTS_REPEAT 7

extern int verbose;
extern int debug;
extern struct dlog *DLOG;

enum {
  SRTS = 1,
  HOMEASY
};

struct rf_device {
  int type;
  int address;
};

static config_t cfg;
static int gpio;
static int publisher_types;
static char *persistence_path = "/tmp";

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
    dlog(DLOG, DLOG_ERR, "No output defined for the publisher line: %d",
            config_setting_source_line(config));
    return -1;
  }

  if (strstr(output, "mqtt://") == output) {
    rc = mqtt_publish(output, translate_value(config, value));
  } else {
    dlog(DLOG, DLOG_ERR, "Output unknown for the publisher line: %d",
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
    dlog(DLOG, DLOG_ERR, "No publisher defined");
    return 0;
  }

  do {
    h = config_setting_get_elem(hs, i);
    if (h != NULL) {
      if (config_setting_lookup_string(h, "type", &type) != CONFIG_TRUE) {
        dlog(DLOG, DLOG_ERR, "No address defined for the publisher line: %d",
                config_setting_source_line(h));
        continue;
      }
      if (strcmp(type, "srts") != 0) {
        continue;
      }

      if (config_setting_lookup_int(h, "address", &value) != CONFIG_TRUE) {
        dlog(DLOG, DLOG_ERR, "No address defined for the publisher line: %d",
                config_setting_source_line(h));
        continue;
      }
      address = srts_get_address(payload);
      if (address == value) {
        ctrl = srts_get_ctrl_str(payload);
        if (ctrl == NULL) {
          dlog(DLOG, DLOG_ERR, "Srts ctrl unknown: %d", payload->ctrl);
          return 0;
        }

        return publish(h, ctrl);
      }
    }
    i++;
  } while (h != NULL);

  return 1;
}

static int srts_handler(int type, int duration)
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
      dlog(DLOG, DLOG_INFO, "Message correctly received");
      if (debug) {
        srts_print_payload(&payload);
      }
    }
    rc = srts_lookup_for_publisher(&payload);
  }
  return rc;
}

void rf_gw_handle_interrupt(int type, long time)
{
  static unsigned int last_change = 0;
  static unsigned int total_duration = 0;
  unsigned int duration = 0;

  if (last_change) {
    duration = time - last_change;
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

static int str_type_to_int(const char *type)
{
  if (strcmp(type, "srts") == 0) {
    return SRTS;
  } else if (strcmp(type, "homeasy") == 0) {
    return HOMEASY;
  }

  return 0;
}

static int add_publisher_type(const char *type)
{
  int t = str_type_to_int(type);
  if (!t) {
    dlog(DLOG, DLOG_ERR, "Publisher type unknown: %s", type);
    return 0;
  }
  publisher_types |= t;

  return 1;
}

static void rf_mqtt_callback(void *obj, const void *payload, int payloadlen)
{
  int ctrl;
  struct rf_device *device = (struct rf_device *) obj;
  char *value = xmalloc(payloadlen + 1);

  memset(value, 0, payloadlen + 1);
  memcpy(value, payload, payloadlen);

  ctrl = srts_get_ctrl_int(value);
  if (ctrl == UNKNOWN) {
    goto clean;
  }

  /* log debug here */
  switch(device->type) {
    case SRTS:
      srts_transmit_persist(gpio, 0, device->address, ctrl,
              SRTS_REPEAT, persistence_path);
      break;
  }

clean:
  free(value);
}

static int subscribe()
{
  config_setting_t *hs, *h;
  struct rf_device *device;
  char *input, *type;
  int address, t, i = 0;

  hs = config_lookup(&cfg, "config.subscribers");
  if (hs == NULL) {
    /* log as warning or info */
    dlog(DLOG, DLOG_ERR, "No subscriber defined");
    return 1;
  }

  do {
    h = config_setting_get_elem(hs, i);
    if (h != NULL) {
      if (!config_setting_lookup_string(h, "input", (const char **) &input)) {
        dlog(DLOG, DLOG_ERR, "No type defined for the subscriber line: %d",
                config_setting_source_line(h));
        return 0;
      }
      if (!config_setting_lookup_string(h, "type", (const char **) &type)) {
        dlog(DLOG, DLOG_ERR, "No type defined for the subscriber line: %d",
                config_setting_source_line(h));
        return 0;
      }
      if (!config_setting_lookup_int(h, "address", (int *) &address)) {
        dlog(DLOG, DLOG_ERR, "No address defined for the subscriber line: %d",
                config_setting_source_line(h));
        return 0;
      }
      t = str_type_to_int(type);
      if (!t) {
        dlog(DLOG, DLOG_ERR, "Subscriber type unknown: %s", type);
        return 0;
      }
      device = xmalloc(sizeof(struct rf_device));
      device->address = address;
      device->type = t;

      if (!mqtt_subscribe(input, device, rf_mqtt_callback)) {
        return 0;
      }
    }
    i++;
  } while (h != NULL);

  return 1;
}

static int config_read_publisher_types()
{
  config_setting_t *hs, *h;
  char *type, *output;
  int address, i = 0;

  hs = config_lookup(&cfg, "config.publishers");
  if (hs == NULL) {
    /* log as warning or info */
    dlog(DLOG, DLOG_ERR, "No publisher defined");
    return 1;
  }

  do {
    h = config_setting_get_elem(hs, i);
    if (h != NULL) {
      if (!config_setting_lookup_string(h, "type", (const char **) &type)) {
        dlog(DLOG, DLOG_ERR, "No type defined for the publisher line: %d",
                config_setting_source_line(h));
        return 0;
      }
      if (!config_setting_lookup_string(h, "output", (const char **) &output)) {
        dlog(DLOG, DLOG_ERR, "No output defined for the publisher line: %d",
                config_setting_source_line(h));
        return 0;
      }
      if (!config_setting_lookup_int(h, "address", (int *) &address)) {
        dlog(DLOG, DLOG_ERR, "No address defined for the publisher line: %d",
                config_setting_source_line(h));
        return 0;
      }

      if (!add_publisher_type(type)) {
        return 0;
      }
    }
    i++;
  } while (h != NULL);

  return 1;
}

static int config_read_globals()
{
  int rc;

  rc = config_lookup_int(&cfg, "config.globals.gpio", &gpio);
  if (rc == CONFIG_FALSE) {
    /* log as warning or info */
    dlog(DLOG, DLOG_ERR, "No gpio defined");
    return 0;
  }

  rc = config_lookup_string(&cfg, "config.globals.persistence_path",
                            (const char **) &persistence_path);
  if (rc == CONFIG_FALSE) {
    /* log as warning or info */
    dlog(DLOG, DLOG_ERR, "No persistence path defined");
  }

  return 1;
}

int rf_gw_read_config(char *in, int file)
{
  int rc;

  config_init(&cfg);

  if (file) {
    rc = config_read_file(&cfg, in);
  } else {
    rc = config_read_string(&cfg, in);
  }

  if (!rc) {
    printf("\n%s:%d - %s", config_error_file(&cfg),
           config_error_line(&cfg), config_error_text(&cfg));
    config_destroy(&cfg);
    return 0;
  }

  if (!config_read_globals()) {
    return 0;
  }

  if (!config_read_publisher_types()) {
    return 0;
  }

  if (!subscribe()) {
    return 0;
  }

  return 1;
}

void rf_gw_loop()
{
  while (1) {
    sleep(1);
  }
}
