#ifndef __MPEG4_Handler_hpp__
#define __MPEG4_Handler_hpp__	1

// =================================================================================================
// ADOBE SYSTEMS INCORPORATED
// Copyright 2002-2007 Adobe Systems Incorporated
// All Rights Reserved
//
// NOTICE: Adobe permits you to use, modify, and distribute this file in accordance with the terms
// of the Adobe license agreement accompanying it.
// =================================================================================================

#include "XMPFiles_Impl.hpp"

//  ================================================================================================
/// \file MPEG4_Handler.hpp
/// \brief File format handler for MPEG-4.
///
/// This header ...
///
//  ================================================================================================

extern XMPFileHandler * MPEG4_MetaHandlerCTor ( XMPFiles * parent );

extern bool MPEG4_CheckFormat ( XMP_FileFormat format,
								 XMP_StringPtr  filePath,
								 LFA_FileRef    fileRef,
								 XMPFiles *     parent );

static const XMP_OptionBits kMPEG4_HandlerFlags = ( kXMPFiles_CanInjectXMP |
													kXMPFiles_CanExpand |
													kXMPFiles_CanRewrite |
													kXMPFiles_PrefersInPlace |
													kXMPFiles_CanReconcile |
													kXMPFiles_AllowsOnlyXMP |
													kXMPFiles_ReturnsRawPacket |
													kXMPFiles_AllowsSafeUpdate
												  );

class MPEG4_MetaHandler : public XMPFileHandler
{
public:

	void CacheFileData();
	void ProcessXMP();
	
	void UpdateFile ( bool doSafeUpdate );
    void WriteFile  ( LFA_FileRef sourceRef, const std::string & sourcePath );

	MPEG4_MetaHandler ( XMPFiles * _parent );
	virtual ~MPEG4_MetaHandler();

private:

	MPEG4_MetaHandler() : xmpBoxPos(0) {};	// Hidden on purpose.

	void MakeLegacyDigest ( std::string * digestStr );
	void PickNewLocation();
	
	XMP_Uns64 xmpBoxPos;	// The file offset of the XMP box (the size field, not the content).
	
	std::string mvhdBox;	// ! Both contain binary data, but std::string is handy.
	std::vector<std::string> cprtBoxes;

};	// MPEG4_MetaHandler

// =================================================================================================

#endif // __MPEG4_Handler_hpp__
