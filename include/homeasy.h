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

#ifndef __HOMEASY_H
#define __HOMEASY_H

enum COMMAND {
  HOMEASY_OFF,
  HOMEASY_ON,
  HOMEASY_UNKNOWN
};

struct homeasy_payload {
  unsigned int address;
  unsigned char receiver;
  unsigned char ctrl;
  unsigned char group;
};

void homeasy_transmit(unsigned int gpio, unsigned int address,
        unsigned char receiver, unsigned char ctrl, unsigned char group,
        unsigned int repeat);
unsigned char homeasy_get_ctrl_int(const char *ctrl);
const char *homeasy_get_ctrl_str(struct homeasy_payload *payload);
int homeasy_receive(unsigned int gpio, unsigned int type,
        unsigned int duration, struct homeasy_payload *payload);

#endif
