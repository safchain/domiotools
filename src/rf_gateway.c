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
#include <ev.h>

#include "common.h"
#include "mem.h"
#include "srts.h"
#include "hl.h"
#include "urlparser.h"
#include "mqtt.h"
#include "logging.h"
#include "homeasy.h"
#include "gpio.h"

enum {
  SRTS = 1,
  HOMEASY
};

struct rf_device {
  unsigned int gpio;
  unsigned int type;
  unsigned int address;
  unsigned int repeat;
  config_setting_t *config_h;
};

struct rf_publisher {
  unsigned int gpio;
  ev_io watcher;
};

extern struct dlog *DLOG;

static config_t rf_cfg;
static int rf_publisher_types[MAX_GPIO + 1];
static char *rf_persistence_path = "/var/lib/";
static LIST *rf_subscribers = NULL;
static LIST *rf_publishers = NULL;
static struct ev_loop *rf_loop = NULL;
static int rf_running_loop = 0;

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

static int config_lookup_type(config_setting_t *h, const char *type)
{
  const char *t;

  if (config_setting_lookup_string(h, "type", &t) != CONFIG_TRUE) {
    dlog(DLOG, DLOG_ERR, "No address defined for the publisher line: %d",
         config_setting_source_line(h));
    return 0;
  }
  if (strcmp(t, type) != 0) {
    return 0;
  }
  return 1;
}

static int srts_lookup_for_publisher(struct srts_payload *payload)
{
  config_setting_t *hs, *h;
  unsigned int address, value, i = 0;
  const char *ctrl;

  hs = config_lookup(&rf_cfg, "config.publishers");
  if (hs == NULL) {
    dlog(DLOG, DLOG_ERR, "No publisher defined");
    return 0;
  }

  do {
    h = config_setting_get_elem(hs, i);
    if (h != NULL) {
      if (!config_lookup_type(h, "srts")) {
        continue;
      }

      if (config_setting_lookup_int(h, "address",
                  (int *) &value) != CONFIG_TRUE) {
        dlog(DLOG, DLOG_ERR, "No address defined for the publisher line: %d",
                config_setting_source_line(h));
        continue;
      }
      address = srts_get_address(payload);
      if (address == value) {
        ctrl = srts_get_ctrl_str(payload);
        if (ctrl == NULL) {
          dlog(DLOG, DLOG_ERR, "Somfy RTS, ctrl unknown: %d", payload->ctrl);
          return 0;
        }

        return publish(h, ctrl);
      }
    }
    i++;
  } while (h != NULL);

  return 1;
}

static int homeasy_lookup_for_publisher(struct homeasy_payload *payload)
{
  config_setting_t *hs, *h;
  unsigned int value, i = 0;
  const char *ctrl;

  hs = config_lookup(&rf_cfg, "config.publishers");
  if (hs == NULL) {
    dlog(DLOG, DLOG_ERR, "No publisher defined");
    return 0;
  }

  do {
    h = config_setting_get_elem(hs, i);
    if (h != NULL) {
      if (!config_lookup_type(h, "homeasy")) {
        continue;
      }

      if (config_setting_lookup_int(h, "address",
                  (int *) &value) != CONFIG_TRUE) {
        dlog(DLOG, DLOG_ERR, "No address defined for the publisher line: %d",
                config_setting_source_line(h));
        continue;
      }

      if (payload->address == value) {
        ctrl = homeasy_get_ctrl_str(payload);
        if (ctrl == NULL) {
          dlog(DLOG, DLOG_ERR, "Homeasy, ctrl unknown: %d", payload->ctrl);
          return 0;
        }

        return publish(h, ctrl);
      }
    }
    i++;
  } while (h != NULL);

  return 1;
}

static int homeasy_handler(unsigned int gpio, unsigned int type, int duration)
{
  struct homeasy_payload payload;
  int rc;

  rc = homeasy_receive(gpio, type, duration, &payload);
  if (rc == 1) {
    dlog(DLOG, DLOG_INFO, "Homeasy, Message received correctly");

    rc = homeasy_lookup_for_publisher(&payload);
  }
  return rc;
}

static int srts_handler(unsigned int gpio, unsigned int type, int duration)
{
  struct srts_payload payload;
  static int last_key = 0;
  int rc;

  rc = srts_receive(gpio, type, duration, &payload);
  if (rc == 1) {
    if (last_key && payload.key == last_key) {
      return 0;
    }
    last_key = payload.key;

    dlog(DLOG, DLOG_INFO, "Somfy RTS, Message received correctly");

    rc = srts_lookup_for_publisher(&payload);
  }
  return rc;
}

void rf_gw_handle_interrupt(unsigned int gpio, unsigned int type, long time)
{
  static unsigned int last_change = 0;
  static unsigned int total_duration = 0;
  unsigned int duration = 0;
  int rc = 0;

  if (last_change) {
    duration = time - last_change;
  }
  last_change = time;

  /* simple noise reduction */
  total_duration += duration;
  if (total_duration > 50) {

    /* ask for all defined publisher */
    if (rf_publisher_types[gpio] & SRTS) {
      rc = srts_handler(gpio, type, total_duration);
    }
    if (rc != 1 && (rf_publisher_types[gpio] & HOMEASY)) {
      homeasy_handler(gpio, type, total_duration);
    }
    total_duration = 0;
  }
}

