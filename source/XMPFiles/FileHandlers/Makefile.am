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



noinst_LTLIBRARIES = libxmpfilehandlers.la

INCLUDES = -I$(srcdir)/.. -I$(srcdir)/../FormatSupport \
	-I$(top_srcdir)/source/common/ \
	-I$(top_srcdir)/public/include/ -I$(top_srcdir)/build/ \
	-I$(top_srcdir)/third-party/MD5


AM_CPPFLAGS = \
	-D@EXEMPI_PLATFORM_DEF@=1 -DXMP_IMPL=1 -DXMP_ClientBuild=0 \
	-D_FILE_OFFSET_BITS=64 -DHAVE_EXPAT_CONFIG_H=1 -DXML_STATIC=1

libxmpfilehandlers_la_SOURCES = \
ASF_Handler.cpp ASF_Handler.hpp\
AVCHD_Handler.cpp AVCHD_Handler.hpp\
AVI_Handler.cpp AVI_Handler.hpp\
Basic_Handler.cpp Basic_Handler.hpp\
FLV_Handler.cpp FLV_Handler.hpp\
GIF_Handler.cpp GIF_Handler.hpp\
InDesign_Handler.cpp InDesign_Handler.hpp\
JPEG_Handler.cpp JPEG_Handler.hpp\
MP3_Handler.cpp MP3_Handler.hpp\
MOV_Handler.cpp MOV_Handler.hpp\
MPEG2_Handler.cpp MPEG2_Handler.hpp \
MPEG4_Handler.cpp MPEG4_Handler.hpp \
P2_Handler.cpp P2_Handler.hpp\
PNG_Handler.cpp PNG_Handler.hpp\
PostScript_Handler.cpp PostScript_Handler.hpp\
PSD_Handler.cpp PSD_Handler.hpp\
Scanner_Handler.cpp Scanner_Handler.hpp\
SonyHDV_Handler.cpp SonyHDV_Handler.hpp\
SWF_Handler.cpp SWF_Handler.hpp\
TIFF_Handler.cpp TIFF_Handler.hpp\
Trivial_Handler.cpp Trivial_Handler.hpp\
UCF_Handler.cpp UCF_Handler.hpp\
WAV_Handler.cpp WAV_Handler.hpp\
XDCAMEX_Handler.cpp XDCAMEX_Handler.hpp\
XDCAM_Handler.cpp XDCAM_Handler.hpp\
$(NULL)
