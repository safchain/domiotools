AM_CFLAGS = $(WIRINGPI_CFLAGS) $(CONFIG_CFLAGS) -I$(top_srcdir)/include -Wall

if DEBUG
	AM_CFLAGS += -g
else
	AM_CFLAGS += -O3
endif


bin_PROGRAMS = srts_sender homeasy_sender rf_eventd
srts_sender_SOURCES = srts.c common.c srts_sender.c
srts_sender_LDADD = $(WIRINGPI_LIBS)

homeasy_sender_SOURCES = homeasy_sender.c common.c
homeasy_sender_LDADD = $(WIRINGPI_LIBS)

rf_eventd_SOURCES = rf_eventd.c common.c srts.c hash.c list.c urlparser.c
rf_eventd_LDADD = $(WIRINGPI_LIBS) $(CONFIG_LIBS) $(MOSQUITTO_LIBS) $(URIPARSER_LIBS) -lpthread
