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
#include <getopt.h>
#include <assert.h>

#include "srts.h"
#include "hl.h"
#include "urlparser.h"
#include "mqtt.h"
#include "rf_gateway.h"
#include "logging.h"
#include "gpio.h"

struct dlog *DLOG;

static void usage(char *name)
{
  fprintf(stderr, "Usage: %s --conf <config file>\n",
          name);
  exit(-1);
}

int main(int argc, char **argv)
{
  struct option long_options[] = { {"conf", 1, 0, 0}, {NULL, 0, 0, 0} };
  char *config = NULL;
  int i, c;

  if (setuid(0)) {
    fprintf(stderr, "RF Gateway has to be started as root: %s\n",
      strerror(errno));
    return -1;
  }

  srand(time(NULL));

  while (1) {
    c = getopt_long(argc, argv, "", long_options, &i);
    if (c == -1)
      break;
    switch (c) {
      case 0:
        if (strcmp(long_options[i].name, "conf") == 0) {
          config = strdup(optarg);
        }
        break;
      default:
        usage(argv[0]);
    }
  }

  if (config == NULL || strlen(config) == 0) {
    usage(argv[0]);
  }

  DLOG = dlog_init(DLOG_STDERR, DLOG_DEBUG, "rf_gateway");
  assert(DLOG != NULL);

  if (!rf_gw_init(config, 1)) {
    return -1;
  }

  mqtt_init();

  if (gpio_sched_priority(55) != 0) {
    return -1;
  }

  rf_gw_loop(-1);

  free(config);

  return 0;
}