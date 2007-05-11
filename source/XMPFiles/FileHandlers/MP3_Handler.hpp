#ifndef __MP3_Handler_hpp__
#define __MP3_Handler_hpp__	1

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
/// \file MP3_Handler.hpp
/// \brief File format handler for MP3.
///
/// This header ...
///
// =================================================================================================

extern XMPFileHandler * MP3_MetaHandlerCTor ( XMPFiles * parent );

extern bool MP3_CheckFormat ( XMP_FileFormat format,
							  XMP_StringPtr  filePath,
			                  LFA_FileRef    fileRef,
			                  XMPFiles *     parent );

static const XMP_OptionBits kMP3_HandlerFlags = (kXMPFiles_CanInjectXMP | 
												 kXMPFiles_CanExpand | 
												 kXMPFiles_PrefersInPlace |
												 kXMPFiles_AllowsOnlyXMP | 
												 kXMPFiles_ReturnsRawPacket);
												// In the future, we'll add kXMPFiles_CanReconcile

class MP3_MetaHandler : public XMPFileHandler
{
public:

	MP3_MetaHandler ( XMPFiles * parent );
	~MP3_MetaHandler();

	void CacheFileData();

	void UpdateFile ( bool doSafeUpdate );
    void WriteFile  ( LFA_FileRef sourceRef, const std::string & sourcePath );

private:
	bool LoadPropertyFromID3(LFA_FileRef inFileRef, char *strFrame, char *strNameSpace, char *strXMPTag, bool fLocalText = false);

};	// MP3_MetaHandler

// =================================================================================================

#endif /* __MP3_Handler_hpp__ */
