AM_CFLAGS = -I$(top_srcdir)/include -Wall @GCOV_CFLAGS@

if DEBUG
	AM_CFLAGS += -g
else
	AM_CFLAGS += -O3
endif

CLEANFILES = *~ *.gcno *.gcda *.gcov

bin_PROGRAMS = rf_gatewayd

rf_gatewayd_SOURCES = rf_gatewayd.c common.c srts.c hmap.c list.c urlparser.c \
					  mqtt.c rf_gateway.c mem.c logging.c homeasy.c gpio.c
rf_gatewayd_LDADD = -lmosquitto -lconfig -lpthread -lev \
					-lrt @GCOV_LIBS@
