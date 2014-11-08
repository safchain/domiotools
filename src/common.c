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

#include <sched.h>
#include <stdio.h>

void scheduler_standard() {
    struct sched_param p;

    p.__sched_priority = 0;
    if (sched_setscheduler(0, SCHED_OTHER, &p) == -1) {
        perror("Failed to switch to normal scheduler.");
    }
}

void scheduler_realtime() {
    struct sched_param p;

    p.__sched_priority = sched_get_priority_max(SCHED_RR);
    if (sched_setscheduler(0, SCHED_RR, &p) == -1) {
        perror("Failed to switch to realtime scheduler.");
    }
}
