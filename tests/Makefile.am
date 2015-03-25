TESTS = check_hl check_urlparser check_mqtt check_rf_gateway check_srts \
		check_logging check_mem check_homeasy check_gpio
check_PROGRAMS = check_hl check_urlparser check_mqtt check_rf_gateway \
				 check_srts check_logging check_mem check_homeasy check_gpio

check_hl_SOURCES = check_hl.c \
				   $(top_builddir)/src/list.c \
				   $(top_builddir)/src/mem.c \
				   $(top_builddir)/src/hmap.c
check_hl_CFLAGS = @CHECK_CFLAGS@ -I$(top_builddir)/include @GCOV_CFLAGS@ -Wall
check_hl_LDADD = @CHECK_LIBS@ @GCOV_LIBS@

check_mem_SOURCES = check_mem.c \
				   $(top_builddir)/src/mem.c
check_mem_CFLAGS = @CHECK_CFLAGS@ -I$(top_builddir)/include @GCOV_CFLAGS@ -Wall
check_mem_LDADD = @CHECK_LIBS@ @GCOV_LIBS@

check_logging_SOURCES = check_logging.c \
				        $(top_builddir)/src/mem.c \
				        $(top_builddir)/src/logging.c
check_logging_CFLAGS = @CHECK_CFLAGS@ -I$(top_builddir)/include @GCOV_CFLAGS@ -Wall
check_logging_LDADD = @CHECK_LIBS@ @GCOV_LIBS@

check_gpio_SOURCES = check_gpio.c \
					 $(top_builddir)/src/mem.c \
					 $(top_builddir)/src/gpio.c
check_gpio_CFLAGS = @CHECK_CFLAGS@ -I$(top_builddir)/include @GCOV_CFLAGS@ -Wall
check_gpio_LDADD = @CHECK_LIBS@ @GCOV_LIBS@ -lrt

check_urlparser_SOURCES = check_urlparser.c \
						  $(top_builddir)/src/mem.c \
						  $(top_builddir)/src/urlparser.c
check_urlparser_CFLAGS = @CHECK_CFLAGS@ -I$(top_builddir)/include @GCOV_CFLAGS@ -Wall
check_urlparser_LDADD = @CHECK_LIBS@ @GCOV_LIBS@

check_srts_SOURCES = check_srts.c \
					 $(top_builddir)/src/mem.c \
					 $(top_builddir)/src/common.c \
					 $(top_builddir)/src/logging.c \
					 $(top_builddir)/src/srts.c
check_srts_CFLAGS = @CHECK_CFLAGS@ -I$(top_builddir)/include @GCOV_CFLAGS@ -Wall
check_srts_LDADD = @CHECK_LIBS@ -lcmocker @GCOV_LIBS@

check_homeasy_SOURCES = check_homeasy.c \
						$(top_builddir)/src/mem.c \
						$(top_builddir)/src/logging.c \
						$(top_builddir)/src/homeasy.c
check_homeasy_CFLAGS = @CHECK_CFLAGS@ -I$(top_builddir)/include @GCOV_CFLAGS@ -Wall
check_homeasy_LDADD = @CHECK_LIBS@ -lcmocker @GCOV_LIBS@

check_mqtt_SOURCES = check_mqtt.c \
					 $(top_builddir)/src/urlparser.c \
					 $(top_builddir)/src/mem.c \
					 $(top_builddir)/src/list.c \
					 $(top_builddir)/src/hmap.c \
					 $(top_builddir)/src/logging.c \
					 $(top_builddir)/src/mqtt.c
check_mqtt_CFLAGS = @CHECK_CFLAGS@ -I$(top_builddir)/include @GCOV_CFLAGS@ -Wall
check_mqtt_LDADD = @CHECK_LIBS@ -lcmocker -lpthread @GCOV_LIBS@

check_rf_gateway_SOURCES = check_rf_gateway.c \
						   $(top_builddir)/src/urlparser.c \
						   $(top_builddir)/src/mem.c \
						   $(top_builddir)/src/list.c \
						   $(top_builddir)/src/hmap.c \
						   $(top_builddir)/src/logging.c \
						   $(top_builddir)/src/gpio.c \
						   $(top_builddir)/src/rf_gateway.c
check_rf_gateway_CFLAGS = @CHECK_CFLAGS@ -I$(top_builddir)/include @GCOV_CFLAGS@ -Wall
check_rf_gateway_LDADD = @CHECK_LIBS@ -lconfig -lcmocker -lev -lrt @GCOV_LIBS@

CLEANFILES = *~ *.gcno *.gcda *.gcov
