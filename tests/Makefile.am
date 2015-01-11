TESTS = check_domiotools
check_PROGRAMS = check_domiotools
check_domiotools_SOURCES = check_domiotools.c \
						   check_urlparser.c \
						   check_mqtt.c \
						   check_hl.c \
						   check_rf_gateway.c \
						   mock.c \
						   $(top_builddir)/src/urlparser.c \
						   $(top_builddir)/src/list.c \
						   $(top_builddir)/src/hash.c \
						   $(top_builddir)/src/mqtt.c
check_domiotools_CFLAGS = @CHECK_CFLAGS@ -I$(top_builddir)/include -DTEST
check_domiotools_LDADD = @CHECK_LIBS@ -lpthread
