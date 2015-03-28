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
#include "gpio.h"

extern int verbose;
extern int debug;

struct dlog *DLOG;

int main(int argc, char **argv)
{
  if (setuid(0)) {
    perror("setuid");
    return -1;
  }

  srand(time(NULL));

  DLOG = dlog_init(DLOG_STDERR, DLOG_DEBUG, "rf_gateway");
  assert(DLOG != NULL);

  mqtt_init();

  if (!rf_gw_init("rules.cfg", 1)) {
    return -1;
  }

  verbose = 1;
  debug = 1;

  rf_gw_loop(-1);

  return 0;
}
