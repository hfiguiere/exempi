#ifndef __ID3_Support_hpp__
#define __ID3_Support_hpp__ 1

// =================================================================================================
// ADOBE SYSTEMS INCORPORATED
// Copyright 2002-2007 Adobe Systems Incorporated
// All Rights Reserved
//
// NOTICE: Adobe permits you to use, modify, and distribute this file in accordance with the terms
// of the Adobe license agreement accompanying it.
// =================================================================================================

#include "XMP_Environment.h"	// ! This must be the first include.

#include <vector>

#include "XMP_Const.h"
#include "XMPFiles_Impl.hpp"

#define TAG_MAX_SIZE 5024

namespace ID3_Support 
{

	bool GetMetaData ( LFA_FileRef inFileRef, char* buffer, unsigned long* pBufferSize, XMP_Int64* fileOffset );
	bool SetMetaData ( LFA_FileRef inFileRef, char * buffer, unsigned long bufferSize,
					   char * strReconciliatedFrames, unsigned long dwReconciliatedFramesSize, bool fRecon );

	bool GetTagInfo ( LFA_FileRef inFileRef, XMP_Uns8 & v1, XMP_Uns8 & v2, XMP_Uns8 & flags, unsigned long & dwTagSize );
	bool FindID3Tag ( LFA_FileRef inFileRef, unsigned long & dwLen, XMP_Uns8 & bMajorVer );

	bool AddXMPTagToID3Buffer ( char * strCur, unsigned long * pdwCurOffset, unsigned long dwMaxSize,
								XMP_Uns8 bVersion, char * strFrameName, const char * strXMPTag, unsigned long dwXMPLength );

	bool GetFrameData ( LFA_FileRef inFileRef, char * strFrame, char * buffer, unsigned long & dwBufferSize );

} // namespace ID3_Support

#endif	// __ID3_Support_hpp__
