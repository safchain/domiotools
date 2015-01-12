TESTS = check_hl check_urlparser check_mqtt check_rf_gateway
check_PROGRAMS = check_hl check_urlparser check_mqtt check_rf_gateway

check_hl_SOURCES = check_hl.c \
				   $(top_builddir)/src/list.c \
				   $(top_builddir)/src/hash.c
check_hl_CFLAGS = @CHECK_CFLAGS@ -I$(top_builddir)/include
check_hl_LDADD = @CHECK_LIBS@

check_urlparser_SOURCES = check_urlparser.c \
						  $(top_builddir)/src/urlparser.c
check_urlparser_CFLAGS = @CHECK_CFLAGS@ -I$(top_builddir)/include
check_urlparser_LDADD = @CHECK_LIBS@

check_mqtt_SOURCES = check_mqtt.c \
					 mock.c \
					 $(top_builddir)/src/urlparser.c \
					 $(top_builddir)/src/list.c \
					 $(top_builddir)/src/hash.c \
					 $(top_builddir)/src/mqtt.c
check_mqtt_CFLAGS = @CHECK_CFLAGS@ -I$(top_builddir)/include -lpthread
check_mqtt_LDADD = @CHECK_LIBS@

check_rf_gateway_SOURCES = check_rf_gateway.c \
						   mock.c \
						   $(top_builddir)/src/urlparser.c \
						   $(top_builddir)/src/list.c \
						   $(top_builddir)/src/hash.c \
						   $(top_builddir)/src/rf_gateway.c
check_rf_gateway_CFLAGS = @CHECK_CFLAGS@ -I$(top_builddir)/include
check_rf_gateway_LDADD = @CHECK_LIBS@ -lconfig
