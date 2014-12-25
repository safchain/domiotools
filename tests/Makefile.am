TESTS = check_domiotools
check_PROGRAMS = check_domiotools
check_domiotools_SOURCES = check_domiotools.c \
						   $(top_builddir)/src/urlparser.c \
						   $(top_builddir)/src/list.c \
						   $(top_builddir)/src/hash.c
check_domiotools_CFLAGS = @CHECK_CFLAGS@ -I$(top_builddir)/include
check_domiotools_LDADD = @CHECK_LIBS@
