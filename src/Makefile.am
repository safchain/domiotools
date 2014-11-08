if DEBUG
  AM_CFLAGS =-I$(top_srcdir)/include -Wall -g
else
  AM_CFLAGS =-I$(top_srcdir)/include -Wall  -O3
endif

AM_CFLAGS += $(WIRINGPI_CFLAGS)

bin_PROGRAMS = srts homeasy
srts_SOURCES = srts.c common.c
srts_LDADD = $(WIRINGPI_LIBS)

homeasy_SOURCES = homeasy.c common.c
homeasy_LDADD = $(WIRINGPI_LIBS)