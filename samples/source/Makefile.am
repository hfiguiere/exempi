

noinst_PROGRAMS = xmpcoverage xmpfilescoverage dumpxmp dumpmainxmp

AM_CXXFLAGS = -fexceptions -funsigned-char -fPIC \
	-Wno-multichar -Wno-implicit -Wno-ctor-dtor-privacy

AM_CPPFLAGS = -DUNIX_ENV=1 -D_FILE_OFFSET_BITS=64
XMPLIBS = $(top_builddir)/source/XMPCore/libXMPCore.la \
	$(top_builddir)/source/XMPFiles/libXMPFiles.la \
	$(top_builddir)/source/common/libxmpcommon.la -lexpat \
	$(top_builddir)/third-party/MD5/libmd5.la

INCLUDES = -I$(top_srcdir)/public/include

noinst_HEADERS = XMPScanner.hpp

xmpcoverage_SOURCES = XMPCoreCoverage.cpp XMPScanner.cpp
xmpcoverage_LDADD = $(XMPLIBS)

xmpfilescoverage_SOURCES = XMPFilesCoverage.cpp
xmpfilescoverage_LDADD = $(XMPLIBS)

dumpxmp_SOURCES = DumpScannedXMP.cpp XMPScanner.cpp
dumpxmp_LDADD = $(XMPLIBS)

dumpmainxmp_SOURCES = DumpMainXMP.cpp
dumpmainxmp_LDADD = $(XMPLIBS)
