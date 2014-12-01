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

#include <stdlib.h>
#include <stdio.h>
#include <sys/stat.h>
#include <errno.h>
#include <sys/file.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>

int verbose = 0;
int debug = 0;

static int do_mkdir(const char *path, mode_t mode) {
    struct stat st;
    int status = 0;

    if (stat(path, &st) != 0) {
        if (mkdir(path, mode) != 0 && errno != EEXIST) {
            status = -1;
        }
    } else if (!S_ISDIR(st.st_mode)) {
        errno = ENOTDIR;
        status = -1;
    }

    return status;
}

int mkpath(const char *path, mode_t mode) {
    char *pp, *sp;
    int status;
    char *copypath = strdup(path);

    status = 0;
    pp = copypath;
    while (status == 0 && (sp = strchr(pp, '/')) != 0) {
        if (sp != pp) {
            *sp = '\0';
            status = do_mkdir(copypath, mode);
            *sp = '/';
        }
        pp = sp + 1;
    }
    if (status == 0) {
        status = do_mkdir(path, mode);
    }
    free(copypath);

    return status;
}

void store_pid() {
    char *path;
    FILE *fp;
    int size;

    size = snprintf(NULL, 0, "/var/run/domiotools.pid");
    if ((path = (char *) malloc(size + 1)) == NULL) {
        fprintf(stderr, "Memory allocation error\n");
        exit(-1);
    }
    sprintf(path, "/var/run/");
    if (mkpath(path, 0755) == -1) {
        fprintf(stderr, "Unable to create the pid path: %s\n", path);
        exit(-1);
    }
    sprintf(path, "/var/run/domiotools.pid");

    if ((fp = fopen(path, "w+")) == NULL) {
        fprintf(stderr, "Unable to open the pid file: %s", path);
        exit(-1);
    }

    if (flock(fileno(fp), LOCK_EX) == -1) {
         fprintf(stderr, "Unable to lock the pid file: %s", path);
        exit(-1);
    }

    if (fprintf(fp, "%d\n", getpid()) < 0) {
        fclose(fp);
        exit(-1);
    }

    fclose(fp);
}

