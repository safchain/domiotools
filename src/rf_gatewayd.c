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
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <limits.h>
#include <stdio.h>
#include <time.h>
#include <assert.h>

#include "common.h"
#include "srts.h"
#include "hl.h"
#include "urlparser.h"
#include "mqtt.h"
#include "rf_gateway.h"
#include "logging.h"

extern int verbose;
extern int debug;

struct dlog *DLOG;

void handle_interrupt()
{
  long time;
  int type;

  type = digitalRead(2);
  if (type == LOW) {
    type = HIGH;
  } else {
    type = LOW;
  }

  time = micros();
  rf_gw_handle_interrupt(type, time);
}

int main(int argc, char **argv)
{
  int gpio = 2;

  if (setuid(0)) {
    perror("setuid");
    return -1;
  }

  if (!rf_gw_read_config("rules.cfg", 1)) {
    return -1;
  }

  if (wiringPiSetup() == -1) {
    fprintf(stderr, "Wiring Pi not installed");
    return -1;
  }

  DLOG = dlog_init(DLOG_SYSLOG, DLOG_DEBUG, "rf_gateway");
  assert(DLOG != NULL);

  srand(time(NULL));

  mqtt_init();

  verbose = 1;
  debug = 1;

  piHiPri(99);
  pinMode(gpio, INPUT);
  wiringPiISR(gpio, INT_EDGE_BOTH, handle_interrupt);

  rf_gw_loop();

  return 0;
}