static unsigned int str_type_to_int(const char *type)
{
  if (strcmp(type, "srts") == 0) {
    return SRTS;
  } else if (strcmp(type, "homeasy") == 0) {
    return HOMEASY;
  }

  return 0;
}

static int add_publisher_type(unsigned int gpio, const char *type)
{
  unsigned int t = str_type_to_int(type);
  if (!t) {
    dlog(DLOG, DLOG_ERR, "Publisher type unknown: %s", type);
    return 0;
  }
  rf_publisher_types[gpio] |= t;

  return 1;
}

static void rf_mqtt_callback(void *obj, const void *payload, int payloadlen)
{
  int ctrl;
  struct rf_device *device = (struct rf_device *) obj;
  char *value = xmalloc(payloadlen + 1);

  memset(value, 0, payloadlen + 1);
  memcpy(value, payload, payloadlen);

  switch(device->type) {
    case SRTS:
      ctrl = srts_get_ctrl_int(value);
      if (ctrl == SRTS_UNKNOWN) {
        goto clean;
      }

      srts_transmit_persist(device->gpio, 0, device->address, ctrl,
              device->repeat, rf_persistence_path);
      break;
    case HOMEASY:
      ctrl = homeasy_get_ctrl_int(value);
      if (ctrl == HOMEASY_UNKNOWN) {
        goto clean;
      }

      homeasy_transmit(device->gpio, device->address, 0, ctrl, 0,
              device->repeat);
      break;
  }

clean:
  free(value);
}

static void gpio_cb(EV_P_ struct ev_io *io, int revents)
{
  struct rf_publisher *publisher;
  long time;
  int type;

  type = gpio_read_fd(io->fd);
  if (type == GPIO_LOW) {
    type = GPIO_HIGH;
  } else {
    type = GPIO_LOW;
  }

  time = gpio_time();
  publisher = (struct rf_publisher *) io->data;
  rf_gw_handle_interrupt(publisher->gpio, type, time);
}

/* MQTT -> RF */
static int start_subscribers()
{
  config_setting_t *hs, *h;
  struct rf_device *device = NULL;
  char *input, *type;
  unsigned int gpio, address, repeat = 0;
  unsigned int t, i = 0;

  hs = config_lookup(&rf_cfg, "config.subscribers");
  if (hs == NULL) {
    dlog(DLOG, DLOG_INFO, "No subscriber defined");
    return 1;
  }

  rf_subscribers = hl_list_alloc();

  do {
    h = config_setting_get_elem(hs, i);
    if (h != NULL) {
      if (!config_setting_lookup_string(h, "input", (const char **) &input)) {
        dlog(DLOG, DLOG_ERR, "No type defined for the subscriber line: %d",
                config_setting_source_line(h));
        return 0;
      }
      if (!config_setting_lookup_int(h, "gpio", (int *) &gpio)) {
        dlog(DLOG, DLOG_ERR, "No GPIO defined for the publisher line: %d",
                config_setting_source_line(h));
        return 0;
      }
      if (!config_setting_lookup_string(h, "type", (const char **) &type)) {
          dlog(DLOG, DLOG_ERR, "No type defined for the subscriber line: %d",
                config_setting_source_line(h));
        return 0;
      }
      t = str_type_to_int(type);
      if (!t) {
        dlog(DLOG, DLOG_ERR, "Subscriber type unknown: %s", type);
        return 0;
      }
      if (!config_setting_lookup_int(h, "address", (int *) &address)) {
        dlog(DLOG, DLOG_ERR, "No address defined for the subscriber line: %d",
                config_setting_source_line(h));
        return 0;
      }
      config_setting_lookup_int(h, "repeat", (int *) &repeat);

      device = xmalloc(sizeof(struct rf_device));
      device->gpio = gpio;
      device->address = address;
      device->type = t;
      device->repeat = repeat;
      device->config_h = h;

      if (gpio_open(gpio, GPIO_OUT) == -1) {
        dlog(DLOG, DLOG_ERR, "Unable to open the GPIO: %d", gpio);
        goto clean;
      }

      if (!mqtt_subscribe(input, device, rf_mqtt_callback)) {
        goto clean;
      }

      hl_list_push(rf_subscribers,  &device, sizeof(struct rf_device *));
    }
    i++;
  } while (h != NULL);

  return 1;

clean:
  if (device) {
    free(device);
  }

  return 0;
}

