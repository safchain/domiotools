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
#include <arpa/inet.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <limits.h>

#include "common.h"
#include "srts.h"
#include "mem.h"
#include "logging.h"

extern struct dlog *DLOG;

static char *get_code_file_path(const char *persistence_path,
        unsigned short address)
{
  char *path;
  int size;

  size = snprintf(NULL, 0, "%s/srts/%d", persistence_path, address);
  path = (char *) xmalloc(size + 1);

  sprintf(path, "%s/srts", persistence_path);
  if (mkpath(path, 0755) == -1) {
    dlog(DLOG, DLOG_ERR, "Unable to create the srts code path: %s", path);
    free(path);

    return NULL;
  }
  sprintf(path, "%s/srts/%d", persistence_path, address);

  return path;
}

static int get_code(const char *persistence_path,
        unsigned short address)
{
  char *path, code[10], *end;
  FILE *fp = NULL;
  int c, rc;

  path = get_code_file_path(persistence_path, address);
  if (path == NULL) {
    return 0;
  }

  if ((fp = fopen(path, "r")) == NULL) {
    rc = 0;
    goto clean;
  }

  memset(code, 0, sizeof(code));
  if (fgets(code, sizeof(code), fp) == NULL) {
    rc = -1;
    goto clean;
  }

  c = strtol(code, &end, 10);
  if (errno == ERANGE && (c == LONG_MAX || c == LONG_MIN)) {
    dlog(DLOG, DLOG_ERR, "Unable to parse the srts code for the address %d",
            address);
    rc = -1;
    goto clean;
  }
  fclose(fp);

  return c;

clean:
  if (fp != NULL) {
    fclose(fp);
  }
  free(path);
  return rc;
}

static int store_code(const char *persistence_path, unsigned short address,
        unsigned short new_code)
{
  char *path, code[10];
  FILE *fp;

  path = get_code_file_path(persistence_path, address);
  if (path == NULL) {
    return -1;
  }

  if ((fp = fopen(path, "w+")) == NULL) {
    dlog(DLOG, DLOG_ERR, "Unable to create the srts code file: %s", path);
    return -1;
  }

  sprintf(code, "%d\n", new_code);
  if (fputs(code, fp) < 0) {
    dlog(DLOG, DLOG_ERR, "Unable to write the srts code to the file: %s",
            path);
  }

  fclose(fp);

  return 1;
}

static void obfuscate_payload(struct srts_payload *payload)
{
  unsigned char *p = (unsigned char *) payload;
  int i = 0;

  for (i = 1; i < 7; i++) {
    p[i] = p[i] ^ p[i - 1];
  }
}

static unsigned char get_checksum(struct srts_payload *payload) {
  unsigned char *p = (unsigned char *) payload;
  unsigned char checksum = 0;
  int i;

  for (i = 0; i < 7; i++) {
    checksum = checksum ^ p[i] ^ (p[i] >> 4);
  }
  return checksum & 0xf;
}

static void checksum_payload(struct srts_payload *payload)
{
  payload->checksum = get_checksum(payload);
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
        unsigned char ctrl, unsigned short code, int repeated)
{
  struct srts_payload payload;

  sync_transmit(gpio, repeated);

  digitalWrite(gpio, HIGH);
  delayMicroseconds(4800);
  digitalWrite(gpio, LOW);
  delayMicroseconds(660);

  if (!key) {
    key = rand() % 255;
  }
  payload.key = key;
  payload.ctrl = ctrl;
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

void srts_transmit_persist(int gpio, char key, unsigned short address,
        unsigned char ctrl, int repeat, const char *persistence_path)
{
  int i, code = get_code(persistence_path, address);

  if (code == -1) {
    /* reading code error, defaulting to 1 */
    srts_transmit(gpio, key, address, ctrl, 1, 0);
    for (i = 0; i < repeat; i++) {
      srts_transmit(gpio, key, address, ctrl, 1, 1);
    }

    return;
  }

  code += 1;

  srts_transmit(gpio, key, address, ctrl, code, 0);
  for (i = 0; i < repeat; i++) {
    srts_transmit(gpio, key, address, ctrl, code, 1);
  }

  store_code(persistence_path, address, code);
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
  unsigned char payload_chk = payload->checksum;
  unsigned char checksum = 0;

  payload->checksum = 0;
  checksum = get_checksum(payload);

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
  } else if (soft_sync == 1 && *duration >= 660) {
    *duration -= 800;
    soft_sync = 2;

    dlog(DLOG, DLOG_DEBUG, "Somfy RTS, Found the sync part of a message");

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

unsigned char srts_get_ctrl_int(const char *ctrl)
{
  if (strcasecmp(ctrl, "my") == 0) {
    return MY;
  } else if (strcasecmp(ctrl, "up") == 0) {
    return UP;
  } else if (strcasecmp(ctrl, "my_up") == 0) {
    return MY_UP;
  } else if (strcasecmp(ctrl, "down") == 0) {
    return DOWN;
  } else if (strcasecmp(ctrl, "my_down") == 0) {
    return MY_DOWN;
  } else if (strcasecmp(ctrl, "up_down") == 0) {
    return UP_DOWN;
  } else if (strcasecmp(ctrl, "prog") == 0) {
    return PROG;
  } else if (strcasecmp(ctrl, "sun_flag") == 0) {
    return SUN_FLAG;
  } else if (strcasecmp(ctrl, "flag") == 0) {
    return FLAG;
  }

  return UNKNOWN;
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
      return 0;
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
      sync = 0;
      index = 0;

      return -1;
    } else if (rc == 1) {
      rc = read_byte(bit, bytes + index);
      if (rc) {
        if (++index == 7) {
          sync = 0;
          index = 0;

          unfuscate_payload(bytes, payload);
          rc = validate_checksum(payload);
          if (rc == 0) {
            dlog(DLOG, DLOG_DEBUG, "Somfy RST, Checksum error");

            return -1;
          }
          payload->code = htons(payload->code);

          return rc;
        }
      }
    }
  }

  return 0;
}
