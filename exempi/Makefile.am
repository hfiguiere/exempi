
SUBDIRS = tests

exempidir = $(includedir)/@EXEMPI_INCLUDE_BASE@/exempi
exempi_HEADERS = xmp.h

INCLUDES = -I$(top_srcdir)/public/include

AM_CPPFLAGS = -Wall
lib_LTLIBRARIES = libexempi.la

libexempi_la_SOURCES = exempi.cpp

libexempi_la_LIBADD = $(top_builddir)/source/common/libxmpcommon.la \
	$(top_builddir)/source/XMPCore/libXMPCore.la \
	$(top_builddir)/source/XMPFiles/libXMPFiles.la \
	$(top_builddir)/third-party/MD5/libmd5.la
libexempi_la_LDFLAGS = -version-info @EXEMPI_VERSION_INFO@