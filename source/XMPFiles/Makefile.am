
SUBDIRS = FileHandlers FormatSupport

lib_LTLIBRARIES = libXMPFiles.la

INCLUDES = -I$(top_srcdir)/public/include \
	-I$(top_srcdir)/public/include/client-glue \
	-I$(top_srcdir)/build/ \
	-I$(srcdir)/FormatSupport \
	-I$(srcdir)/../common/ \
	-I$(top_srcdir)/third-party/MD5


AM_CXXFLAGS = -Wno-multichar -Wno-implicit -Wno-ctor-dtor-privacy \
	-funsigned-char -fexceptions
AM_CPPFLAGS = \
	-DUNIX_ENV=1 -DXMP_IMPL=1 -DXMP_ClientBuild=0 \
	-D_FILE_OFFSET_BITS=64 -DHAVE_EXPAT_CONFIG_H=1 -DXML_STATIC=1 \
	-DDISABLE_QUICKTIME



noinst_HEADERS = XMPFiles.hpp XMPFiles_Impl.hpp

libXMPFiles_la_SOURCES = WXMPFiles.cpp XMPFiles.cpp \
	XMPFiles_Impl.cpp


libXMPFiles_la_LIBADD = -lexpat ./FileHandlers/libxmpfilehandlers.la \
	./FormatSupport/libformatsupport.la
libXMPFiles_la_LDFLAGS = -version-info @LIBXMPCORE_VERSION_INFO@
