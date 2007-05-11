// =================================================================================================
// ADOBE SYSTEMS INCORPORATED
// Copyright 2006-2007 Adobe Systems Incorporated
// All Rights Reserved
//
// NOTICE: Adobe permits you to use, modify, and distribute this file in accordance with the terms
// of the Adobe license agreement accompanying it.
// =================================================================================================

#include "XMP_Environment.h"	// ! This must be the first include.

#include "Reconcile_Impl.hpp"

#include "UnicodeConversions.hpp"

#if XMP_WinBuild
#elif XMP_MacBuild
	#include "UnicodeConverter.h"
#endif

// =================================================================================================
/// \file Reconcile_Impl.cpp
/// \brief Implementation utilities for the legacy metadata reconciliation support.
///
// =================================================================================================

// =================================================================================================
// IsASCII
// =======
//
// See if a string is 7 bit ASCII.

static inline bool IsASCII ( const void * strPtr, size_t strLen )
{
	
	for ( const XMP_Uns8 * strPos = (XMP_Uns8*)strPtr; strLen > 0; --strLen, ++strPos ) {
		if ( *strPos >= 0x80 ) return false;
	}
	
	return true;

}	// IsASCII

// =================================================================================================
// ReconcileUtils::IsUTF8
// ======================
//
// See if a string contains valid UTF-8. Allow nul bytes, they can appear inside of multi-part Exif
// strings. We don't use CodePoint_from_UTF8_Multi in UnicodeConversions because it throws an
// exception for non-Unicode and we don't need to actually compute the code points.

bool ReconcileUtils::IsUTF8 ( const void * utf8Ptr, size_t utf8Len )
{
	const XMP_Uns8 * utf8Pos = (XMP_Uns8*)utf8Ptr;
	const XMP_Uns8 * utf8End = utf8Pos + utf8Len;
	
	while ( utf8Pos < utf8End ) {
	
		if ( *utf8Pos < 0x80 ) {
		
			++utf8Pos;	// ASCII is UTF-8, tolerate nuls.
		
		} else {
		
			// -------------------------------------------------------------------------------------
			// We've got a multibyte UTF-8 character. The first byte has the number of bytes as the
			// number of high order 1 bits. The remaining bytes must have 1 and 0 as the top 2 bits.
	
			#if 0	// *** This might be a more effcient way to count the bytes.
				static XMP_Uns8 kByteCounts[16] = { 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 2, 2, 3, 4 };
				size_t bytesNeeded = kByteCounts [ *utf8Pos >> 4 ];
				if ( (bytesNeeded < 2) || ((bytesNeeded == 4) && ((*utf8Pos & 0x08) != 0)) ) return false;
				if ( (utf8Pos + bytesNeeded) > utf8End ) return false;
			#endif
		
			size_t bytesNeeded = 0;	// Count the high order 1 bits in the first byte.
			for ( XMP_Uns8 temp = *utf8Pos; temp > 0x7F; temp = temp << 1 ) ++bytesNeeded;
				// *** Consider CPU-specific assembly inline, e.g. cntlzw on PowerPC.
			
			if ( (bytesNeeded < 2) || (bytesNeeded > 4) || ((utf8Pos+bytesNeeded) > utf8End) ) return false;
			
			for ( --bytesNeeded, ++utf8Pos; bytesNeeded > 0; --bytesNeeded, ++utf8Pos ) {
				if ( (*utf8Pos >> 6) != 2 ) return false;
			}
		
		}
	
	}
	
	return true;

}	// ReconcileUtils::IsUTF8

// =================================================================================================
// UTF8ToHostEncoding
// ==================

