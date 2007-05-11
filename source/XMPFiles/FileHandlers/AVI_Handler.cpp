// =================================================================================================
// ADOBE SYSTEMS INCORPORATED
// Copyright 2002-2007 Adobe Systems Incorporated
// All Rights Reserved
//
// NOTICE: Adobe permits you to use, modify, and distribute this file in accordance with the terms
// of the Adobe license agreement accompanying it.
// =================================================================================================

#include "AVI_Handler.hpp"
#include "RIFF_Support.hpp"

#if XMP_WinBuild
	#include <vfw.h>
#else
	#ifndef formtypeAVI
		#define	formtypeAVI	MakeFourCC('A', 'V', 'I', ' ')
	#else
		#error "formtypeAVI already defined"
	#endif
#endif


using namespace std;

#define kXMPUserDataType MakeFourCC ( '_', 'P', 'M', 'X' )	/* Yes, backwards! */


/*******************************************************
** Premiere Pro specific info for reconciliation
*******************************************************/
// FourCC codes for the RIFF chunks
#define	aviTimeChunk	MakeFourCC('I','S','M','T')
#define	avihdrlChunk	MakeFourCC('h','d','r','l')
#define	myOrgTimeChunk	MakeFourCC('t','c','_','O')		/* 0x4f5f6374 */
#define	myAltTimeChunk	MakeFourCC('t','c','_','A')
#define	myOrgReelChunk	MakeFourCC('r','n','_','O')		/* 0x4f5f6e72 */
#define	myAltReelChunk	MakeFourCC('r','n','_','A')
#define	myCommentChunk	MakeFourCC('c','m','n','t')
#define	myTimeList		MakeFourCC('T','d','a','t')		/* 0x74616454 */
#define	myCommentList	MakeFourCC('C','d','a','t')

#define	TIMELEN			18
#define	REELLEN			40
#define REALTIMELEN		11
#define COMMENTLEN		256

/* list id (4 bytes) + four tags hdrs (8 each) + 2 TIMEs + 2 REELs */
#define	PR_AVI_TIMELEN (12 + 2 * (8 + TIMELEN) + 2 * (8 + REELLEN))

#define PR_AVI_COMMENTLEN (12 + 8 + COMMENTLEN)

#define kStartTimecode "startTimecode"
#define kTimeValue "timeValue"
#define kAltTimecode "altTimecode"
#define kTapeName "tapeName"
#define kAltTapeName "altTapeName"
#define kLogComment "logComment"

/*******************************************************
*******************************************************/


// =================================================================================================
/// \file AVI_Handler.cpp
/// \brief File format handler for AVI.
///
/// This header ...
///
// =================================================================================================

// =================================================================================================
// AVI_MetaHandlerCTor
// ====================

XMPFileHandler * AVI_MetaHandlerCTor ( XMPFiles * parent )
{
	return new AVI_MetaHandler ( parent );

}	// AVI_MetaHandlerCTor

// =================================================================================================
// AVI_CheckFormat
// ===============
//
// An AVI file must begin with "RIFF", a 4 byte little endian length, then "AVI ". The length should
// be fileSize-8, but we don't bother checking this here.

bool AVI_CheckFormat ( XMP_FileFormat format,
					   XMP_StringPtr  filePath,
			           LFA_FileRef    fileRef,
			           XMPFiles *     parent )
{
	IgnoreParam(format); IgnoreParam(parent);
	XMP_Assert ( format == kXMP_AVIFile );

	if ( fileRef == 0 ) return false;
	
	enum { kBufferSize = 12 };
	XMP_Uns8 buffer [kBufferSize];
	
	LFA_Seek ( fileRef, 0, SEEK_SET );
	LFA_Read ( fileRef, buffer, kBufferSize );
	
	// "RIFF" is 52 49 46 46, "AVI " is 41 56 49 20
	if ( (! CheckBytes ( &buffer[0], "\x52\x49\x46\x46", 4 )) ||
		 (! CheckBytes ( &buffer[8], "\x41\x56\x49\x20", 4 )) ) return false;

	return true;
	
}	// AVI_CheckFormat

