## Process this file with automake to produce Makefile.in

# `make QUIET=' overrides `./configure --enable-quiet'.
# `make QUIET=y' overrides `./configure' without `--enable-quiet'.
LIBTOOL = @LIBTOOL@ $(QUIET:y=--quiet)

AUTOMAKE_OPTIONS = 1.5 subdir-objects
ACLOCAL_AMFLAGS = -I m4

INCLUDES = -I$(top_srcdir)/ext -I$(top_srcdir)/src

AM_CPPFLAGS = -DCONFIGFILE_SYSTEM=\"$(sysconfdir)/xappy.conf\"
AM_CXXFLAGS += $(XAPIAN_CXXFLAGS)

bin_PROGRAMS = xappy-server

dist_sysconf_DATA = xappy.conf

noinst_HEADERS = \
	ext/str.h \
	src/io_wrappers.h \
	src/logger.h \
	src/server.h \
	src/serverinternal.h \
	src/settings.h \
	src/signals.h \
	src/utils.h

AM_LDFLAGS =

xappy_server_SOURCES = \
	ext/str.cc \
	src/cli.cc \
	src/io_wrappers.cc \
	src/logger.cc \
	src/server.cc \
	src/settings.cc \
	src/signals.cc \
	src/utils.cc

xappy_server_LDADD = $(XAPIAN_LIBS)

DISTCHECK_CONFIGURE_FLAGS = "XAPIAN_CONFIG=$(XAPIAN_CONFIG)"
