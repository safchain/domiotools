AM_CFLAGS = -I$(top_srcdir)/include -Wall @GCOV_CFLAGS@

if DEBUG
	AM_CFLAGS += -g
else
	AM_CFLAGS += -O3
endif

CLEANFILES = *~ *.gcno *.gcda *.gcov

bin_PROGRAMS = srts_sender homeasy_sender
srts_sender_SOURCES = $(top_srcdir)/src/srts.c \
                      $(top_srcdir)/src/common.c \
                      $(top_srcdir)/src/logging.c \
                      $(top_srcdir)/src/mem.c \
                      srts_sender.c
srts_sender_LDADD = -lwiringPi -lpthread @GCOV_LIBS@

homeasy_sender_SOURCES = $(top_srcdir)/src/common.c \
                         $(top_srcdir)/src/mem.c \
                         $(top_srcdir)/src/homeasy.c \
                         $(top_srcdir)/src/logging.c \
                         homeasy_sender.c
homeasy_sender_LDADD = -lwiringPi -lpthread @GCOV_LIBS@
