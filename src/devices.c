/*
 * Copyright (C) 2015 Sylvain Afchain
 *
 * This program is free software; you can redistribute it and/or modify it under the terms of the
 * GNU General Public License as published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without
 / even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along with this program; if
 * not, write to the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
 * 02110-1301, USA.
 */

#include <string.h>

#include "devices.h"

int device_from_str(const char *str)
{
  if (strcmp(str, "srts") == 0) {
    return SRTS;
  }
  if (strcmp(str, "homeasy") == 0) {
    return HOMEASY;
  }

  return 0;
}

const char *device_from_int(int i) {
  switch(i) {
      case SRTS:
          return "srts";
      case HOMEASY:
          return "homeasy";
  }

  return "";
}
