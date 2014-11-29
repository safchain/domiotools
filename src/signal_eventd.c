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
#include <stdio.h>
#include <sys/time.h>
#include <time.h>
#include <stdlib.h>
#include <sched.h>
#include <syslog.h>
#include <getopt.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <errno.h>
#include <arpa/inet.h>
#include <libgen.h>
#include <errno.h>
#include <limits.h>

#include "common.h"
#include "srts.h"

extern int verbose;

int somfy_handler(int type, int duration) {
    struct srts_payload payload;
    int rtv;

    rtv = srts_receive(type, duration, &payload);
    if (rtv == 1 && verbose) {
        printf("Message correctly received\n");
    }
    return rtv;
}

void handle_interrupt() {
    static unsigned int last_change = 0;
    static int duration = 0;
    static int accu = 0;
    int type;

    type = digitalRead (2);
    if (type == LOW) {
        type = HIGH;
    } else {
        type = LOW;
    }

    long time = micros();
    if (last_change) {
        duration = time - last_change;
    } else {
        last_change = time;
        return;
    }
    last_change = time;

    somfy_handler(type, duration);

    return;
//    printf("CHG: %d - %d:%d\n", time - last_change, accu, duration);

    /* noise reduction */
    if ((accu - duration) <= 0) {
        if (type == HIGH) {
            type = LOW;
        } else {
            type = HIGH;
        }
        //printf("Somfy\n");
    //    somfy(type, duration);

        accu = duration;
        if (accu > 50) {
            accu = 50;
        }
    }
    else {
        accu -= duration;
    }

    last_change = time;
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

    piHiPri (99);
    pinMode(gpio, INPUT);
    wiringPiISR (gpio, INT_EDGE_BOTH, handle_interrupt);

    while(1) {
        sleep(1);
    }

    return 0;
}
