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
#include <stdio.h>
#include <arpa/inet.h>
#include <string.h>

#include "srts.h"

extern int verbose;

static void obfuscate_payload(struct srts_payload *payload)
{
  unsigned char *p = (unsigned char *) payload;
  int i = 0;

  for (i = 1; i < 7; i++) {
    p[i] = p[i] ^ p[i - 1];
  }
}

static void checksum_payload(struct srts_payload *payload)
{
  unsigned char *p = (unsigned char *) payload;
  unsigned char checksum = 0;
  int i = 0;

  for (i = 0; i < 7; i++) {
    checksum = checksum ^ p[i] ^ (p[i] >> 4);
  }
  checksum = checksum & 0xf;
  payload->checksum = checksum;
}

static void write_bit(int gpio, char bit)
{
  if (bit) {
    digitalWrite(gpio, LOW);
    delayMicroseconds(660);
    digitalWrite(gpio, HIGH);
    delayMicroseconds(660);
  } else {
    digitalWrite(gpio, HIGH);
    delayMicroseconds(660);
    digitalWrite(gpio, LOW);
    delayMicroseconds(660);
  }
}

static void write_byte(int gpio, unsigned char byte)
{
  unsigned int mask;

  for (mask = 0b10000000; mask != 0x0; mask >>= 1) {
    write_bit(gpio, byte & mask);
  }
}

static void write_payload(int gpio, struct srts_payload *payload)
{
  unsigned char *p = (unsigned char *) payload;
  int i;

  for (i = 0; i < 7; i++) {
    write_byte(gpio, p[i]);
  }
}

static void write_interval_gap(int gpio)
{
  digitalWrite(gpio, LOW);
  delayMicroseconds(30400);
}

static void sync_transmit(int gpio, int repeated)
{
  int count, i;

  if (repeated) {
    count = 7;
  } else {
    digitalWrite(gpio, HIGH);
    delayMicroseconds(12400);
    digitalWrite(gpio, LOW);
    delayMicroseconds(80600);
    count = 2;
  }
  for (i = 0; i != count; i++) {
    digitalWrite(gpio, HIGH);
    delayMicroseconds(2560);
    digitalWrite(gpio, LOW);
    delayMicroseconds(2560);
  }
}

void srts_transmit(int gpio, unsigned char key, unsigned short address,
                   unsigned char command, unsigned short code, int repeated)
{
  struct srts_payload payload;

  sync_transmit(gpio, repeated);

  digitalWrite(gpio, HIGH);
  delayMicroseconds(4800);
  digitalWrite(gpio, LOW);
  delayMicroseconds(660);

  payload.key = key;
  payload.ctrl = command;
  payload.checksum = 0;
  payload.code = htons(code);
  payload.address.byte1 = ((char *) &address)[0];
  payload.address.byte2 = ((char *) &address)[1];
  payload.address.byte3 = 0;

  checksum_payload(&payload);
  obfuscate_payload(&payload);

  write_payload(gpio, &payload);
  write_interval_gap(gpio);
}

static void unfuscate_payload(char *bytes, struct srts_payload *payload)
{
  unsigned char *p;
  int i = 0;

  p = (unsigned char *) payload;

  p[0] = bytes[0];
  for (i = 1; i < 7; i++) {
    p[i] = bytes[i] ^ bytes[i - 1];
  }
}

static int validate_checksum(struct srts_payload *payload)
{
  unsigned char *p = (unsigned char *) payload;
  unsigned char payload_chk = payload->checksum;
  unsigned char checksum = 0;
  int i = 0;

  payload->checksum = 0;
  for (i = 0; i < 7; i++) {
    checksum = checksum ^ p[i] ^ (p[i] >> 4);
  }
  checksum = checksum & 0xf;

  payload->checksum = payload_chk;
  if (payload_chk == checksum) {
    return 1;
  }

  return 0;
}

static int is_on_time(int duration, int expected)
{
  int v = expected * 10 / 100;

  return duration > (expected - v) && duration < (expected + v);
}

