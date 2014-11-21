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
    unsigned char checksum :4;
    unsigned char ctrl :4;
    unsigned short code;
    struct address {
        unsigned char byte1;
        unsigned char byte2;
        unsigned char byte3;
    } address;
};

static void obfuscate_payload(struct srts_payload *payload) {
    unsigned char *p = (unsigned char *) payload;
    int i = 0;

    for (i = 1; i < 7; i++) {
        p[i] = p[i] ^ p[i - 1];
    }
}

/*
static void unfuscate_payload(struct srts_payload *payload) {
    struct srts_payload crypted;
    unsigned char *p, *c;
    int i = 0;

    memcpy((char *) &crypted, (char *) payload, sizeof(struct srts_payload));
    p = (unsigned char *) payload;
    c = (unsigned char *) &crypted;

    for (i = 1; i < 7; i++) {
        p[i] = p[i] ^ c[i - 1];
    }
}
*/

static void checksum_payload(struct srts_payload *payload) {
    unsigned char *p = (unsigned char *) payload;
    unsigned char checksum = 0;
    int i = 0;

    for (i = 0; i < 7; i++) {
        checksum = checksum ^ p[i] ^ (p[i] >> 4);
    }
    checksum = checksum & 0xf;
    payload->checksum = checksum;
}

static void write_bit(int gpio, char bit) {
    if (bit) {
        digitalWrite(gpio, LOW);
        delayMicroseconds(660);
        digitalWrite(gpio, HIGH);
        delayMicroseconds(660);
    } else {
        digitalWrite(gpio, HIGH);
        delayMicroseconds(660);
        digitalWrite(gpio, LOW);
        delayMicroseconds(660);
    }
}

static void write_byte(int gpio, unsigned char byte) {
    unsigned int mask;

    for (mask = 0b10000000; mask != 0x0; mask >>= 1) {
        write_bit(gpio, byte & mask);
    }
}

static void write_payload(int gpio, struct srts_payload *payload) {
    unsigned char *p = (unsigned char *) payload;
    int i;

    for (i = 0; i < 7; i++) {
        write_byte(gpio, p[i]);
    }
}

static void write_interval_gap(int gpio) {
    digitalWrite(gpio, LOW);
    delayMicroseconds(30400);
}

static unsigned short get_next_code(char *progname, unsigned short address) {
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

    memset(code, sizeof(code), 0);
    if (fgets(code, sizeof(code), fp) == NULL) {
        fclose(fp);
        return 1;
    }
    fclose(fp);

    return ((unsigned short) atoi(code)) + 1;
}

static void store_code(char *progname, unsigned short address, unsigned short new_code) {
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

static void sync_transmit(int gpio, int repeated) {
    int count, i;

    if (repeated) {
        count = 7;
    } else {
        digitalWrite(gpio, HIGH);
        delayMicroseconds(12400);
        digitalWrite(gpio, LOW);
        delayMicroseconds(80600);
        count = 2;
    }
    for (i = 0; i != count; i++) {
        digitalWrite(gpio, HIGH);
        delayMicroseconds(2560);
        digitalWrite(gpio, LOW);
        delayMicroseconds(2560);
    }
}

static void transmit(int gpio, unsigned char key, unsigned short address,
              unsigned char command, unsigned short code, int repeated) {
    struct srts_payload payload;

    sync_transmit(gpio, repeated);

    digitalWrite(gpio, HIGH);
    delayMicroseconds(4800);
    digitalWrite(gpio, LOW);
    delayMicroseconds(660);

    payload.key = key;
    payload.ctrl = command;
    payload.checksum = 0;
    payload.code = htons(code);
    payload.address.byte1 = ((char *) &address)[0];
    payload.address.byte2 = ((char *) &address)[1];
    payload.address.byte3 = 0;

    checksum_payload(&payload);
    obfuscate_payload(&payload);
    write_payload(gpio, &payload);
    write_interval_gap(gpio);
}

static char get_command_char(char *command) {
    if (strcasecmp(command, "my") == 0) {
        return MY;
    } else if (strcasecmp(command, "up") == 0) {
        return UP;
    } else if (strcasecmp(command, "my_up") == 0) {
        return MY_UP;
    } else if (strcasecmp(command, "down") == 0) {
        return DOWN;
    } else if (strcasecmp(command, "my_down") == 0) {
        return MY_DOWN;
    } else if (strcasecmp(command, "up_down") == 0) {
        return UP_DOWN;
    } else if (strcasecmp(command, "prog") == 0) {
        return PROG;
    } else if (strcasecmp(command, "sun_flag") == 0) {
        return SUN_FLAG;
    } else if (strcasecmp(command, "flag") == 0) {
        return FLAG;
    }

    return UNKNOWN;
}

static void usage(char *name) {
    printf(
        "Usage: %s --gpio <gpio pin> --address <remote address> --comand <command>\n",
        name);
    exit(-1);
}

int main(int argc, char **argv) {
    struct option long_options[] = { { "gpio", 1, 0, 0 },
        { "address", 1, 0, 0 }, { "command", 1, 0, 0 }, { NULL, 0, 0, 0 } };
    unsigned char key;
    unsigned short address = 0;
    unsigned short code = 0;
    long int a2i;
    int gpio = -1, i, c;
    char command = UNKNOWN;
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
                        break;
                    }
                    gpio = a2i;
                } else if (strcmp(long_options[i].name, "address") == 0) {
                    a2i = strtol(optarg, &end, 10);
                    if (errno == ERANGE && (a2i == LONG_MAX || a2i == LONG_MIN)) {
                        break;
                    }
                    address = a2i;
                } else if (strcmp(long_options[i].name, "command") == 0) {
                    command = get_command_char(optarg);
                }
                break;
            default:
                usage(argv[0]);
        }
    }

    if (command == UNKNOWN || address == 0 || gpio == -1) {
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

    pinMode(gpio, OUTPUT);

    openlog("srts", LOG_PID | LOG_CONS, LOG_USER);
    scheduler_realtime();

    progname = basename(argv[0]);

    code = get_next_code(progname, address);
    syslog(LOG_INFO, "remote: %d, command: %d, code: %d\n", address, command,
           code);
    closelog();

    transmit(gpio, key, address, command, code, 0);

    c = 7;
    if (command == PROG) {
        c = 20;
    }

    for (i = 0; i < c; i++) {
        transmit(gpio, key, address, command, code, 1);
    }
    store_code(progname, address, code);
    scheduler_standard();

    return 0;
}
