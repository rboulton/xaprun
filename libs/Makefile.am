

include re2/mkincl.mak

INCLUDES = \
	-I$(srcdir)/http-parser \
	-I$(srcdir)/re2/include \
	-I$(srcdir)/re2/src \
	-I$(srcdir)/jsoncpp/include

noinst_LIBRARIES = libxaprunlibs.a
noinst_HEADERS = \
	http-parser/http_parser.h \
	$(re2_internal_headers) \
	$(re2_public_headers) \
	jsoncpp/include/json/autolink.h \
	jsoncpp/include/json/config.h \
	jsoncpp/include/json/features.h \
	jsoncpp/include/json/forwards.h \
	jsoncpp/include/json/json.h \
	jsoncpp/include/json/reader.h \
	jsoncpp/include/json/value.h \
	jsoncpp/include/json/writer.h \
	jsoncpp/src/lib_json/json_batchallocator.h \
	jsoncpp/src/lib_json/json_internalarray.inl \
	jsoncpp/src/lib_json/json_internalmap.inl \
	jsoncpp/src/lib_json/json_valueiterator.inl

AM_CXXFLAGS += -Wno-error

libxaprunlibs_a_SOURCES = \
	http-parser/http_parser.cc \
	$(re2_sources) \
	jsoncpp/src/lib_json/json_reader.cpp \
	jsoncpp/src/lib_json/json_value.cpp \
	jsoncpp/src/lib_json/json_writer.cpp


EXTRA_DIST = \
	$(re2_extra_files)