static int detect_sync(int type, int *duration)
{
  static unsigned int init_sync = 0;
  static unsigned int hard_sync = 0;
  static unsigned int soft_sync = 0;

  if (type && is_on_time(*duration, 12400)) {
    init_sync = 1;
  } else if (!type && init_sync == 1 && is_on_time(*duration, 80600)) {
    init_sync = 2;
    hard_sync = 10;
  } else if (init_sync == 2 && hard_sync != 14 && is_on_time(*duration, 2560)) {
    hard_sync++;
  } else if (hard_sync == 14 && soft_sync == 0 && is_on_time(*duration, 4800)) {
    soft_sync = 1;
  } else if (soft_sync == 1 && *duration > 660) {
    *duration -= 800;
    soft_sync = 2;

    /* full sync, hard and soft */
    if (verbose) {
      fprintf(stderr, "Found the sync part of a message\n");
    }
    return 1;
  } else {
    hard_sync = 0;
    soft_sync = 0;
  }

  return 0;
}

static int read_bit(int type, int *duration, char *bit, int last)
{
  static unsigned int pass = 0;

  /* maximum transmit length for a bit is around 1600 */
  if (!last && *duration > 2000) {
    pass = 0;

    return -1;
  }

  /* duration to low to be a part of only one bit, so split into two parts */
  if (*duration > 1100) {
    *duration /= 2;
  } else {
    *duration = 0;
  }

  /* got the two part of a bit */
  if (pass) {
    *bit = type;
    pass = 0;

    return 1;
  }
  pass++;

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

const char *srts_get_ctrl_str(struct srts_payload *payload)
{
  switch (payload->ctrl) {
    case UNKNOWN:
      return "UNKNOWN";
    case MY:
      return "MY";
    case UP:
      return "UP";
    case DOWN:
      return "DOWN";
    case MY_DOWN:
      return "MY_DOWN";
    case UP_DOWN:
      return "UP_DOWN";
    case PROG:
      return "PROG";
    case SUN_FLAG:
      return "SUN_FLAG";
    case FLAG:
      return "FLAG";
  }

  return NULL;
}

int srts_get_address(struct srts_payload *payload)
{
  int address = 0;
  char *ptr;

  ptr = (char *) &address;
  ptr[0] = payload->address.byte1;
  ptr[1] = payload->address.byte2;
  ptr[2] = payload->address.byte3;

  return address;
}

void srts_print_payload(struct srts_payload *payload)
{
  printf("key: %d\n", payload->key);
  printf("checksum: %d\n", payload->checksum);
  printf("ctrl: %d\n", payload->ctrl);
  printf("code: %d\n", payload->code);
  printf("address 1: %d\n", payload->address.byte1);
  printf("address 2: %d\n", payload->address.byte2);
  printf("address 3: %d\n", payload->address.byte3);
  printf("address: %d\n", srts_get_address(payload));
}

int srts_receive(int type, int duration, struct srts_payload *payload)
{
  static unsigned int sync = 0;
  static unsigned int index = 0;
  static char bytes[7];
  char bit;
  int rc;

  if (!sync) {
    sync = detect_sync(type, &duration);
    if (!sync) {
      return -1;
    }
    memset(bytes, 0, 7);

    /* to short, ignore trailling signal */
    if (duration < 400) {
      return 0;
    }
  }

  while (duration > 0) {
    rc = read_bit(type, &duration, &bit, index == 6);
    if (rc == -1) {
      if (verbose) {
        fprintf(stderr, "Error while reading a bit\n");
      }
      sync = 0;
      index = 0;

      return -1;
    }
    if (rc == 1) {
      rc = read_byte(bit, bytes + index);
      if (rc) {
        if (++index == 7) {
          sync = 0;
          index = 0;

          unfuscate_payload(bytes, payload);
          rc = validate_checksum(payload);
          if (rc == 0 && verbose) {
            fprintf(stderr, "Checksum error\n");
          }

          return rc;
        }
      }
    }
  }

  return 0;
}
