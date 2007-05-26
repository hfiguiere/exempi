
noinst_LTLIBRARIES = libformatsupport.la
noinst_HEADERS = EndianUtils.hpp  IPTC_Support.hpp  PSIR_Support.hpp\
	Reconcile_Impl.hpp   RIFF_Support.hpp  XMPScanner.hpp\
	ID3_Support.hpp  PNG_Support.hpp   QuickTime_Support.hpp\
	ReconcileLegacy.hpp  TIFF_Support.hpp


INCLUDES = -I$(srcdir)/.. -I$(srcdir)/../../common \
	-I$(top_srcdir)/public/include/ -I$(top_srcdir)/build/ \
	-I$(top_srcdir)/third-party/MD5


AM_CPPFLAGS = \
	-DUNIX_ENV=1 -DXMP_IMPL=1 -DXMP_ClientBuild=0 \
	-D_FILE_OFFSET_BITS=64 -DHAVE_EXPAT_CONFIG_H=1 -DXML_STATIC=1



libformatsupport_la_SOURCES = ID3_Support.cpp   PSIR_FileWriter.cpp    Reconcile_Impl.cpp \
	ReconcileTIFF.cpp    TIFF_MemoryReader.cpp \
	IPTC_Support.cpp  PSIR_MemoryReader.cpp  ReconcileIPTC.cpp   \
	RIFF_Support.cpp     TIFF_Support.cpp \
	PNG_Support.cpp   ReconcileLegacy.cpp  \
	TIFF_FileWriter.cpp  XMPScanner.cpp