// =================================================================================================
// AVI_MetaHandler::AVI_MetaHandler
// ================================

AVI_MetaHandler::AVI_MetaHandler ( XMPFiles * _parent )
{
	this->parent = _parent;
	this->handlerFlags = kAVI_HandlerFlags;
	this->stdCharForm  = kXMP_Char8Bit;

}	// AVI_MetaHandler::AVI_MetaHandler

// =================================================================================================
// AVI_MetaHandler::~AVI_MetaHandler
// =================================

AVI_MetaHandler::~AVI_MetaHandler()
{
	// Nothing to do.
	
}	// AVI_MetaHandler::~AVI_MetaHandler

// =================================================================================================
// AVI_MetaHandler::UpdateFile
// ===========================

void AVI_MetaHandler::UpdateFile ( bool doSafeUpdate )
{
	bool ok;
	
	if ( ! this->needsUpdate ) return;
	if ( doSafeUpdate ) XMP_Throw ( "AVI_MetaHandler::UpdateFile: Safe update not supported", kXMPErr_Unavailable );
	
	XMP_StringPtr packetStr = xmpPacket.c_str();
	XMP_StringLen packetLen = xmpPacket.size();
	if ( packetLen == 0 ) return;

	// Make sure we're writing an even number of bytes as required by the RIFF specification.
	if ( (xmpPacket.size() & 1) == 1 ) xmpPacket.push_back ( ' ' );
	XMP_Assert ( (xmpPacket.size() & 1) == 0 );
	packetStr = xmpPacket.c_str();	// ! Make sure they are current.
	packetLen = xmpPacket.size();
	
	LFA_FileRef fileRef(this->parent->fileRef);
	if ( fileRef == 0 ) return;

	RIFF_Support::RiffState riffState;
	long numTags = RIFF_Support::OpenRIFF ( fileRef, riffState );
	if ( numTags == 0 ) return;

	ok = RIFF_Support::PutChunk ( fileRef, riffState, formtypeAVI, kXMPUserDataType, (char*)packetStr, packetLen );
	if ( ! ok )return;	// If there's an error writing the chunk, bail.

	// Update legacy metadata

	std::string startTimecodeString, altTimecodeString, orgReelString, altReelString, logCommentString;
	
	this->xmpObj.GetStructField ( kXMP_NS_DM, kStartTimecode, kXMP_NS_DM, kTimeValue, &startTimecodeString, 0 );
	this->xmpObj.GetStructField ( kXMP_NS_DM, kAltTimecode, kXMP_NS_DM, kTimeValue, &altTimecodeString, 0 );
	this->xmpObj.GetProperty ( kXMP_NS_DM, kTapeName, &orgReelString, 0 );
	this->xmpObj.GetProperty ( kXMP_NS_DM, kAltTapeName, &altReelString, 0 );
	this->xmpObj.GetProperty ( kXMP_NS_DM, kLogComment, &logCommentString, 0 );

	if ( startTimecodeString.size() != 0 ) {
		// I'm not sure why we copy into this 12 char buffer, but this is what Premiere code does.
		char aviTime [12];
		memset ( aviTime, 0, 12 );
		memcpy ( aviTime, startTimecodeString.data(), 11 );	// AUDIT: 11 is less than 12
		RewriteChunk ( fileRef, riffState, aviTimeChunk, avihdrlChunk, aviTime );
	}

	if ( logCommentString.size() != 0 ) {

		ok = FindChunk ( riffState, myCommentChunk, myCommentList, 0, 0, 0, 0 );

		if ( ! ok ) {

			// Always rewrite the comment string, even if empty, so the user can erase it.
			RIFF_Support::RewriteChunk ( fileRef, riffState, myCommentChunk, myCommentList, logCommentString.c_str() );
		
		} else {

			ok = MakeChunk ( fileRef, riffState, formtypeAVI, PR_AVI_COMMENTLEN );
			if ( ! ok ) return; // If there's an error making a chunk, bail

			RIFF_Support::ltag listtag;
			listtag.id = MakeUns32LE ( FOURCC_LIST );
			listtag.len = MakeUns32LE ( PR_AVI_COMMENTLEN - 8 );
			listtag.subid = MakeUns32LE ( myCommentList );
			LFA_Write ( fileRef, &listtag, 12 );

			RIFF_Support::WriteChunk ( fileRef, myCommentChunk, logCommentString.c_str(), COMMENTLEN );

		}

	}

	ok = RIFF_Support::FindChunk ( riffState, myOrgTimeChunk, myTimeList, 0, 0, 0, 0 );
	
	if ( ok ) {

		if ( startTimecodeString.size() != 0 ) {
			RewriteChunk ( fileRef, riffState, myOrgTimeChunk, myTimeList, startTimecodeString.c_str() );
		}

		if ( altTimecodeString.size() != 0 ) {
			RewriteChunk ( fileRef, riffState, myAltTimeChunk, myTimeList, altTimecodeString.c_str() );
		}

		// Always rewrite the reel strings, even if empty, so the user can erase them.
		RewriteChunk ( fileRef, riffState, myOrgReelChunk, myTimeList, orgReelString.c_str() );
		RewriteChunk ( fileRef, riffState, myAltReelChunk, myTimeList, altReelString.c_str() );

	} else {
	
		ok = MakeChunk ( fileRef, riffState, formtypeAVI, PR_AVI_TIMELEN );
		if ( ! ok ) return; // If there's an error making a chunk, bail

		RIFF_Support::ltag listtag;
		listtag.id = MakeUns32LE ( FOURCC_LIST );
		listtag.len = MakeUns32LE ( PR_AVI_TIMELEN - 8 );
		listtag.subid = MakeUns32LE ( myTimeList );
		LFA_Write(fileRef, &listtag, 12);

		RIFF_Support::WriteChunk ( fileRef, myOrgTimeChunk, startTimecodeString.c_str(), TIMELEN );
		RIFF_Support::WriteChunk ( fileRef, myAltTimeChunk, altTimecodeString.c_str(), TIMELEN );
		RIFF_Support::WriteChunk ( fileRef, myOrgReelChunk, orgReelString.c_str(), REELLEN );
		RIFF_Support::WriteChunk ( fileRef, myAltReelChunk, altReelString.c_str(), REELLEN );

	}

	this->needsUpdate = false;

}	// AVI_MetaHandler::UpdateFile

