


noinst_LTLIBRARIES = libxmpfilehandlers.la

INCLUDES = -I$(srcdir)/.. -I$(srcdir)/../FormatSupport \
	-I$(top_srcdir)/public/include/ -I$(top_srcdir)/build/ \
	-I$(top_srcdir)/third-party/MD5


AM_CPPFLAGS = \
	-DUNIX_ENV=1 -DXMP_IMPL=1 -DXMP_ClientBuild=0 \
	-D_FILE_OFFSET_BITS=64 -DHAVE_EXPAT_CONFIG_H=1 -DXML_STATIC=1

libxmpfilehandlers_la_SOURCES = AVI_Handler.cpp    InDesign_Handler.cpp  MPEG_Handler.cpp  PostScript_Handler.cpp  Scanner_Handler.cpp  Trivial_Handler.cpp \
Basic_Handler.cpp  JPEG_Handler.cpp      MP3_Handler.cpp  PNG_Handler.cpp   PSD_Handler.cpp         TIFF_Handler.cpp     WAV_Handler.cpp
