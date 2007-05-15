
SUBDIRS = tests

openxmpdir = $(includedir)/@OPENXMP_INCLUDE_BASE@/openxmp
openxmp_HEADERS = xmp.h

INCLUDES = -I$(top_srcdir)/public/include

AM_CPPFLAGS = -Wall
lib_LTLIBRARIES = libopenxmp.la

libopenxmp_la_SOURCES = openxmp.cpp

libopenxmp_la_LIBADD = $(top_builddir)/source/common/libxmpcommon.la \
	$(top_builddir)/source/XMPCore/libXMPCore.la \
	$(top_builddir)/source/XMPFiles/libXMPFiles.la \
	$(top_builddir)/third-party/MD5/libmd5.la
libopenxmp_la_LDFLAGS = -version-info @LIBXMPCORE_VERSION_INFO@