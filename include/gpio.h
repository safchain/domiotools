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

#ifndef __GPIO_H
#define __GPIO_H

#define MAX_GPIO    64

#define GPIO_LOW    0
#define GPIO_HIGH   1

#define GPIO_OUT    "out"
#define GPIO_IN     "in"

#define GPIO_EDGE_FALLING    "falling"
#define GPIO_EDGE_RISING     "rising"
#define GPIO_EDGE_BOTH       "both"

void gpio_set_syspath(const char *path);
int gpio_export(unsigned int gpio);
int gpio_direction(unsigned int gpio, const char *direction);
int gpio_edge_detection(unsigned int gpio, const char *edge);
int gpio_open(unsigned int gpio, const char *direction);
int gpio_write_fd(unsigned int fd, char value);
int gpio_write(unsigned int gpio, char value);
char gpio_read_fd(unsigned int fd);
char gpio_read(unsigned int gpio);
unsigned long gpio_time();
void gpio_usleep(unsigned int usec);
int gpio_sched_priority(int priority);
void gpio_close(unsigned int gpio);
int gpio_is_opened(unsigned int gpio);
void gpio_event_stop();
int gpio_event_loop(int loop);
int gpio_event_add(unsigned int gpio,
        void (*callback)(unsigned int gpio, void *data), void *data);
int gpio_event_init();

#endif