#if XMP_WinBuild

	static void UTF8ToWinEncoding ( UINT codePage,
									const XMP_Uns8 * utf8Ptr, size_t utf8Len, std::string * host )
	{
	
		std::string utf16;	// WideCharToMultiByte wants native UTF-16.
		ToUTF16Native ( (UTF8Unit*)utf8Ptr, utf8Len, &utf16 );
	
		LPCWSTR utf16Ptr = (LPCWSTR) utf16.c_str();
		size_t  utf16Len = utf16.size() / 2;
	
		int hostLen = WideCharToMultiByte ( codePage, 0, utf16Ptr, utf16Len, 0, 0, 0, 0 );
		host->assign ( hostLen, ' ' );	// Allocate space for the results.
	
		(void) WideCharToMultiByte ( codePage, 0, utf16Ptr, utf16Len, (LPSTR)host->data(), hostLen, 0, 0 );
		XMP_Assert ( hostLen == host->size() );
	
	}	// UTF8ToWinEncoding

#elif XMP_MacBuild

	static void UTF8ToMacEncoding ( TextEncoding & destEncoding,
									const XMP_Uns8 * utf8Ptr, size_t utf8Len, std::string * host )
	{
		OSStatus err;
		
		UnicodeMapping mappingInfo;
		mappingInfo.mappingVersion  = kUnicodeUseLatestMapping;
		mappingInfo.otherEncoding   = GetTextEncodingBase ( destEncoding );
		mappingInfo.unicodeEncoding = CreateTextEncoding ( kTextEncodingUnicodeDefault,
														   kUnicodeNoSubset, kUnicodeUTF8Format );
		
		UnicodeToTextInfo converterInfo;
		err = CreateUnicodeToTextInfo ( &mappingInfo, &converterInfo );
		if ( err != noErr ) XMP_Throw ( "CreateUnicodeToTextInfo failed", kXMPErr_ExternalFailure );
		
		try {	// ! Need to call DisposeUnicodeToTextInfo before exiting.
		
			OptionBits convFlags = kUnicodeUseFallbacksMask | 
								   kUnicodeLooseMappingsMask |  kUnicodeDefaultDirectionMask;
			ByteCount bytesRead, bytesWritten;
	
			enum { kBufferLen = 1000 };	// Ought to be enough in practice, without using too much stack.
			char buffer [kBufferLen];
			
			host->reserve ( utf8Len );	// As good a guess as any.
		
			while ( utf8Len > 0 ) {
				// Ignore all errors from ConvertFromUnicodeToText. It returns info like "output 
				// buffer full" or "use substitution" as errors.
				err = ConvertFromUnicodeToText ( converterInfo, utf8Len, (UniChar*)utf8Ptr, convFlags,
												 0, 0, 0, 0, kBufferLen, &bytesRead, &bytesWritten, buffer );
				if ( bytesRead == 0 ) break;	// Make sure forward progress happens.
				host->append ( &buffer[0], bytesWritten );
				utf8Ptr += bytesRead;
				utf8Len -= bytesRead;
			}
		
			DisposeUnicodeToTextInfo ( &converterInfo );
	
		} catch ( ... ) {
	
			DisposeUnicodeToTextInfo ( &converterInfo );
			throw;
	
		}
	
	}	// UTF8ToMacEncoding

#elif XMP_UNIXBuild

	#error "UTF8ToHostEncoding is not implemented for UNIX"
	// *** A nice definition of Windows 1252 is at http://www.microsoft.com/globaldev/reference/sbcs/1252.mspx
	// *** We should code our own conversions for this, and use it for UNIX - unless better POSIX routines exist.

#endif

// =================================================================================================
// ReconcileUtils::UTF8ToLocal
// ===========================

