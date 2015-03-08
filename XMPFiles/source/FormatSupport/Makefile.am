#
# exempi - Makefile.am
#
# Copyright (C) 2007-2013 Hubert Figuiere
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

noinst_LTLIBRARIES = libformatsupport.la
noinst_HEADERS = IPTC_Support.hpp  PSIR_Support.hpp\
	Reconcile_Impl.hpp   RIFF_Support.hpp  XMPScanner.hpp\
	ID3_Support.hpp  PNG_Support.hpp   QuickTime_Support.hpp\
	ReconcileLegacy.hpp  TIFF_Support.hpp  GIF_Support.hpp


INCLUDES = -I$(top_srcdir) \
	-I$(top_srcdir)/public/include/

AM_CPPFLAGS = -Wno-multichar\
	-D@EXEMPI_PLATFORM_DEF@=1 -DXMP_IMPL=1 -DXMP_ClientBuild=0 \
	-DXMP_StaticBuild=1 \
	-D_FILE_OFFSET_BITS=64 -DHAVE_EXPAT_CONFIG_H=1 -DXML_STATIC=1

libformatsupport_la_SOURCES = \
	PackageFormat_Support.cpp PackageFormat_Support.hpp \
	PostScript_Support.cpp PostScript_Support.hpp \
	PSIR_FileWriter.cpp    Reconcile_Impl.cpp \
	ReconcileTIFF.cpp    TIFF_MemoryReader.cpp \
	IPTC_Support.cpp  PSIR_MemoryReader.cpp  ReconcileIPTC.cpp   \
	RIFF.cpp RIFF.hpp ID3_Support.cpp ID3_Support.hpp\
	RIFF_Support.cpp     TIFF_Support.cpp \
	ISOBaseMedia_Support.hpp ISOBaseMedia_Support.cpp \
	PNG_Support.cpp   ReconcileLegacy.cpp  \
	GIF_Support.cpp \
	SWF_Support.hpp SWF_Support.cpp\
	XDCAM_Support.hpp XDCAM_Support.cpp\
	TIFF_FileWriter.cpp  XMPScanner.cpp \
	MOOV_Support.cpp MOOV_Support.hpp \
	QuickTime_Support.cpp \
	ASF_Support.hpp ASF_Support.cpp \
	MacScriptExtracts.h \
	TimeConversionUtils.h TimeConversionUtils.cpp \
	AIFF/AIFFBehavior.cpp  AIFF/AIFFBehavior.h \
	AIFF/AIFFMetadata.cpp  AIFF/AIFFMetadata.h \
	AIFF/AIFFReconcile.cpp  AIFF/AIFFReconcile.h \
	IFF/ChunkController.cpp  IFF/Chunk.cpp  IFF/ChunkPath.cpp  IFF/IChunkBehavior.cpp  IFF/IChunkContainer.h \
	IFF/ChunkController.h    IFF/Chunk.h    IFF/ChunkPath.h    IFF/IChunkBehavior.h    IFF/IChunkData.h \
	WAVE/BEXTMetadata.cpp  WAVE/CartMetadata.h    WAVE/DISPMetadata.cpp  WAVE/INFOMetadata.h    WAVE/WAVEBehavior.cpp   WAVE/WAVEReconcile.h \
	WAVE/BEXTMetadata.h    WAVE/Cr8rMetadata.cpp  WAVE/DISPMetadata.h    WAVE/PrmLMetadata.cpp  WAVE/WAVEBehavior.h \
	WAVE/CartMetadata.cpp  WAVE/Cr8rMetadata.h    WAVE/INFOMetadata.cpp  WAVE/PrmLMetadata.h    WAVE/WAVEReconcile.cpp \
	WAVE/iXMLMetadata.h WAVE/iXMLMetadata.cpp \
	P2_Support.hpp P2_Support.cpp \
	$(NULL)


