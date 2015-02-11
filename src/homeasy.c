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

#include <wiringPi.h>
#include <stdio.h>
#include <sys/types.h>
#include <limits.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdlib.h>

#include "homeasy.h"
#include "logging.h"

extern struct dlog *DLOG;

static void _write_bit(int gpio, char bit)
{
  if (bit) {
    digitalWrite(gpio, HIGH);
    delayMicroseconds(300);
    digitalWrite(gpio, LOW);
    delayMicroseconds(1300);
  } else {
    digitalWrite(gpio, HIGH);
    delayMicroseconds(300);
    digitalWrite(gpio, LOW);
    delayMicroseconds(300);
  }
}

static void write_bit(int gpio, char bit)
{
  if (bit) {
    _write_bit(gpio, 1);
    _write_bit(gpio, 0);
  } else {
    _write_bit(gpio, 0);
    _write_bit(gpio, 1);
  }
}

static void sync_transmit(int gpio)
{
  digitalWrite(gpio, HIGH);
  delayMicroseconds(275);
  digitalWrite(gpio, LOW);
  delayMicroseconds(9900);
  digitalWrite(gpio, HIGH);
  delayMicroseconds(275);
  digitalWrite(gpio, LOW);
  delayMicroseconds(2600);
}

static void write_interval_gap(int gpio)
{
  digitalWrite(gpio, HIGH);
  delayMicroseconds(275);
  digitalWrite(gpio, LOW);
  delay(10);
}

void homeasy_transmit(int gpio, unsigned int address, unsigned char receiver,
        unsigned char ctrl)
{
  unsigned int mask;

  sync_transmit(gpio);

  for (mask = 0x2000000; mask != 0x0; mask >>= 1) {
    if (address & mask) {
      write_bit(gpio, 1);
    } else {
      write_bit(gpio, 0);
    }
  }

  // never grouped
  write_bit(gpio, 0);

  write_bit(gpio, ctrl);

  for (mask = 0b1000; mask != 0x0; mask >>= 1) {
    if (receiver & mask) {
      write_bit(gpio, 1);
    } else {
      write_bit(gpio, 0);
    }
  }

  write_interval_gap(gpio);
}

static int is_on_time(int duration, int expected)
{
  int v = expected * 10 / 100;

  return duration > (expected - v) && duration < (expected + v);
}

static int detect_sync(int type, int *duration)
{
  static unsigned int sync = 0;

  if (!type && sync == 1 && is_on_time(*duration, 9900)) {
    sync = 2;
  } else if (type && sync == 2 && is_on_time(*duration, 275)) {
    sync = 3;
  } else if (!type && sync == 3 && is_on_time(*duration, 2600)) {
    sync = 0;

    dlog(DLOG, DLOG_DEBUG, "Homeasy, found the sync part of a message");

    return 1;
  } else if (type && is_on_time(*duration, 275)) {
    sync = 1;
  } else {
    sync = 0;
  }

  return 0;
}

static int _read_bit(int type, int duration, char *bit)
{
  static unsigned int pass = 0;

  if (pass) {
    pass = 0;
    if (is_on_time(duration, 1300)) {
      *bit = 1;

      return 1;
    } else if (is_on_time(duration, 300)) {
      *bit = 0;

      return 1;
    }
  } else if (is_on_time(duration, 300)) {
    pass++;

    return 0;
  }
  pass = 0;

  return -1;
}

static int read_bit(int type, int duration, char *bit)
{
  static unsigned int pass = 0;
  char b;
  int rc;

  rc = _read_bit(type, duration, &b);
  if (rc == -1) {
    pass = 0;

    return -1;
  } else if (rc == 1) {
    if (pass) {
      if (b == 0) {
        *bit = 1;
      } else {
        *bit = 0;
      }

      pass = 0;

      return 1;
    }
    pass++;
  }

  return 0;
}

static int read_byte(char bit, char *byte)
{
  static char b = 0;
  static char d = 7;

  if (d != 0) {
    b |= bit << d--;

    return 0;
  }
  *byte = b | bit;

  b = 0;
  d = 7;

  return 1;
}

int homeasy_receive(int type, int duration, struct homeasy_payload *payload)
{
  static unsigned int sync = 0;
  static unsigned int index = 3;
  static char bytes[4];
  char bit;
  int rc, i;

  if (!sync) {
    sync = detect_sync(type, &duration);
    if (!sync) {
      return 0;
    }
    memset(bytes, 0, 4);

    return 0;
  }

  rc = read_bit(type, duration, &bit);
  if (rc == -1) {
    sync = 0;
    index = 3;

    return -1;
  } else if (rc == 1) {
    rc = read_byte(bit, bytes + index);
    if (rc) {
      if (index-- == 0) {
        sync = 0;
        index = 3;

        i = *((int *) bytes);
        payload->address = i >> 6;
        payload->group = (i >> 5) & 1;
        payload->ctrl = (i >> 4) & 1;
        payload->receiver = i & 0xF;

        return 1;
      }
    }
  }

  return 0;
}

const char *homeasy_get_ctrl_str(struct homeasy_payload *payload)
{
  switch (payload->ctrl) {
    case UNKNOWN:
      return "UNKNOWN";
    case ON:
      return "ON";
    case OFF:
      return "OFF";
  }

  return NULL;
}

unsigned char homeasy_get_ctrl_int(const char *ctrl)
{
  if (strcasecmp(ctrl, "on") == 0) {
    return ON;
  } else if (strcasecmp(ctrl, "off") == 0) {
    return OFF;
  }

  return UNKNOWN;
}
