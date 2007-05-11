#ifndef __MPEG_Handler_hpp__
#define __MPEG_Handler_hpp__	1

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
/// \file MPEG_Handler.hpp
/// \brief File format handler for MPEG.
///
/// This header ...
///
// =================================================================================================

extern XMPFileHandler * MPEG_MetaHandlerCTor ( XMPFiles * parent );

extern bool MPEG_CheckFormat ( XMP_FileFormat format,
							   XMP_StringPtr  filePath,
							   LFA_FileRef    fileRef,
							   XMPFiles *     parent);

static const XMP_OptionBits kMPEG_HandlerFlags = ( kXMPFiles_CanInjectXMP |
												   kXMPFiles_CanExpand |
												   kXMPFiles_CanRewrite |
												   kXMPFiles_AllowsOnlyXMP |
												   kXMPFiles_ReturnsRawPacket |
												   kXMPFiles_HandlerOwnsFile |
												   kXMPFiles_AllowsSafeUpdate | 
												   kXMPFiles_UsesSidecarXMP );

class MPEG_MetaHandler : public XMPFileHandler
{
public:

	std::string sidecarPath;

	MPEG_MetaHandler ( XMPFiles * parent );
	~MPEG_MetaHandler();

	void CacheFileData();

	void UpdateFile ( bool doSafeUpdate );
    void WriteFile  ( LFA_FileRef sourceRef, const std::string & sourcePath );

};	// MPEG_MetaHandler

// =================================================================================================

#endif /* __MPEG_Handler_hpp__ */
