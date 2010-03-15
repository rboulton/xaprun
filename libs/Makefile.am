

include re2/mkincl.mak

INCLUDES = \
	-I$(srcdir)/http-parser \
	-I$(srcdir)/re2/include \
	-I$(srcdir)/re2/src

noinst_LIBRARIES = libxaprunlibs.a
noinst_HEADERS = \
	http-parser/http_parser.h \
	$(re2_internal_headers) \
	$(re2_public_headers)

AM_CXXFLAGS += -Wno-error

libxaprunlibs_a_SOURCES = \
	http-parser/http_parser.cc \
	$(re2_sources)

EXTRA_DIST = \
	$(re2_extra_files)
