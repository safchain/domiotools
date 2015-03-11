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

#define GPIO_LOW    0
#define GPIO_HIGH   1

void gpio_set_syspath(const char *path);
int gpio_export(unsigned int gpio);
int gpio_direction(unsigned int gpio, const char *direction);
int gpio_edge_detection(unsigned int gpio, const char *edge);
int gpio_open(unsigned int gpio);
unsigned long gpio_time();

#endif
