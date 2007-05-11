#ifndef __WAV_Handler_hpp__
#define __WAV_Handler_hpp__	1

// =================================================================================================
// ADOBE SYSTEMS INCORPORATED
// Copyright 2002-2007 Adobe Systems Incorporated
// All Rights Reserved
//
// NOTICE: Adobe permits you to use, modify, and distribute this file in accordance with the terms
// of the Adobe license agreement accompanying it.
// =================================================================================================

#include "XMPFiles_Impl.hpp"

#include "MD5.h"

// =================================================================================================
/// \file WAV_Handler.hpp
/// \brief File format handler for WAV.
///
/// This header ...
///
// =================================================================================================

namespace RIFF_Support
{
	class RiffState;
}

extern XMPFileHandler * WAV_MetaHandlerCTor ( XMPFiles * parent );

extern bool WAV_CheckFormat ( XMP_FileFormat format,
							  XMP_StringPtr  filePath,
			                  LFA_FileRef    fileRef,
			                  XMPFiles *     parent );

static const XMP_OptionBits kWAV_HandlerFlags = ( kXMPFiles_CanInjectXMP | 
												  kXMPFiles_CanExpand | 
												  kXMPFiles_PrefersInPlace |
												  kXMPFiles_AllowsOnlyXMP | 
												  kXMPFiles_ReturnsRawPacket );
												  // In the future, we'll add kXMPFiles_CanReconcile

class WAV_MetaHandler : public XMPFileHandler
{
public:

	WAV_MetaHandler ( XMPFiles * parent );
	~WAV_MetaHandler();

	void CacheFileData();

	void UpdateFile ( bool doSafeUpdate );
    void WriteFile  ( LFA_FileRef sourceRef, const std::string & sourcePath );

private:

	void ImportLegacyItem ( RIFF_Support::RiffState& inOutRiffState, XMP_Uns32 tagID, XMP_Uns32 parentID,
							XMP_StringPtr xmpNS, XMP_StringPtr xmpProp, bool keepExistingXMP, bool langAlt = false );

	void PrepareLegacyExport ( XMP_StringPtr xmpNS, XMP_StringPtr xmpProp, XMP_Uns32 legacyID, std::string * legacyStr,
							   std::string  * digestStr, MD5_CTX * md5, bool langAlt = false );

	void UTF8ToMBCS ( std::string * str );
	void MBCSToUTF8 ( std::string * str );

};	// WAV_MetaHandler

// =================================================================================================

#endif /* __WAV_Handler_hpp__ */
