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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <errno.h>
#include <check.h>

#include "logging.h"

START_TEST(test_log_null)
{
  struct dlog *log = dlog_init(DLOG_NULL, DLOG_DEBUG, NULL);
  ck_assert(log != NULL);

  dlog(log, DLOG_DEBUG, "%s", "test");

  dlog_destroy(log);
}
END_TEST

START_TEST(test_log_file_fails)
{
  struct dlog *log;

  log = dlog_init(DLOG_FILE, DLOG_DEBUG, "/tmpXXX");
  ck_assert(log == NULL);
}
END_TEST

START_TEST(test_log_file)
{
  struct dlog *log;
  char *filename, buff[BUFSIZ], *str;
  FILE *fp;

  filename = tmpnam(NULL);

  log = dlog_init(DLOG_FILE, DLOG_DEBUG, filename);
  ck_assert(log != NULL);

  dlog(log, DLOG_DEBUG, "%s", "test123");

  fp = fopen(filename, "r");
  ck_assert(fp != NULL);

  fgets(buff, BUFSIZ, fp);

  str = strstr(buff, "test123");
  ck_assert(str != NULL);

  dlog_destroy(log);
  fclose(fp);
}
END_TEST

START_TEST(test_log_fp)
{
  struct dlog *log;
  char buff[BUFSIZ], *str;
  FILE *fp;

  fp = tmpfile();
  ck_assert(fp != NULL);

  log = dlog_init(DLOG_FP, DLOG_DEBUG, fp);
  ck_assert(log != NULL);

  dlog(log, DLOG_DEBUG, "%s", "test123");

  fseek(fp, 0, SEEK_SET);
  fgets(buff, BUFSIZ, fp);

  str = strstr(buff, "test123");
  ck_assert(str != NULL);

  dlog_destroy(log);
  fclose(fp);
}
END_TEST

START_TEST(test_log_fp_all_priorities)
{
  struct dlog *log;
  char buff[BUFSIZ], *str;
  FILE *fp;
  int i;

  fp = tmpfile();
  ck_assert(fp != NULL);

  log = dlog_init(DLOG_FP, DLOG_DEBUG, fp);
  ck_assert(log != NULL);

  for (i = DLOG_EMERG; i <= DLOG_DEBUG; i++) {
    dlog(log, i, "%s", "test123");

    fseek(fp, 0, SEEK_SET);
    fgets(buff, BUFSIZ, fp);

    str = strstr(buff, "test123");
    ck_assert(str != NULL);
  }

  dlog_destroy(log);
  fclose(fp);
}
END_TEST

START_TEST(test_log_syslog_all_priorities)
{
  struct dlog *log;
  int i;

  log = dlog_init(DLOG_SYSLOG, DLOG_DEBUG, "test");
  ck_assert(log != NULL);

  for (i = DLOG_EMERG; i <= DLOG_DEBUG; i++) {
    dlog(log, i, "%s", "test123");
  }

  dlog_destroy(log);
}
END_TEST

START_TEST(test_log_syslog)
{
  struct dlog *log;

  log = dlog_init(DLOG_SYSLOG, DLOG_DEBUG, "test");
  ck_assert(log != NULL);

  dlog(log, DLOG_DEBUG, "%s", "test123");

  dlog_destroy(log);
}
END_TEST

START_TEST(test_log_fp_priority)
{
  struct dlog *log;
  char buff[BUFSIZ], *str;
  FILE *fp;

  fp = tmpfile();
  ck_assert(fp != NULL);

  log = dlog_init(DLOG_FP, DLOG_WARNING, fp);
  ck_assert(log != NULL);

  dlog(log, DLOG_DEBUG, "%s", "test123");

  fseek(fp, 0, SEEK_SET);
  fgets(buff, BUFSIZ, fp);

  str = strstr(buff, "test123");
  ck_assert(str == NULL);

  dlog(log, DLOG_ERR, "%s", "test456");

  fseek(fp, 0, SEEK_SET);
  fgets(buff, BUFSIZ, fp);

  str = strstr(buff, "test456");
  ck_assert(str != NULL);

  dlog_destroy(log);
  fclose(fp);
}
END_TEST

Suite *logging_suite(void)
{
  Suite *s;
  TCase *tc_logging;

  s = suite_create("logging");
  tc_logging = tcase_create("logging");

  tcase_add_test(tc_logging, test_log_null);
  tcase_add_test(tc_logging, test_log_file_fails);
  tcase_add_test(tc_logging, test_log_file);
  tcase_add_test(tc_logging, test_log_fp);
  tcase_add_test(tc_logging, test_log_fp_priority);
  tcase_add_test(tc_logging, test_log_syslog);
  tcase_add_test(tc_logging, test_log_fp_all_priorities);
  tcase_add_test(tc_logging, test_log_syslog_all_priorities);
  suite_add_tcase(s, tc_logging);

  return s;
}

int main(void)
{
  SRunner *sr;
  int number_failed;

  sr = srunner_create(logging_suite());
  srunner_run_all(sr, CK_VERBOSE);

  number_failed = srunner_ntests_failed(sr);
  srunner_free(sr);

  return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
