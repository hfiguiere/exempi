#ifndef __MOV_Handler_hpp__
#define __MOV_Handler_hpp__	1

// =================================================================================================
// ADOBE SYSTEMS INCORPORATED
// Copyright 2002-2007 Adobe Systems Incorporated
// All Rights Reserved
//
// NOTICE: Adobe permits you to use, modify, and distribute this file in accordance with the terms
// of the Adobe license agreement accompanying it.
// =================================================================================================

#include "XMPFiles_Impl.hpp"

// Include these first to prevent collision with CIcon
#if XMP_WinBuild
#include "QTML.h"
#include "Movies.h"
#endif

#if XMP_MacBuild
#include <Movies.h>
#endif

// =================================================================================================
/// \file MOV_Handler.hpp
/// \brief File format handler for MOV.
///
/// This header ...
///
// =================================================================================================

extern XMPFileHandler * MOV_MetaHandlerCTor ( XMPFiles * parent );

extern bool MOV_CheckFormat (  XMP_FileFormat format,
							   XMP_StringPtr  filePath,
							   LFA_FileRef    fileRef,
							   XMPFiles *     parent );

static const XMP_OptionBits kMOV_HandlerFlags = (kXMPFiles_CanInjectXMP |
												 kXMPFiles_CanExpand |
												 kXMPFiles_PrefersInPlace |
												 kXMPFiles_AllowsOnlyXMP |
												 kXMPFiles_ReturnsRawPacket |
												 kXMPFiles_HandlerOwnsFile);
												// In the future, we'll add kXMPFiles_CanReconcile

class MOV_MetaHandler : public XMPFileHandler
{
public:

	MOV_MetaHandler ( XMPFiles * parent );
	~MOV_MetaHandler();

	void CacheFileData();

	void UpdateFile ( bool doSafeUpdate );
    void WriteFile  ( LFA_FileRef sourceRef, const std::string & sourcePath );

protected:

	bool		mQTInit;
	Handle		mMovieDataRef;
	DataHandler	mMovieDataHandler;
	Movie		mMovie;
	short		mMovieResourceID;
	long		mFilePermission;

	bool OpenMovie ( long inPermission );
	void CloseMovie();

};	// MOV_MetaHandler

// =================================================================================================

#endif /* __MOV_Handler_hpp__ */