// =================================================================================================
// AVI_MetaHandler::WriteFile
// ==========================

void AVI_MetaHandler::WriteFile ( LFA_FileRef         sourceRef,
								  const std::string & sourcePath )
{
	IgnoreParam(sourceRef); IgnoreParam(sourcePath);
	
	XMP_Throw ( "AVI_MetaHandler::WriteFile: Not supported", kXMPErr_Unavailable );

}	// AVI_MetaHandler::WriteFile

// =================================================================================================
// AVI_MetaHandler::CacheFileData
// ==============================

void AVI_MetaHandler::CacheFileData()
{
	bool ok;
	
	this->containsXMP = false;
	
	LFA_FileRef fileRef ( this->parent->fileRef );
	if ( fileRef == 0 ) return;

	RIFF_Support::RiffState riffState;
	long numTags = RIFF_Support::OpenRIFF ( fileRef, riffState );
	if ( numTags == 0 ) return;

	// Determine the size of the metadata
	unsigned long bufferSize(0);
	ok = RIFF_Support::GetRIFFChunk ( fileRef, riffState, kXMPUserDataType, 0, 0, 0, &bufferSize );

	if ( ! ok ) {

		packetInfo.writeable = true;	// If no packet found, created packets will be writeable.

	} else if ( bufferSize > 0 ) {

		// Size and clear the buffer
		this->xmpPacket.reserve ( bufferSize );
		this->xmpPacket.assign ( bufferSize, ' ' );

		// Get the metadata
		ok = RIFF_Support::GetRIFFChunk ( fileRef, riffState, kXMPUserDataType, 0, 0, (char*)this->xmpPacket.c_str(), &bufferSize );
		if ( ok ) {
			this->packetInfo.offset = kXMPFiles_UnknownOffset;
			this->packetInfo.length = bufferSize;
			this->xmpObj.ParseFromBuffer ( this->xmpPacket.c_str(), this->xmpPacket.size() );
			this->containsXMP = true;
		}

	}


	// Reconcile legacy metadata.
	
	std::string aviTimeString, orgTimeString, altTimeString;
	unsigned long aviTimeSize, orgTimeSize, altTimeSize;
	
	ok = RIFF_Support::GetRIFFChunk ( fileRef, riffState, aviTimeChunk, avihdrlChunk, 0, 0, &aviTimeSize );
	if ( ok ) {
		aviTimeString.reserve ( aviTimeSize );
		aviTimeString.assign ( aviTimeSize, ' ' );
		RIFF_Support::GetRIFFChunk ( fileRef, riffState, aviTimeChunk, avihdrlChunk, 0, (char*)aviTimeString.c_str(), &aviTimeSize );
	}

	ok = RIFF_Support::GetRIFFChunk ( fileRef, riffState, myOrgTimeChunk, myTimeList, 0, 0, &orgTimeSize );
	if ( ok ) {
		orgTimeString.reserve ( orgTimeSize );
		orgTimeString.assign ( orgTimeSize, ' ' );
		RIFF_Support::GetRIFFChunk ( fileRef, riffState, myOrgTimeChunk, myTimeList, 0, (char*)orgTimeString.c_str(), &orgTimeSize );
	}

	ok = RIFF_Support::GetRIFFChunk ( fileRef, riffState, myAltTimeChunk, myTimeList, 0, 0, &altTimeSize );
	if ( ok ) {
		altTimeString.reserve ( altTimeSize );
		altTimeString.assign ( altTimeSize, ' ' );
		RIFF_Support::GetRIFFChunk ( fileRef, riffState, myAltTimeChunk, myTimeList, 0, (char*)altTimeString.c_str(), &altTimeSize );
	}

	if ( (! aviTimeString.empty()) && orgTimeString.empty() && (altTimeString.empty()) ) {

		// If we have an avi time, and not the org or alt, use the avi. I suspect this is for some earlier backwards compatibility.

		std::string xmpString;
		this->xmpObj.GetStructField ( kXMP_NS_DM, kStartTimecode, kXMP_NS_DM, kTimeValue, &xmpString, 0 );
		if ( xmpString.compare ( 0, REALTIMELEN, aviTimeString, 0, REALTIMELEN ) ) {
			this->xmpObj.SetStructField ( kXMP_NS_DM, kStartTimecode, kXMP_NS_DM, kTimeValue, aviTimeString, 0 );
			this->containsXMP = true;
		}

	} else {

		// Otherwise, check the original and alt timecodes.

		if ( ! orgTimeString.empty() ) {
			std::string xmpString;
			this->xmpObj.GetStructField ( kXMP_NS_DM, kStartTimecode, kXMP_NS_DM, kTimeValue, &xmpString, 0 );
			if (xmpString.compare ( 0, REALTIMELEN, orgTimeString, 0, REALTIMELEN ) ) {
				this->xmpObj.SetStructField ( kXMP_NS_DM, kStartTimecode, kXMP_NS_DM, kTimeValue, orgTimeString, 0 );
				this->containsXMP = true;
			}
		}

		if ( ! altTimeString.empty() ) {
			std::string xmpString;
			this->xmpObj.GetStructField ( kXMP_NS_DM, kAltTimecode, kXMP_NS_DM, kTimeValue, &xmpString, 0 );
			if ( xmpString.compare ( 0, REALTIMELEN, altTimeString, 0, REALTIMELEN ) ) {
				this->xmpObj.SetStructField ( kXMP_NS_DM, kAltTimecode, kXMP_NS_DM, kTimeValue, altTimeString, 0 );
				this->containsXMP = true;
			}
		}

	}

	unsigned long orgReelSize;
	ok = RIFF_Support::GetRIFFChunk ( fileRef, riffState, myOrgReelChunk, myTimeList, 0, 0, &orgReelSize );
	if ( ok ) {

		std::string orgReelString;
		orgReelString.reserve ( orgReelSize );
		orgReelString.assign ( orgReelSize, ' ' );
		RIFF_Support::GetRIFFChunk ( fileRef, riffState, myOrgReelChunk, myTimeList, 0, (char*)orgReelString.c_str(), &orgReelSize );

		if ( ! orgReelString.empty() ) {
			std::string xmpString;
			this->xmpObj.GetProperty ( kXMP_NS_DM, kTapeName, &xmpString, 0 );
			if ( xmpString.compare ( 0, REELLEN, orgReelString, 0, REELLEN ) ) {
				this->xmpObj.SetProperty ( kXMP_NS_DM, kTapeName, orgReelString, 0 );
				this->containsXMP = true;
			}
		}

	}

	unsigned long altReelSize;
	ok = RIFF_Support::GetRIFFChunk ( fileRef, riffState, myAltReelChunk, myTimeList, 0, 0, &altReelSize );
	if ( ok ) {

		std::string altReelString;
		altReelString.reserve ( altReelSize );
		altReelString.assign ( altReelSize, ' ' );
		RIFF_Support::GetRIFFChunk ( fileRef, riffState, myAltReelChunk, myTimeList, 0, (char*)altReelString.c_str(), &altReelSize );

		if ( ! altReelString.empty() ) {
			std::string xmpString;
			this->xmpObj.GetProperty ( kXMP_NS_DM, kAltTapeName, &xmpString, 0 );
			if ( xmpString.compare ( 0, REELLEN, altReelString, 0, REELLEN ) ) {
				this->xmpObj.SetProperty ( kXMP_NS_DM, kAltTapeName, altReelString, 0 );
				this->containsXMP = true;
			}
		}

	}

	unsigned long logCommentSize;
	ok = RIFF_Support::GetRIFFChunk ( fileRef, riffState, myCommentChunk, myCommentList, 0, 0, &logCommentSize );
	if ( ok ) {

		std::string logCommentString;
		logCommentString.reserve ( logCommentSize );
		logCommentString.assign ( logCommentSize, ' ' );
		RIFF_Support::GetRIFFChunk ( fileRef, riffState, myCommentChunk, myCommentList, 0, (char*)logCommentString.c_str(), &logCommentSize );

		if ( ! logCommentString.empty() ) {
			std::string xmpString;
			this->xmpObj.GetProperty ( kXMP_NS_DM, kLogComment, &xmpString, 0 );
			if ( xmpString.compare ( logCommentString ) ) {
				this->xmpObj.SetProperty ( kXMP_NS_DM, kLogComment, logCommentString, 0 );
				this->containsXMP = true;
			}
		}

	}

	// Update the xmpPacket, as the xmpObj might have been updated with legacy info.
	this->xmpObj.SerializeToBuffer ( &this->xmpPacket, kXMP_UseCompactFormat );
	this->packetInfo.offset = kXMPFiles_UnknownOffset;
	this->packetInfo.length = this->xmpPacket.size();

	this->processedXMP = this->containsXMP;
	
}	// AVI_MetaHandler::CacheFileData
