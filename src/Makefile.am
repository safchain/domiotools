AM_CFLAGS = -I$(top_srcdir)/include -Wall @GCOV_CFLAGS@

if DEBUG
	AM_CFLAGS += -g
else
	AM_CFLAGS += -O3
endif

CLEANFILES = *~ *.gcno *.gcda *.gcov

bin_PROGRAMS = srts_sender homeasy_sender rf_gatewayd
srts_sender_SOURCES = srts.c common.c srts_sender.c mem.c
srts_sender_LDADD = -lwiringPi -lpthread @GCOV_LIBS@

homeasy_sender_SOURCES = homeasy_sender.c common.c mem.c
homeasy_sender_LDADD = -lwiringPi -lpthread @GCOV_LIBS@

rf_gatewayd_SOURCES = rf_gatewayd.c common.c srts.c hmap.c list.c urlparser.c mqtt.c rf_gateway.c mem.c
rf_gatewayd_LDADD = -lwiringPi -lmosquitto -lconfig -lpthread @GCOV_LIBS@