void ReconcileUtils::UTF8ToLocal ( const void * _utf8Ptr, size_t utf8Len, std::string * local )
{
	const XMP_Uns8* utf8Ptr = (XMP_Uns8*)_utf8Ptr;

	local->erase();
	
	if ( IsASCII ( utf8Ptr, utf8Len ) ) {
		local->assign ( (const char *)utf8Ptr, utf8Len );
		return;
	}
	
	#if XMP_WinBuild
	
		UTF8ToWinEncoding ( CP_ACP, utf8Ptr, utf8Len, local );
	
	#elif XMP_MacBuild
	
		OSStatus err;
		
		TextEncoding localEncoding;
		err = UpgradeScriptInfoToTextEncoding ( smSystemScript,
												kTextLanguageDontCare, kTextRegionDontCare, 0, &localEncoding );
		if ( err != noErr ) XMP_Throw ( "UpgradeScriptInfoToTextEncoding failed", kXMPErr_ExternalFailure );
		
		UTF8ToMacEncoding ( localEncoding, utf8Ptr, utf8Len, local );
	
	#elif XMP_UNIXBuild
	
		#error "UTF8ToLocal is not implemented for UNIX"
	
	#endif

}	// ReconcileUtils::UTF8ToLocal

// =================================================================================================
// ReconcileUtils::UTF8ToLatin1
// ============================
//
// Actually to the Windows code page 1252 superset of 8859-1.

void ReconcileUtils::UTF8ToLatin1 ( const void * _utf8Ptr, size_t utf8Len, std::string * latin1 )
{
	const XMP_Uns8* utf8Ptr = (XMP_Uns8*)_utf8Ptr;

	latin1->erase();
	
	if ( IsASCII ( utf8Ptr, utf8Len ) ) {
		latin1->assign ( (const char *)utf8Ptr, utf8Len );
		return;
	}
	
	#if XMP_WinBuild
	
		UTF8ToWinEncoding ( 1252, utf8Ptr, utf8Len, latin1 );
	
	#elif XMP_MacBuild
	
		TextEncoding latin1Encoding;
		latin1Encoding = CreateTextEncoding ( kTextEncodingWindowsLatin1,
											  kTextEncodingDefaultVariant, kTextEncodingDefaultFormat );
		
		UTF8ToMacEncoding ( latin1Encoding, utf8Ptr, utf8Len, latin1 );
	
	#elif XMP_UNIXBuild
	
		#error "UTF8ToLatin1 is not implemented for UNIX"
	
	#endif

}	// ReconcileUtils::UTF8ToLatin1

// =================================================================================================
// HostEncodingToUTF8
// ==================

#if XMP_WinBuild

	static void WinEncodingToUTF8 ( UINT codePage,
									const XMP_Uns8 * hostPtr, size_t hostLen, std::string * utf8 )
	{

		size_t utf16Len = MultiByteToWideChar ( codePage, 0, (LPCSTR)hostPtr, hostLen, 0, 0 );
		std::vector<UTF16Unit> utf16 ( utf16Len, 0 );	// MultiByteToWideChar returns native UTF-16.

		(void) MultiByteToWideChar ( codePage, 0, (LPCSTR)hostPtr, hostLen, (LPWSTR)&utf16[0], utf16Len );
		FromUTF16Native ( &utf16[0], utf16Len, utf8 );
	
	}	// WinEncodingToUTF8