static int config_read_publishers()
{
  config_setting_t *hs, *h;
  char *type, *output;
  unsigned int gpio, address, i = 0;

  hs = config_lookup(&rf_cfg, "config.publishers");
  if (hs == NULL) {
    dlog(DLOG, DLOG_INFO, "No publisher defined");
    return 1;
  }

  do {
    h = config_setting_get_elem(hs, i);
    if (h != NULL) {
      if (!config_setting_lookup_int(h, "gpio", (int *) &gpio)) {
        dlog(DLOG, DLOG_ERR, "No GPIO defined for the publisher line: %d",
                config_setting_source_line(h));
        return 0;
      }
      if (gpio > MAX_GPIO) {
        dlog(DLOG, DLOG_ERR, "Unsupported GPIO for the publisher line: %d,"
                "maximum supported %d", config_setting_source_line(h),
                MAX_GPIO);
        return 0;
      }
      if (!config_setting_lookup_string(h, "type", (const char **) &type)) {
          dlog(DLOG, DLOG_ERR, "No type defined for the publisher line: %d",
                config_setting_source_line(h));
        return 0;
      }
      if (!config_setting_lookup_string(h, "output",
                  (const char **) &output)) {
        dlog(DLOG, DLOG_ERR, "No output defined for the publisher line: %d",
                config_setting_source_line(h));
        return 0;
      }
      if (!config_setting_lookup_int(h, "address", (int *) &address)) {
        dlog(DLOG, DLOG_ERR, "No address defined for the publisher line: %d",
                config_setting_source_line(h));
        return 0;
      }

      if (!add_publisher_type(gpio, type)) {
        return 0;
      }
    }
    i++;
  } while (h != NULL);

  return 1;
}

/* RF -> MQTT */
static int start_publishers()
{
  struct rf_publisher publisher, *ptr;
  unsigned int gpio, fd;
  ev_io *watcher;

  rf_publishers = hl_list_alloc();

  if (!config_read_publishers()) {
    return 0;
  }

  for (gpio = 0; gpio != MAX_GPIO; gpio++) {
    if (rf_publisher_types[gpio]) {
      if (!gpio_export(gpio)) {
          dlog(DLOG, DLOG_ERR, "Unable to export the GPIO: %d", gpio);
          return 0;
      }

      if (!gpio_edge_detection(gpio, GPIO_EDGE_BOTH)) {
        dlog(DLOG, DLOG_ERR,
                "Unable to set the edge detection mode for the GPIO: %d",
                gpio);
        return 0;
      }

      fd = gpio_open(gpio, GPIO_IN);
      if (fd == -1) {
        dlog(DLOG, DLOG_ERR,
                "Unable to open GPIO pin: %d, %s", gpio, strerror(errno));
        return 0;
      }
      ptr = hl_list_push(rf_publishers, &publisher,
              sizeof(struct rf_publisher));

      ptr->gpio = gpio;

      watcher = &(ptr->watcher);
      ev_io_init(watcher, gpio_cb, fd, EV_READ);
      ev_io_start(rf_loop, watcher);

      ptr->watcher.data = ptr;
    }
  }

  return 1;
}

static int config_read_globals()
{
  int rc;

  rc = config_lookup_string(&rf_cfg, "config.globals.persistence_path",
                            (const char **) &rf_persistence_path);
  if (rc == CONFIG_FALSE) {
    dlog(DLOG, DLOG_WARNING, "No persistence path defined, using the "
         "default value: %s", rf_persistence_path);
  }

  return 1;
}

int rf_gw_init(char *in, int file)
{
  int rc;

  config_init(&rf_cfg);

  if (file) {
    if (in == NULL || access(in, R_OK) != 0) {
      dlog(DLOG, DLOG_CRIT, "Configuration file not readable: %s", in);
      return 0;
    }

    rc = config_read_file(&rf_cfg, in);
  } else {
    rc = config_read_string(&rf_cfg, in);
  }

  if (!rc) {
    dlog(DLOG, DLOG_CRIT, "Configuration file malformed, %s:%d - %s",
         config_error_file(&rf_cfg), config_error_line(&rf_cfg),
         config_error_text(&rf_cfg));
    config_destroy(&rf_cfg);
    return 0;
  }

  rf_loop = ev_loop_new(EVFLAG_AUTO);
  if (rf_loop == NULL) {
    dlog(DLOG, DLOG_CRIT, "could not initialise libev !");
    config_destroy(&rf_cfg);
    return 0;
  }

  if (!config_read_globals()) {
    return 0;
  }

  if (!config_read_publishers()) {
    return 0;
  }

  if (!start_subscribers()) {
    return 0;
  }

  if (!start_publishers()) {
    return 0;
  }

  return 1;
}

void rf_gw_loop(int loop)
{
  rf_running_loop = loop;
  while (rf_running_loop) {
    ev_run (rf_loop, EVRUN_ONCE);
    if (rf_running_loop > 0) {
      rf_running_loop--;
    }
  }
}

void rf_gw_stop()
{
  rf_running_loop = 0;
}

void rf_gw_destroy() {
  LIST_ITERATOR iterator;
  struct rf_device **device;

  if (rf_subscribers != NULL) {
    hl_list_init_iterator(rf_subscribers, &iterator);
    while((device = hl_list_iterate(&iterator)) != NULL) {
      free(*device);
    }
  }
}
