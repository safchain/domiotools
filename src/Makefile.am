if DEBUG
  AM_CFLAGS =-I$(top_srcdir)/include -Wall -g
else
  AM_CFLAGS =-I$(top_srcdir)/include -Wall  -O3
endif

AM_CFLAGS += $(WIRINGPI_CFLAGS)

bin_PROGRAMS = srts_sender homeasy_sender signal_eventd
srts_sender_SOURCES = srts.c common.c srts_sender.c
srts_sender_LDADD = $(WIRINGPI_LIBS)

homeasy_sender_SOURCES = homeasy_sender.c common.c
homeasy_sender_LDADD = $(WIRINGPI_LIBS)

signal_eventd_SOURCES = signal_eventd.c common.c srts.c
signal_eventd_LDADD = $(WIRINGPI_LIBS)
