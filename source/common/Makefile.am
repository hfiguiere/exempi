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



noinst_LTLIBRARIES = libxmpcommon.la

INCLUDES = -I$(top_srcdir)/public/include \
	-I$(top_srcdir)/public/include/client-glue

AM_CXXFLAGS = -Wno-multichar -Wno-implicit -Wno-ctor-dtor-privacy \
	-funsigned-char -fexceptions
AM_CPPFLAGS = \
	-D@EXEMPI_PLATFORM_DEF@=1 -DXMP_IMPL=1 -DXMP_ClientBuild=0 \
	-D_FILE_OFFSET_BITS=64 -DHAVE_EXPAT_CONFIG_H=1 -DXML_STATIC=1



noinst_HEADERS = UnicodeConversions.hpp ExpatAdapter.hpp EndianUtils.hpp\
	LargeFileAccess.hpp XMLParserAdapter.hpp


libxmpcommon_la_SOURCES = UnicodeInlines.incl_cpp UnicodeConversions.cpp\
	XML_Node.cpp LargeFileAccess.cpp 