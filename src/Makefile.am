AM_CFLAGS = -I$(top_srcdir)/include -Wall @GCOV_CFLAGS@

if DEBUG
	AM_CFLAGS += -g
else
	AM_CFLAGS += -O3
endif

CLEANFILES = *~ *.gcno *.gcda *.gcov

bin_PROGRAMS = rf_gatewayd i2c_gatewayd

rf_gatewayd_SOURCES = rf_gatewayd.c utils.c srts.c hmap.c list.c urlparser.c \
					  mqtt.c rf_gateway.c mem.c logging.c homeasy.c gpio.c \
					  tree.c str.c
rf_gatewayd_LDADD = -lmosquitto -lconfig -lpthread \
					-lrt @GCOV_LIBS@

i2c_gatewayd_SOURCES = utils.c hmap.c list.c urlparser.c str.c \
					  mqtt.c i2c.c mem.c logging.c gpio.c tree.c
i2c_gatewayd_LDADD = -lmosquitto -lconfig -lpthread \
					-lrt @GCOV_LIBS@


