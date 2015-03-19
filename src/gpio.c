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
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/time.h>
#include <unistd.h>
#include <time.h>

#include "gpio.h"
#include "mem.h"

static const char *syspath = "/sys/class/gpio";
static int fds[MAX_GPIO + 1];

void gpio_set_syspath(const char *path)
{
  syspath = path;
}

int gpio_export(unsigned int gpio)
{
  FILE *fp;
  char *filename;
  int len;

  len = snprintf(NULL, 0, "%s/export", syspath);
  filename = xcalloc(1, len + 1);

  sprintf(filename, "%s/export", syspath);

  fp = fopen(filename, "w");
  if (fp == NULL) {
    goto clean;
  }

  fprintf(fp, "%d\n", gpio);
  fclose(fp);

  free(filename);

  return 1;

clean:
  free(filename);

  return 0;
}

static int gpio_write_setting(unsigned int gpio, const char *setting,
        const char *value)
{
  FILE *fp;
  char *filename;
  int len;

  len = snprintf(NULL, 0, "%s/gpio%d/%s", syspath, gpio, setting);
  filename = xcalloc(1, len + 1);

  sprintf(filename, "%s/gpio%d/%s", syspath, gpio, setting);

  fp = fopen(filename, "w");
  if (fp == NULL) {
    goto clean;
  }

  fprintf(fp, "%s\n", value);
  fclose(fp);

  free(filename);

  return 1;

clean:
  free(filename);

  return 0;
}

int gpio_direction(unsigned int gpio, const char *direction)
{
  return gpio_write_setting(gpio, "direction", direction);
}

int gpio_edge_detection(unsigned int gpio, const char *edge)
{
  return gpio_write_setting(gpio, "edge", edge);
}

int gpio_open(unsigned int gpio)
{
  char *filename;
  int fd, len;

  len = snprintf(NULL, 0, "%s/gpio%d/value", syspath, gpio);
  filename = xcalloc(1, len + 1);

  sprintf(filename, "%s/gpio%d/value", syspath, gpio);

  fd = open(filename, O_RDWR);

  free(filename);

  fds[gpio] = fd;

  return fd;
}

int gpio_write(unsigned int gpio, char value)
{
  return write(fds[gpio], &value, sizeof(char));
}

char gpio_read(unsigned int gpio)
{
  char value;

  if (read(fds[gpio], &value, sizeof(unsigned char)) > 0) {
    return value;
  }

  return -1;
}

void gpio_close(unsigned int gpio)
{
  close(fds[gpio]);
}

unsigned long gpio_time()
{
  struct timeval tv;
  unsigned long now;

  gettimeofday(&tv, NULL);
  now = tv.tv_sec * 1000000 + tv.tv_usec;

  return now;
}

void gpio_usleep(unsigned int usec)
{
  struct timespec ttime, curtime;
  unsigned int nsec = usec * 1000;

  if (usec > 500) {
    ttime.tv_sec = 0;
    ttime.tv_nsec = nsec;
    nanosleep(&ttime, NULL);
  } else {
    clock_gettime(CLOCK_REALTIME, &ttime);

    nsec += ttime.tv_nsec;
    if (nsec > 999999999) {
      ttime.tv_sec += 1;
      ttime.tv_nsec = nsec - 999999999;
    } else {
      ttime.tv_nsec = nsec;
    }

    while(1) {
      clock_gettime(CLOCK_REALTIME, &curtime);
      if (curtime.tv_sec == ttime.tv_sec && curtime.tv_nsec > ttime.tv_nsec) {
        break;
      }
    }
  }
}