#elif XMP_MacBuild

	static void MacEncodingToUTF8 ( TextEncoding & srcEncoding,
									const XMP_Uns8 * hostPtr, size_t hostLen, std::string * utf8 )
	{
		OSStatus err;
		
		UnicodeMapping mappingInfo;
		mappingInfo.mappingVersion  = kUnicodeUseLatestMapping;
		mappingInfo.otherEncoding   = GetTextEncodingBase ( srcEncoding );
		mappingInfo.unicodeEncoding = CreateTextEncoding ( kTextEncodingUnicodeDefault,
														   kUnicodeNoSubset, kUnicodeUTF8Format );
		
		TextToUnicodeInfo converterInfo;
		err = CreateTextToUnicodeInfo ( &mappingInfo, &converterInfo );
		if ( err != noErr ) XMP_Throw ( "CreateTextToUnicodeInfo failed", kXMPErr_ExternalFailure );
		
		try {	// ! Need to call DisposeTextToUnicodeInfo before exiting.
		
			ByteCount bytesRead, bytesWritten;

			enum { kBufferLen = 1000 };	// Ought to be enough in practice, without using too much stack.
			char buffer [kBufferLen];
			
			utf8->reserve ( hostLen );	// As good a guess as any.
		
			while ( hostLen > 0 ) {
				// Ignore all errors from ConvertFromTextToUnicode. It returns info like "output 
				// buffer full" or "use substitution" as errors.
				err = ConvertFromTextToUnicode ( converterInfo, hostLen, hostPtr, kNilOptions,
												 0, 0, 0, 0, kBufferLen, &bytesRead, &bytesWritten, (UniChar*)buffer );
				if ( bytesRead == 0 ) break;	// Make sure forward progress happens.
				utf8->append ( &buffer[0], bytesWritten );
				hostPtr += bytesRead;
				hostLen -= bytesRead;
			}
		
			DisposeTextToUnicodeInfo ( &converterInfo );

		} catch ( ... ) {

			DisposeTextToUnicodeInfo ( &converterInfo );
			throw;

		}
	
	}	// MacEncodingToUTF8

#elif XMP_UNIXBuild

	#error "HostEncodingToUTF8 is not implemented for UNIX"

#endif

// =================================================================================================
// ReconcileUtils::LocalToUTF8
// ===========================

void ReconcileUtils::LocalToUTF8 ( const void * _localPtr, size_t localLen, std::string * utf8 )
{
	const XMP_Uns8* localPtr = (XMP_Uns8*)_localPtr;

	utf8->erase();
	
	if ( IsASCII ( localPtr, localLen ) ) {
		utf8->assign ( (const char *)localPtr, localLen );
		return;
	} 
	
	#if XMP_WinBuild

		WinEncodingToUTF8 ( CP_ACP, localPtr, localLen, utf8 );
		
	#elif XMP_MacBuild
	
		OSStatus err;
		
		TextEncoding localEncoding;
		err = UpgradeScriptInfoToTextEncoding ( smSystemScript, kTextLanguageDontCare, kTextRegionDontCare, 0, &localEncoding );
		if ( err != noErr ) XMP_Throw ( "UpgradeScriptInfoToTextEncoding failed", kXMPErr_ExternalFailure );
		
		MacEncodingToUTF8 ( localEncoding, localPtr, localLen, utf8 );

	#elif XMP_UNIXBuild
	
		#error "LocalToUTF8 is not implemented for UNIX"
	
	#endif

}	// ReconcileUtils::LocalToUTF8

// =================================================================================================
// ReconcileUtils::Latin1ToUTF8
// ============================
//
// Actually from the Windows code page 1252 superset of 8859-1.

void ReconcileUtils::Latin1ToUTF8 ( const void * _latin1Ptr, size_t latin1Len, std::string * utf8 )
{
	const XMP_Uns8* latin1Ptr = (XMP_Uns8*)_latin1Ptr;

	utf8->erase();
	
	if ( IsASCII ( latin1Ptr, latin1Len ) ) {
		utf8->assign ( (const char *)latin1Ptr, latin1Len );
		return;
	} 
	
	#if XMP_WinBuild

		WinEncodingToUTF8 ( 1252, latin1Ptr, latin1Len, utf8 );
		
	#elif XMP_MacBuild
	
		TextEncoding latin1Encoding;
		latin1Encoding = CreateTextEncoding ( kTextEncodingWindowsLatin1,
											  kTextEncodingDefaultVariant, kTextEncodingDefaultFormat );
		
		MacEncodingToUTF8 ( latin1Encoding, latin1Ptr, latin1Len, utf8 );

	#elif XMP_UNIXBuild
	
		#error "Latin1ToUTF8 is not implemented for UNIX"
	
	#endif

}	// ReconcileUtils::Latin1ToUTF8
