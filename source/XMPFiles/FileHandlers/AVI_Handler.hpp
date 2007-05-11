#ifndef __AVI_Handler_hpp__
#define __AVI_Handler_hpp__	1

// =================================================================================================
// ADOBE SYSTEMS INCORPORATED
// Copyright 2002-2007 Adobe Systems Incorporated
// All Rights Reserved
//
// NOTICE: Adobe permits you to use, modify, and distribute this file in accordance with the terms
// of the Adobe license agreement accompanying it.
// =================================================================================================

#include "XMPFiles_Impl.hpp"

// =================================================================================================
/// \file AVI_Handler.hpp
/// \brief File format handler for AVI.
///
/// This header ...
///
// =================================================================================================

extern XMPFileHandler * AVI_MetaHandlerCTor ( XMPFiles * parent );

extern bool AVI_CheckFormat ( XMP_FileFormat format,
							  XMP_StringPtr  filePath,
			                  LFA_FileRef    fileRef,
			                  XMPFiles *     parent );

static const XMP_OptionBits kAVI_HandlerFlags = (kXMPFiles_CanInjectXMP | 
												 kXMPFiles_CanExpand | 
												 kXMPFiles_PrefersInPlace |
												 kXMPFiles_AllowsOnlyXMP | 
												 kXMPFiles_ReturnsRawPacket);
												// In the future, we'll add kXMPFiles_CanReconcile

class AVI_MetaHandler : public XMPFileHandler
{
public:

	AVI_MetaHandler ( XMPFiles * parent );
	~AVI_MetaHandler();

	void CacheFileData();

	void UpdateFile ( bool doSafeUpdate );
    void WriteFile  ( LFA_FileRef sourceRef, const std::string & sourcePath );

};	// AVI_MetaHandler

// =================================================================================================

#endif /* __AVI_Handler_hpp__ */
