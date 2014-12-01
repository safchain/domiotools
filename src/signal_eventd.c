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

#include <wiringPi.h>
#include <arpa/inet.h>
#include <libconfig.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>

#include "common.h"
#include "srts.h"

extern int verbose;
extern int debug;

int somfy_handler(int type, int duration) {
    struct srts_payload payload;
    unsigned short addr;
    unsigned char *ptr;
    int rtv;

    rtv = srts_receive(type, duration, &payload);
    if (rtv == 1 && verbose) {
        printf("Message correctly received\n");
        if (debug) {
            printf("key: %d\n", payload.key);
            printf("checksum: %d\n", payload.checksum);
            printf("ctrl: %d\n", payload.ctrl);
            printf("code: %d\n", payload.code);
            printf("address 1: %d\n", payload.address.byte1);
            printf("address 2: %d\n", payload.address.byte2);
            printf("address 3: %d\n", payload.address.byte3);

            ptr = (unsigned char *)&addr;
            ptr[0] = payload.address.byte1;
            ptr[1] = payload.address.byte2;

            printf("address: %d\n", addr);
        }
    }
    return rtv;
}

void handle_interrupt() {
    static unsigned int last_change = 0;
    static unsigned int total_duration = 0;
    unsigned int duration = 0;
    long time;
    int type;

    type = digitalRead (2);
    if (type == LOW) {
        type = HIGH;
    } else {
        type = LOW;
    }

    time = micros();
    if (last_change) {
        duration = time - last_change;
    } else {
        last_change = time;
        return;
    }
    last_change = time;

    /* simple noise reduction */
    total_duration += duration;
    if (duration > 200) {
        somfy_handler(type, total_duration);
        total_duration = 0;
    }
}

int main(int argc, char **argv) {
    int gpio = 2;

    if (setuid(0)) {
        perror("setuid");
        return -1;
    }

    if (wiringPiSetup() == -1) {
        fprintf(stderr, "Wiring Pi not installed");
        return -1;
    }

    verbose = 1;

    piHiPri (99);
    pinMode(gpio, INPUT);
    wiringPiISR (gpio, INT_EDGE_BOTH, handle_interrupt);

    while(1) {
        sleep(1);
    }

    return 0;
}
