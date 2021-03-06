## Process this file with automake to produce Makefile.in

# `make QUIET=' overrides `./configure --enable-quiet'.
# `make QUIET=y' overrides `./configure' without `--enable-quiet'.
LIBTOOL = @LIBTOOL@ $(QUIET:y=--quiet)

AUTOMAKE_OPTIONS = 1.5 subdir-objects
ACLOCAL_AMFLAGS = -I m4

SUBDIRS = libs .

INCLUDES = \
	-I$(top_srcdir)/ext \
	-I$(top_srcdir)/libs/http-parser \
	-I$(top_srcdir)/libs/re2/include \
	-I$(top_srcdir)/libs/jsoncpp/include \
	-I$(top_srcdir)/src

AM_CPPFLAGS = -DCONFIGFILE_SYSTEM=\"$(sysconfdir)/xaprun.conf\"
AM_CXXFLAGS += $(XAPIAN_CXXFLAGS)

bin_PROGRAMS = xaprun

dist_sysconf_DATA = xaprun.conf

noinst_HEADERS = \
	ext/str.h \
	src/server/io_wrappers.h \
	src/server/logger.h \
	src/server/server.h \
	src/server/serverinternal.h \
	src/server/signals.h \
	src/server/worker.h \
	src/server/workerpool.h \
	src/xappy/indexerworker.h \
	src/xappy/searchworker.h \
	src/settings.h \
	src/utils.h

xaprun_SOURCES = \
	ext/str.cc \
	src/server/io_wrappers.cc \
	src/server/logger.cc \
	src/server/server.cc \
	src/server/signals.cc \
	src/server/worker.cc \
	src/server/workerpool.cc \
	src/xappy/dispatch.cc \
	src/xappy/indexerworker.cc \
	src/xappy/searchworker.cc \
	src/cli.cc \
	src/settings.cc \
	src/utils.cc

xaprun_LDADD = $(XAPIAN_LIBS) libs/libxaprunlibs.a
xaprun_LDFLAGS = -pthread

DISTCHECK_CONFIGURE_FLAGS = "XAPIAN_CONFIG=$(XAPIAN_CONFIG)"
