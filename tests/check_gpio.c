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
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <unistd.h>
#include <time.h>

#include "gpio.h"

START_TEST(test_export_fails)
{
  gpio_set_syspath("/tmpXXX");
  ck_assert_int_eq(0, gpio_export(2));
}
END_TEST

START_TEST(test_export_success)
{
  mkdir("/tmp/gpio2", 0755);
  gpio_set_syspath("/tmp");
  ck_assert_int_eq(1, gpio_export(2));
}
END_TEST

START_TEST(test_direction_fails)
{
  gpio_set_syspath("/tmpXXX");
  ck_assert_int_eq(0, gpio_direction(2, GPIO_IN));
}
END_TEST

START_TEST(test_direction_success)
{
  mkdir("/tmp/gpio2", 0755);
  gpio_set_syspath("/tmp");
  ck_assert_int_eq(1, gpio_direction(2, GPIO_IN));
}
END_TEST

START_TEST(test_edge_detection_fails)
{
  gpio_set_syspath("/tmpXXX");
  ck_assert_int_eq(0, gpio_edge_detection(2, GPIO_EDGE_BOTH));
}
END_TEST

START_TEST(test_edge_detection_success)
{
  mkdir("/tmp/gpio2", 0755);
  gpio_set_syspath("/tmp");
  ck_assert_int_eq(1, gpio_edge_detection(2, GPIO_EDGE_BOTH));
}
END_TEST

START_TEST(test_open_fails)
{
  gpio_set_syspath("/tmpXXX");
  ck_assert_int_eq(-1, gpio_open(2, GPIO_OUT));
}
END_TEST

START_TEST(test_open_success)
{
  int fd;

  mkdir("/tmp/gpio2", 0755);
  gpio_set_syspath("/tmp");

  fd = open("/tmp/gpio2/value", O_WRONLY | O_CREAT, 0655);
  ck_assert_int_ne(-1, fd);
  close(fd);

  ck_assert_int_ne(-1, gpio_open(2, GPIO_OUT));
  gpio_close(2);
}
END_TEST

START_TEST(test_two_opens)
{
  int fd;

  mkdir("/tmp/gpio2", 0755);
  gpio_set_syspath("/tmp");

  fd = open("/tmp/gpio2/value", O_WRONLY | O_CREAT, 0655);
  ck_assert_int_ne(-1, fd);
  close(fd);

  fd = gpio_open(2, GPIO_OUT);
  ck_assert_int_ne(-1, fd);

  ck_assert_int_eq(fd, gpio_open(2, GPIO_OUT));
  gpio_close(2);
}
END_TEST

START_TEST(test_read_write)
{
  char value;
  int fd;

  mkdir("/tmp/gpio2", 0755);
  gpio_set_syspath("/tmp");

  fd = open("/tmp/gpio2/value", O_WRONLY | O_CREAT, 0655);
  ck_assert_int_ne(-1, fd);
  close(fd);

  fd = gpio_open(2, GPIO_OUT);
  ck_assert_int_ne(-1, fd);

  ck_assert_int_ne(-1, gpio_write(2, GPIO_HIGH));
  gpio_close(2);

  fd = gpio_open(2, GPIO_IN);
  ck_assert_int_ne(-1, fd);

  value = gpio_read(2);
  ck_assert_int_eq(GPIO_HIGH, value);
  gpio_close(2);
}
END_TEST

START_TEST(test_usleep)
{
  unsigned long start, end, diff;

  start = gpio_time();
  gpio_usleep(400);
  end = gpio_time();

  diff = end - start;
  ck_assert(diff > 350 && diff < 550);

  start = gpio_time();
  gpio_usleep(2500);
  end = gpio_time();

  diff = end - start;
  ck_assert(diff > 2500 && diff < 2600);
}
END_TEST

START_TEST(test_sched)
{
  int rc;

  /* need to be root */
  rc = gpio_sched_priority(88);
  ck_assert_int_eq(-1, rc);
  ck_assert_int_eq(EPERM, errno);

  rc = gpio_sched_priority(88888888);
  ck_assert_int_eq(-1, rc);
  ck_assert_int_eq(EPERM, errno);
}
END_TEST

Suite *gpio_suite(void)
{
  Suite *s;
  TCase *tc_gpio;

  s = suite_create("gpio");
  tc_gpio = tcase_create("gpio");

  tcase_add_test(tc_gpio, test_export_fails);
  tcase_add_test(tc_gpio, test_export_success);
  tcase_add_test(tc_gpio, test_direction_fails);
  tcase_add_test(tc_gpio, test_direction_success);
  tcase_add_test(tc_gpio, test_edge_detection_fails);
  tcase_add_test(tc_gpio, test_edge_detection_success);
  tcase_add_test(tc_gpio, test_open_fails);
  tcase_add_test(tc_gpio, test_open_success);
  tcase_add_test(tc_gpio, test_two_opens);
  tcase_add_test(tc_gpio, test_read_write);
  tcase_add_test(tc_gpio, test_usleep);
  tcase_add_test(tc_gpio, test_sched);
  suite_add_tcase(s, tc_gpio);

  return s;
}

int main(void)
{
  SRunner *sr;
  int number_failed;

  sr = srunner_create(gpio_suite());
  srunner_run_all(sr, CK_VERBOSE);

  number_failed = srunner_ntests_failed(sr);
  srunner_free(sr);

  return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
