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
#include "srts.h"
#include "hl.h"
#include "urlparser.h"
#include "mqtt.h"

extern int verbose;
extern int debug;

enum {
  SRTS = 1,
  HOMEASY
};

static config_t cfg;
static int publisher_types;

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
    rc = mqtt_publish(output, translate_value(config, value));
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
      printf("Message correctly received\n");
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

  if (!read_publisher_types()) {
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
