AM_CFLAGS = -I$(top_srcdir)/include -Wall

if DEBUG
	AM_CFLAGS += -g
else
	AM_CFLAGS += -O3
endif

bin_PROGRAMS = srts_sender homeasy_sender rf_gatewayd
srts_sender_SOURCES = srts.c common.c srts_sender.c
srts_sender_LDADD = -lwiringPi -lpthread

homeasy_sender_SOURCES = homeasy_sender.c common.c
homeasy_sender_LDADD = -lwiringPi -lpthread

rf_gatewayd_SOURCES = rf_gatewayd.c common.c srts.c hmap.c list.c urlparser.c mqtt.c rf_gateway.c
rf_gatewayd_LDADD = -lwiringPi -lmosquitto -lconfig -lpthread
