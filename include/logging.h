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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#ifndef __LOGGING_H
#define __LOGGING_H

#include <stdio.h>

enum {
  DLOG_STDERR = 1,
  DLOG_STDOUT,
  DLOG_SYSLOG,
  DLOG_FILE,
  DLOG_FP,
  DLOG_NULL
};

enum {
  DLOG_EMERG = 1,
  DLOG_ALERT,
  DLOG_CRIT,
  DLOG_ERR,
  DLOG_WARNING,
  DLOG_NOTICE,
  DLOG_INFO,
  DLOG_DEBUG
};

struct dlog {
  int type;
  int priority;
  FILE *fp;
};

struct dlog *dlog_init(int type, int priority, void *name);
void dlog_destroy(struct dlog *log);
void dlog_vfprintf(FILE *fp, int priority, const char *fmt, va_list ap);
void dlog_vprintf(FILE *fp, int priority, const char *fmt, ...);
void dlog_vsyslog(int priority, const char *fmt, va_list ap);
void dlog(struct dlog *log, int priority, const char *fmt, ...);

#endif
