#ifndef __WEBP_Handler_hpp__
#define __WEBP_Handler_hpp__ 1

#include "public/include/XMP_Const.h"
#include "public/include/XMP_Environment.h"

#include "XMPFiles/source/FormatSupport/WEBP_Support.hpp"
#include "XMPFiles/source/FormatSupport/TIFF_Support.hpp"

#include "source/XIO.hpp"

// File format handler for WEBP

extern XMPFileHandler* WEBP_MetaHandlerCTor(XMPFiles* parent);

extern bool WEBP_CheckFormat(XMP_FileFormat format, XMP_StringPtr filePath,
                             XMP_IO* fileRef, XMPFiles* parent);

static const XMP_OptionBits kWEBP_HandlerFlags =
    (kXMPFiles_CanInjectXMP | kXMPFiles_CanExpand | kXMPFiles_PrefersInPlace |
     kXMPFiles_AllowsOnlyXMP | kXMPFiles_ReturnsRawPacket |
     kXMPFiles_CanReconcile);

class WEBP_MetaHandler : public XMPFileHandler {
public:
    WEBP_MetaHandler(XMPFiles* parent);
    ~WEBP_MetaHandler();

    void CacheFileData();
    void ProcessXMP();
    void UpdateFile(bool doSafeUpdate);
    void WriteTempFile(XMP_IO* tempRef);

    WEBP::Container* mainChunk;
    WEBP::XMPChunk* xmpChunk;
    XMP_Int64 initialFileSize;
    TIFF_Manager* exifMgr;
	
};

#endif /* __WEBP_Handler_hpp__ */
