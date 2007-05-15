

noinst_PROGRAMS = xmpcoverage xmpfilescoverage dumpxmp dumpmainxmp

AM_CXXFLAGS = -fexceptions -funsigned-char -fPIC \
	-Wno-multichar -Wno-implicit -Wno-ctor-dtor-privacy

AM_CPPFLAGS = -DUNIX_ENV=1 -D_FILE_OFFSET_BITS=64
AM_LDFLAGS = $(top_builddir)/source/XMPCore/libXMPCore.la \
	$(top_builddir)/source/XMPFiles/libXMPFiles.la \
	$(top_builddir)/source/common/libxmpcommon.la -lexpat \
	$(top_builddir)/third-party/MD5/libmd5.la

INCLUDES = -I$(top_srcdir)/public/include

noinst_HEADERS = XMPScanner.hpp

xmpcoverage_SOURCES = XMPCoreCoverage.cpp XMPScanner.cpp
xmpfilescoverage_SOURCES = XMPFilesCoverage.cpp
dumpxmp_SOURCES = DumpScannedXMP.cpp XMPScanner.cpp
dumpmainxmp_SOURCES = DumpMainXMP.cpp
