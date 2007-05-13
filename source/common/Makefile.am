


noinst_LTLIBRARIES = libxmpcommon.la

INCLUDES = -I$(top_srcdir)/public/include \
	-I$(top_srcdir)/public/include/client-glue

AM_CXXFLAGS = -Wno-multichar -Wno-implicit -Wno-ctor-dtor-privacy \
	-funsigned-char -fexceptions
AM_CPPFLAGS = \
	-DUNIX_ENV=1 -DXMP_IMPL=1 -DXMP_ClientBuild=0 \
	-D_FILE_OFFSET_BITS=64 -DHAVE_EXPAT_CONFIG_H=1 -DXML_STATIC=1



noinst_HEADERS = UnicodeConversions.hpp


libxmpcommon_la_SOURCES = UnicodeInlines.incl_cpp UnicodeConversions.cpp