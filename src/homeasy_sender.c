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
#include <syslog.h>
#include <getopt.h>
#include <sys/types.h>
#include <limits.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdlib.h>

#include "common.h"

enum COMMAND {
    OFF = 0,
    ON = 1,
    UNKNOWN
};

static void _write_bit(int gpio, char bit) {
    if (bit) {
        digitalWrite(gpio, HIGH);
        delayMicroseconds(300);
        digitalWrite(gpio, LOW);
        delayMicroseconds(1300);
    } else {
        digitalWrite(gpio, HIGH);
        delayMicroseconds(300);
        digitalWrite(gpio, LOW);
        delayMicroseconds(300);
    }
}

static void write_bit(int gpio, char bit) {
    if (bit) {
        _write_bit(gpio, 1);
        _write_bit(gpio, 0);
    } else {
        _write_bit(gpio, 0);
        _write_bit(gpio, 1);
    }
}

static void sync_transmit(int gpio) {
    digitalWrite(gpio, HIGH);
    delayMicroseconds(275);
    digitalWrite(gpio, LOW);
    delayMicroseconds(9900);
    digitalWrite(gpio, HIGH);
    delayMicroseconds(275);
    digitalWrite(gpio, LOW);
    delayMicroseconds(2600);
    digitalWrite(gpio, HIGH);
}

static void write_interval_gap(int gpio) {
    digitalWrite(gpio, HIGH);
    delayMicroseconds(275);
    digitalWrite(gpio, LOW);
    delay(10);
}

void transmit(int gpio, unsigned int address, unsigned char receiver,
              unsigned char command) {
    unsigned int mask;

    sync_transmit(gpio);

    for (mask = 0x2000000; mask != 0x0; mask >>= 1) {
        if (address & mask) {
            write_bit(gpio, 1);
        } else {
            write_bit(gpio, 0);
        }
    }

    // never grouped
    write_bit(gpio, 0);

    write_bit(gpio, command);

    for (mask = 0b1000; mask != 0x0; mask >>= 1) {
        write_bit(gpio, receiver & mask);
    }

    write_interval_gap(gpio);
}

static char get_command_char(char *command) {
    if (strcasecmp(command, "on") == 0) {
        return ON;
    } else if (strcasecmp(command, "off") == 0) {
        return OFF;
    }

    return UNKNOWN;
}

static void usage(char *name) {
    printf(
        "Usage: %s --gpio <gpio pin> --address <remote address> --comand <command>\n",
        name);
    exit(-1);
}

int main(int argc, char** argv) {
    struct option long_options[] = { { "gpio", 1, 0, 0 },
        { "address", 1, 0, 0 }, { "command", 1, 0, 0 }, { "receiver", 1, 0, 0 },
        { "retry", 1, 0, 0 }, { NULL, 0, 0, 0 } };
    unsigned int address = 0;
    unsigned char receiver = 1;
    long int a2i;
    int gpio = -1;
    char command = UNKNOWN;
    char *end;
    int retry = 5, i, c;

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
                        break;
                    }
                    gpio = a2i;
                } else if (strcmp(long_options[i].name, "address") == 0) {
                    a2i = strtol(optarg, &end, 10);
                    if (errno == ERANGE && (a2i == LONG_MAX || a2i == LONG_MIN)) {
                        break;
                    }
                    address = a2i;
                } else if (strcmp(long_options[i].name, "receiver") == 0) {
                    a2i = strtol(optarg, &end, 10);
                    if (errno == ERANGE && (a2i == LONG_MAX || a2i == LONG_MIN)) {
                        break;
                    }
                    receiver = a2i;
                } else if (strcmp(long_options[i].name, "command") == 0) {
                    command = get_command_char(optarg);
                } else if (strcmp(long_options[i].name, "command") == 0) {
                    a2i = strtol(optarg, &end, 10);
                    if (errno == ERANGE && (a2i == LONG_MAX || a2i == LONG_MIN)) {
                        break;
                    }
                    retry = a2i;
                }
                break;
            default:
                usage(argv[0]);
        }
    }

    if (command == UNKNOWN || address == 0 || gpio == -1 || receiver == 0) {
        usage(argv[0]);
    }

    // store pid and lock it
    store_pid();

    if (wiringPiSetup() == -1) {
        fprintf(stderr, "Wiring Pi not installed");
        return -1;
    }

    openlog("homeasy", LOG_PID | LOG_CONS, LOG_USER);
    syslog(LOG_INFO, "remote: %d, receiver, %d, command: %d\n", address,
           receiver, command);
    closelog();

    pinMode(gpio, OUTPUT);
    piHiPri(99);

    for (c = 0; c != retry; c++) {
        for (i = 0; i < 5; i++) {
            transmit(gpio, address, receiver, command);
        }

        sleep(1);
    }

    return 0;
}
