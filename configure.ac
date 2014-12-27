#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.59])
AC_INIT([domiotools], [0.1], [safchain@gmail.com])
AM_INIT_AUTOMAKE
AC_CONFIG_SRCDIR([include/common.h])
AC_CONFIG_HEADERS([config.h])

# Checks for programs.
AC_PROG_CC

EXTERNAL_CFLAGS="$CFLAGS"

AC_LANG([C])

# Checks for libraries.
AC_CHECK_LIB([pthread], pthread_create)
AC_CHECK_LIB([wiringPi], [wiringPiSetup])
AC_CHECK_LIB([config], [config_init])
AC_CHECK_LIB([mosquitto], mosquitto_lib_init)

PKG_CHECK_MODULES([CHECK], [check >= 0.9.8])

PKG_CHECK_MODULES([WIRINGPI], [wiringPi], [have_libwiringpi=yes], [have_libwiringpi=no])
AM_CONDITIONAL([WIRINGPI],  [test "$have_libwiringpi" = "yes"])

PKG_CHECK_MODULES([LIBCONFIG], [libconfig], [have_libconfig=yes], [have_libconfig=no])
AM_CONDITIONAL([LIBCONFIG],  [test "$have_libconfig" = "yes"])

PKG_CHECK_MODULES([LIBMOSQUITTO], [libmosquitto], [have_libmosquitto=yes], [have_libmosquitto=no])
AM_CONDITIONAL([LIBMOSQUITTO],  [test "$have_libmosquitto" = "yes"])

# Checks for header files.
AC_CHECK_HEADERS([stdlib.h string.h sys/time.h syslog.h unistd.h pthread.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_TYPE_MODE_T

AM_PROG_CC_C_O

# Checks for library functions.
AC_FUNC_MALLOC
AC_CHECK_FUNCS([mkdir strcasecmp strchr strdup])

AC_ARG_ENABLE(debug,
  AS_HELP_STRING(
    [--enable-debug],
    [enable debugging, default: no]),
    [case "${enableval}" in
      yes) debug=true ;;
      no)  debug=false ;;
      *)   AC_MSG_ERROR([bad value ${enableval} for --enable-debug]) ;;
    esac],
    [debug=false])
AM_CONDITIONAL(DEBUG, test x"$debug" = x"true")
AM_COND_IF(DEBUG,
    AC_DEFINE(DEBUG, 1, [Define to 0 if this is a release build]),
    AC_DEFINE(DEBUG, 0, [Define to 1 or higher if this is a debug build]))


AC_CONFIG_FILES([Makefile
                 tests/Makefile
                 src/Makefile])
AC_OUTPUT
