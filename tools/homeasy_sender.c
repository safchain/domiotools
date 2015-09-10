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

#include <stdio.h>
#include <syslog.h>
#include <getopt.h>
#include <sys/types.h>
#include <limits.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdlib.h>
#include <assert.h>

#include "common.h"
#include "homeasy.h"
#include "logging.h"
#include "gpio.h"

struct dlog *DLOG;

static void usage(char *name)
{
  fprintf(stderr, "Usage: %s --gpio <gpio pin> --address <remote address> "
          "--command <command> --repeat <number>\n", name);
  exit(-1);
}

int main(int argc, char **argv)
{
  struct option long_options[] = { {"gpio", 1, 0, 0},
    {"address", 1, 0, 0}, {"command", 1, 0, 0}, {"receiver", 1, 0, 0},
    {"repeat", 1, 0, 0}, {NULL, 0, 0, 0}
  };
  unsigned int address = 0;
  unsigned char receiver = 0;
  long int a2i;
  int gpio = -1;
  char command = HOMEASY_UNKNOWN;
  char *end;
  int repeat = 5, i, c;

  if (setuid(0)) {
    perror("setuid");
    return -1;
  }

  while (1) {
    c = getopt_long(argc, argv, "", long_options, &i);
    if (c == -1) {
      break;
    }
    switch (c) {
      case 0:
        if (strcmp(long_options[i].name, "gpio") == 0) {
          a2i = strtol(optarg, &end, 10);
          if (errno == ERANGE && (a2i == LONG_MAX || a2i == LONG_MIN)) {
            fprintf(stderr, "Unable to parse the gpio parameter\n");
            break;
          }
          gpio = a2i;
        } else if (strcmp(long_options[i].name, "address") == 0) {
          a2i = strtol(optarg, &end, 10);
          if (errno == ERANGE && (a2i == LONG_MAX || a2i == LONG_MIN)) {
            fprintf(stderr, "Unable to parse the address parameter\n");
            break;
          }
          address = a2i;
        } else if (strcmp(long_options[i].name, "receiver") == 0) {
          a2i = strtol(optarg, &end, 10);
          if (errno == ERANGE && (a2i == LONG_MAX || a2i == LONG_MIN)) {
            fprintf(stderr, "Unable to parse the receiver parameter\n");
            break;
          }
          receiver = a2i;
        } else if (strcmp(long_options[i].name, "command") == 0) {
          command = homeasy_get_ctrl_int(optarg);
        } else if (strcmp(long_options[i].name, "repeat") == 0) {
          a2i = strtol(optarg, &end, 10);
          if (errno == ERANGE && (a2i == LONG_MAX || a2i == LONG_MIN)) {
            break;
          }
          repeat = a2i;
        }
        break;
      default:
        usage(argv[0]);
    }
  }

  if (command == HOMEASY_UNKNOWN || address == 0 || gpio == -1) {
    usage(argv[0]);
  }
  // store pid and lock it
  store_pid();

  DLOG = dlog_init(DLOG_NULL, DLOG_INFO, NULL);
  assert(DLOG != NULL);

  openlog("homeasy", LOG_PID | LOG_CONS, LOG_USER);
  syslog(LOG_INFO, "Homeasy, remote: %d, receiver, %d, command: %d\n", address,
         receiver, command);
  closelog();

  gpio_sched_priority(99);

  gpio_open(gpio, GPIO_OUT);

  homeasy_transmit(gpio, address, receiver, command, 0, repeat);

  return 0;
}
