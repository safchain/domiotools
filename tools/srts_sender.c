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
#include <syslog.h>
#include <getopt.h>
#include <errno.h>
#include <arpa/inet.h>
#include <errno.h>
#include <limits.h>
#include <unistd.h>
#include <sys/types.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <libgen.h>
#include <assert.h>

#include "common.h"
#include "srts.h"
#include "logging.h"

struct dlog *DLOG;

static unsigned short get_next_code(char *progname, unsigned short address)
{
  char *path, code[10];
  FILE *fp;
  int size;

  size = snprintf(NULL, 0, "/var/lib/%s/%d", progname, address);
  if ((path = (char *) malloc(size + 1)) == NULL) {
    fprintf(stderr, "Memory allocation error\n");
    exit(-1);
  }
  sprintf(path, "/var/lib/%s", progname);
  if (mkpath(path, 0755) == -1) {
    fprintf(stderr, "Unable to create the state path: %s\n", path);
    exit(-1);
  }
  sprintf(path, "/var/lib/%s/%d", progname, address);

  if ((fp = fopen(path, "r")) == NULL) {
    return 1;
  }

  memset(code, 0, sizeof(code));
  if (fgets(code, sizeof(code), fp) == NULL) {
    fclose(fp);
    return 1;
  }
  fclose(fp);

  return ((unsigned short) atoi(code)) + 1;
}

static void store_code(char *progname, unsigned short address,
                       unsigned short new_code)
{
  char *path, code[10];
  FILE *fp;
  int size;

  size = snprintf(NULL, 0, "/var/lib/%s/%d", progname, address);
  if ((path = (char *) malloc(size + 1)) == NULL) {
    fprintf(stderr, "Memory allocation error\n");
    exit(-1);
  }
  sprintf(path, "/var/lib/%s", progname);
  if (mkpath(path, 0755) == -1) {
    fprintf(stderr, "Unable to create the state path: %s\n", path);
    exit(-1);
  }
  sprintf(path, "/var/lib/%s/%d", progname, address);

  if ((fp = fopen(path, "w+")) == NULL) {
    fprintf(stderr, "Unable to open the state file: %s", path);
    exit(-1);
  }

  sprintf(code, "%d\n", new_code);
  if (fputs(code, fp) < 0) {
    fclose(fp);
    exit(-1);
  }

  fclose(fp);
}

static void usage(char *name)
{
  printf
    ("Usage: %s --gpio <gpio pin> --address <remote address> --comand <command>\n",
     name);
  exit(-1);
}

int main(int argc, char **argv)
{
  struct option long_options[] = { {"gpio", 1, 0, 0},
  {"address", 1, 0, 0}, {"command", 1, 0, 0}, {NULL, 0, 0, 0}
  };
  unsigned char key;
  unsigned short address = 0;
  unsigned short code = 0;
  long int a2i;
  int gpio = -1, i, c;
  char command = SRTS_UNKNOWN;
  char *progname, *end;

  if (setuid(0)) {
    perror("setuid");
    return -1;
  }

  while (1) {
    c = getopt_long(argc, argv, "", long_options, &i);
    if (c == -1)
      break;
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
        } else if (strcmp(long_options[i].name, "command") == 0) {
          command = srts_get_ctrl_int(optarg);
        }
        break;
      default:
        usage(argv[0]);
    }
  }

  if (command == SRTS_UNKNOWN || address == 0 || gpio == -1) {
    usage(argv[0]);
  }
  // store pid and lock it
  store_pid();

  srand(time(NULL));
  key = rand() % 255;

  if (wiringPiSetup() == -1) {
    fprintf(stderr, "Wiring Pi not installed");
    return -1;
  }

  DLOG = dlog_init(DLOG_NULL, DLOG_INFO, NULL);
  assert(DLOG != NULL);

  openlog("srts", LOG_PID | LOG_CONS, LOG_USER);

  progname = basename(argv[0]);

  code = get_next_code(progname, address);
  syslog(LOG_INFO, "remote: %d, command: %d, code: %d\n", address, command,
         code);
  closelog();

  piHiPri(99);
  pinMode(gpio, OUTPUT);
  srts_transmit(gpio, key, address, command, code, 0);

  c = 7;
  if (command == SRTS_PROG) {
    c = 20;
  }

  for (i = 0; i < c; i++) {
    srts_transmit(gpio, key, address, command, code, 1);
  }
  store_code(progname, address, code);

  return 0;
}
