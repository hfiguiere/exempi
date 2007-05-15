


noinst_LTLIBRARIES = libXMPCore.la

INCLUDES = -I$(top_srcdir)/public/include \
	-I$(top_srcdir)/public/include/client-glue \
	-I$(top_srcdir)/build/ \
	-I$(srcdir)/../common/ \
	-I$(top_srcdir)/third-party/MD5

AM_CXXFLAGS = -Wno-multichar -Wno-implicit -Wno-ctor-dtor-privacy \
	-funsigned-char -fexceptions
AM_CPPFLAGS = -Wall \
	-DUNIX_ENV=1 -DXMP_IMPL=1 -DXMP_ClientBuild=0 \
	-D_FILE_OFFSET_BITS=64 -DHAVE_EXPAT_CONFIG_H=1 -DXML_STATIC=1



noinst_HEADERS = ExpatAdapter.hpp  XMLParserAdapter.hpp  \
	XMPCore_Impl.hpp  XMPIterator.hpp \
	XMPMeta.hpp  XMPUtils.hpp


libXMPCore_la_SOURCES = ExpatAdapter.cpp  \
	WXMPIterator.cpp  \
	WXMPUtils.cpp     \
	XMPIterator.cpp  \
	XMPMeta-GetSet.cpp  \
	XMPMeta-Serialize.cpp  \
	XMPUtils-FileInfo.cpp \
	ParseRDF.cpp      \
	WXMPMeta.cpp     \
	XMPCore_Impl.cpp  \
	XMPMeta.cpp      \
	XMPMeta-Parse.cpp   \
	XMPUtils.cpp

#libXMPCore_la_LIBADD = -lexpat $(top_builddir)/third-party/MD5/libmd5.la \
	../common/libxmpcommon.la
#libXMPCore_la_LDFLAGS = -version-info @LIBXMPCORE_VERSION_INFO@
