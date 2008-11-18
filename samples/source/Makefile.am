#
# exempi - Makefile.am
#
# Copyright (C) 2007 Hubert Figuiere
# All rights reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions
# are met:
#
# 1 Redistributions of source code must retain the above copyright
# notice, this list of conditions and the following disclaimer.
# 
# 2 Redistributions in binary form must reproduce the above copyright
# notice, this list of conditions and the following disclaimer in the
# documentation and/or other materials provided with the
# distribution.
#
# 3 Neither the name of the Authors, nor the names of its
# contributors may be used to endorse or promote products derived
# from this software wit hout specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
# "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
# LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
# FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
# COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
# INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
# (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
# SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
# HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
# STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
# ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
# OF THE POSSIBILITY OF SUCH DAMAGE.
#


noinst_PROGRAMS = xmpcoverage xmpfilescoverage dumpxmp dumpmainxmp\
	customschema \
	modifyingxmp \
	readingxmp \
	$(NULL)

AM_CXXFLAGS = -fexceptions -funsigned-char -fPIC \
	-Wno-multichar -Wno-implicit -Wno-ctor-dtor-privacy

AM_CPPFLAGS = -D@EXEMPI_PLATFORM_DEF@=1 -D_FILE_OFFSET_BITS=64
XMPLIBS = $(top_builddir)/source/XMPCore/libXMPCore.la \
	$(top_builddir)/source/XMPFiles/libXMPFiles.la \
	$(top_builddir)/source/common/libxmpcommon.la -lexpat -lz \
	$(top_builddir)/third-party/MD5/libmd5.la

INCLUDES = -I$(top_srcdir)/public/include

noinst_HEADERS = XMPScanner.hpp

customschema_SOURCES = CustomSchema.cpp
customschema_LDADD = $(XMPLIBS)

modifyingxmp_SOURCES = ModifyingXMP.cpp
modifyingxmp_LDADD = $(XMPLIBS)

readingxmp_SOURCES = ReadingXMP.cpp
readingxmp_LDADD = $(XMPLIBS)

xmpcoverage_SOURCES = XMPCoreCoverage.cpp XMPScanner.cpp
xmpcoverage_LDADD = $(XMPLIBS)

xmpfilescoverage_SOURCES = XMPFilesCoverage.cpp
xmpfilescoverage_LDADD = $(XMPLIBS)

dumpxmp_SOURCES = DumpScannedXMP.cpp XMPScanner.cpp
dumpxmp_LDADD = $(XMPLIBS)

dumpmainxmp_SOURCES = DumpMainXMP.cpp
dumpmainxmp_LDADD = $(XMPLIBS)
