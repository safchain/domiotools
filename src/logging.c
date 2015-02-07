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

#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <syslog.h>
#include <stdlib.h>
#include <time.h>

#include "mem.h"
#include "logging.h"

struct dlog *dlog_init(int type, int priority, void *obj) {
  struct dlog *log = xmalloc(sizeof(struct dlog));

  log->type = type;
  log->priority = priority;

  if (type == DLOG_SYSLOG) {
    openlog((char *) obj, LOG_PID, LOG_DAEMON);
  } else if (type == DLOG_FILE) {
    log->fp = fopen((char *) obj, "a");
    if (log->fp == NULL) {
      dlog_vprintf(stderr, LOG_CRIT, "Unable to open the log file: %s",
                  (char *) obj);
      goto clean;
    }
  } else if (type == DLOG_FP) {
    log->fp = (FILE *) obj;
  }

  return log;

clean:
  free(log);

  return NULL;
}

void dlog_destroy(struct dlog *log)
{
  if (log->type & DLOG_SYSLOG) {
    closelog();
  } else if (log->type & DLOG_FILE) {
    fclose(log->fp);
  }
  free(log);
}

static const char *priority_str(int priority)
{
  switch(priority) {
    case DLOG_DEBUG:
      return "DEBUG";
    case DLOG_INFO:
      return "INFO";
    case DLOG_NOTICE:
      return "NOTICE";
    case DLOG_WARNING:
      return "WARNING";
    case DLOG_ERR:
      return "ERR";
    case DLOG_CRIT:
      return "CRIT";
    case DLOG_ALERT:
      return "ALERT";
    case DLOG_EMERG:
      return "EMERG";
    default:
      return "UNKNOW";
  }
}

static int syslog_priority(int priority)
{
  switch(priority) {
    case DLOG_DEBUG:
      return LOG_DEBUG;
    case DLOG_INFO:
      return LOG_INFO;
    case DLOG_NOTICE:
      return LOG_NOTICE;
    case DLOG_WARNING:
      return LOG_WARNING;
    case DLOG_ERR:
      return LOG_ERR;
    case DLOG_CRIT:
      return LOG_CRIT;
    case DLOG_ALERT:
      return LOG_ALERT;
    case DLOG_EMERG:
      return LOG_EMERG;
    default:
      return 0;
  }
}

void dlog_vfprintf(FILE *fp, int priority, const char *fmt, va_list ap)
{
  time_t curtime;
  struct tm *loctime;
  char bufftime[BUFSIZ];

  time(&curtime);
  loctime = gmtime(&curtime);

  strftime(bufftime, BUFSIZ, "%Y-%m-%d %H:%M:%S", loctime);

  fprintf(fp, "%s\t%s\t", bufftime, priority_str(priority));

  vfprintf(fp, fmt, ap);

  fputs("\n", fp);
}

void dlog_vprintf(FILE *fp, int priority, const char *fmt, ...)
{
  va_list ap;

  va_start(ap, fmt);
  dlog_vfprintf(fp, priority, fmt, ap);
  va_end(ap);
}

void dlog_vsyslog(int priority, const char *fmt, va_list ap)
{
  va_list aq;
  char *str;
  int len;


  va_copy(aq, ap);

  len = vsnprintf(NULL, 0, fmt, ap);
  str = xmalloc(len + 1);
  vsprintf(str, fmt, aq);

  syslog(syslog_priority(priority), "%s", str);

  va_end(aq);
  free(str);
}

void dlog(struct dlog *log, int priority, const char *fmt, ...)
{
  va_list ap;
  FILE *fp = NULL;

  if (priority > log->priority || log->type == DLOG_NULL) {
    return;
  }

  va_start(ap, fmt);

  switch(log->type) {
    case DLOG_STDOUT:
      fp = stdout;
      break;
    case DLOG_STDERR:
      fp = stderr;
      break;
    case DLOG_FILE:
    case DLOG_FP:
      fp = log->fp;
      break;
    case DLOG_SYSLOG:
      dlog_vsyslog(priority, fmt, ap);
      va_end(ap);

      return;
    default:
      fp = stderr;
  }
  dlog_vfprintf(log->fp, priority, fmt, ap);

  va_end(ap);

  fflush(fp);
}
