TESTS = check_hl check_urlparser check_mqtt check_rf_gateway check_srts
check_PROGRAMS = check_hl check_urlparser check_mqtt check_rf_gateway check_srts

check_hl_SOURCES = check_hl.c \
				   $(top_builddir)/src/list.c \
				   $(top_builddir)/src/hmap.c
check_hl_CFLAGS = @CHECK_CFLAGS@ -I$(top_builddir)/include @GCOV_CFLAGS@
check_hl_LDADD = @CHECK_LIBS@ @GCOV_LIBS@

check_urlparser_SOURCES = check_urlparser.c \
						  $(top_builddir)/src/urlparser.c
check_urlparser_CFLAGS = @CHECK_CFLAGS@ -I$(top_builddir)/include @GCOV_CFLAGS@
check_urlparser_LDADD = @CHECK_LIBS@ @GCOV_LIBS@

check_srts_SOURCES = check_srts.c \
					 $(top_builddir)/src/mem.c \
					 $(top_builddir)/src/common.c \
					 $(top_builddir)/src/srts.c
check_srts_CFLAGS = @CHECK_CFLAGS@ -I$(top_builddir)/include @GCOV_CFLAGS@
check_srts_LDADD = @CHECK_LIBS@ -lcmocker @GCOV_LIBS@

check_mqtt_SOURCES = check_mqtt.c \
					 $(top_builddir)/src/urlparser.c \
					 $(top_builddir)/src/mem.c \
					 $(top_builddir)/src/list.c \
					 $(top_builddir)/src/hmap.c \
					 $(top_builddir)/src/mqtt.c
check_mqtt_CFLAGS = @CHECK_CFLAGS@ -I$(top_builddir)/include @GCOV_CFLAGS@
check_mqtt_LDADD = @CHECK_LIBS@ -lcmocker -lpthread @GCOV_LIBS@

check_rf_gateway_SOURCES = check_rf_gateway.c \
						   $(top_builddir)/src/urlparser.c \
						   $(top_builddir)/src/mem.c \
						   $(top_builddir)/src/list.c \
						   $(top_builddir)/src/hmap.c \
						   $(top_builddir)/src/rf_gateway.c
check_rf_gateway_CFLAGS = @CHECK_CFLAGS@ -I$(top_builddir)/include @GCOV_CFLAGS@
check_rf_gateway_LDADD = @CHECK_LIBS@ -lconfig -lcmocker @GCOV_LIBS@

CLEANFILES = *~ *.gcno *.gcda *.gcov
