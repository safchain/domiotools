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

#ifndef __SRTS_H
#define __SRTS_H

enum COMMAND {
  UNKNOWN = 0,
  MY = 1,
  UP,
  MY_UP,
  DOWN,
  MY_DOWN,
  UP_DOWN,
  PROG = 8,
  SUN_FLAG,
  FLAG,
};

struct srts_payload {
  unsigned char key;
  unsigned char checksum:4;
  unsigned char ctrl:4;
  unsigned short code;
  struct address {
    unsigned char byte1;
    unsigned char byte2;
    unsigned char byte3;
  } address;
};

void srts_transmit(int gpio, unsigned char key, unsigned short address,
        unsigned char command, unsigned short code, int repeated);
void srts_transmit_persist(int gpio, char key, unsigned short address,
        unsigned char command, int repeated, const char *persist_path);
int srts_receive(int type, int duration, struct srts_payload *payload);
int srts_get_address(struct srts_payload *payload);
void srts_print_payload(struct srts_payload *payload);
const char *srts_get_ctrl_str(struct srts_payload *payload);

#endif
