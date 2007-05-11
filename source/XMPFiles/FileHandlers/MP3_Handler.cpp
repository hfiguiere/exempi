// =================================================================================================
// ADOBE SYSTEMS INCORPORATED
// Copyright 2002-2007 Adobe Systems Incorporated
// All Rights Reserved
//
// NOTICE: Adobe permits you to use, modify, and distribute this file in accordance with the terms
// of the Adobe license agreement accompanying it.
// =================================================================================================

#include "MP3_Handler.hpp"
#include "ID3_Support.hpp"

using namespace std;

#define	mp3TitleChunk		"TIT2"
#define	mp3CreateDateChunk3	"TYER"
#define mp3CreateDateChunk4 "TDRV"
#define	mp3ArtistChunk		"TPE1"
#define	mp3AlbumChunk		"TALB"
#define	mp3GenreChunk		"TCON"
#define	mp3CommentChunk		"COMM"
#define mp3TrackChunk		"TRCK"

// DC
#define kTitle		"title"

// XMP
#define kCreateDate	"CreateDate"

// DM
#define kArtist		"artist"
#define kAlbum		"album"
#define kGenre		"genre"
#define kLogComment "logComment"
#define kTrack		"trackNumber"

// =================================================================================================
/// \file MP3_Handler.cpp
/// \brief File format handler for MP3.
///
/// This header ...
///
// =================================================================================================

// =================================================================================================
// MP3_MetaHandlerCTor
// ====================

XMPFileHandler * MP3_MetaHandlerCTor ( XMPFiles * parent )
{
	return new MP3_MetaHandler ( parent );

}	// MP3_MetaHandlerCTor

// =================================================================================================
// MP3_CheckFormat
// ===============

// For MP3 we check parts .... See the MP3 spec for offset info.
//
//

bool MP3_CheckFormat ( XMP_FileFormat format,
					   XMP_StringPtr  filePath,
			           LFA_FileRef    inFileRef,
			           XMPFiles *     parent )
{
	IgnoreParam(format); IgnoreParam(filePath);
	XMP_Assert ( format == kXMP_MP3File );

	if ( inFileRef == 0 ) return false;

	// BUG FIX 1219125: If we find that the "unsynchronistation" flag is turned on, we should fail on that file.
	// TODO: Support "unsynchronized" files.

	LFA_Seek ( inFileRef, 0ULL, SEEK_SET );

	char szID [4] = { "xxx" };
	long bytesRead = LFA_Read ( inFileRef, szID, 3 );
	if ( bytesRead != 3 ) return false;

	if ( strncmp ( szID, "ID3", 3 ) != 0 ) {

		return (parent->format == kXMP_MP3File);	// No ID3 signature, depend on first call hint.
	
	} else {

		// Read the version, flag and size.
		XMP_Uns8 v2 = 0, flags = 0, bMajorVer = 0;
		unsigned long dwLen = 0;
		bool ok = ID3_Support::GetTagInfo ( inFileRef, bMajorVer, v2, flags, dwLen );

		if ( ok ) {
			if ( (bMajorVer < 3)  || (bMajorVer > 4) ) return false;
			if ( flags & 0x80 ) return false;	// 0x80 == "unsynchronized"
		}

	}

	return true;
	
}	// MP3_CheckFormat


// =================================================================================================
// MP3_MetaHandler::MP3_MetaHandler
// ================================

MP3_MetaHandler::MP3_MetaHandler ( XMPFiles * _parent )
{
	this->parent = _parent;
	this->handlerFlags = kMP3_HandlerFlags;
	this->stdCharForm  = kXMP_Char8Bit;

}	// MP3_MetaHandler::MP3_MetaHandler

// =================================================================================================
// MP3_MetaHandler::~MP3_MetaHandler
// =================================

MP3_MetaHandler::~MP3_MetaHandler()
{
	// Nothing to do.
	
}	// MP3_MetaHandler::~MP3_MetaHandler

// =================================================================================================
// MP3_MetaHandler::UpdateFile
// ===========================

void MP3_MetaHandler::UpdateFile ( bool doSafeUpdate )
{
	if ( ! this->needsUpdate ) return;
	if ( doSafeUpdate ) XMP_Throw ( "MP3_MetaHandler::UpdateFile: Safe update not supported", kXMPErr_Unavailable );

	bool fReconciliate = !(this->parent->openFlags & kXMPFiles_OpenOnlyXMP);
	
	XMP_StringPtr packetStr = xmpPacket.c_str();
	XMP_StringLen packetLen = xmpPacket.size();
	if ( packetLen == 0 ) return;
	
	LFA_FileRef fileRef ( this->parent->fileRef );
	if ( fileRef == 0 ) return;

	// Get the id3v2 version
	XMP_Uns8 bVersion = 3;
	unsigned long dwTagSize;
	bool ok = ID3_Support::FindID3Tag ( fileRef, dwTagSize, bVersion );
	if ( ! ok ) bVersion = 3;
	
	// Allocate the temp buffer for the native frames we have to overwrite
	unsigned long bufferSize = 7*TAG_MAX_SIZE; // Just enough buffer for all 7 tags
	char buffer[7*TAG_MAX_SIZE];
	memset ( buffer, 0, bufferSize );
	unsigned long dwCurOffset = 0;
    
	if ( fReconciliate ) {

		std::string strTitle;
		this->xmpObj.GetLocalizedText ( kXMP_NS_DC, kTitle, "", "x-default", 0, &strTitle, 0 );
		ID3_Support::AddXMPTagToID3Buffer ( buffer, &dwCurOffset, bufferSize, bVersion,
											mp3TitleChunk, strTitle.c_str(), strTitle.size() );

		std::string strDate;
		this->xmpObj.GetProperty ( kXMP_NS_XMP, kCreateDate, &strDate, 0 );
		if ( bVersion == 4 ) {
			ID3_Support::AddXMPTagToID3Buffer ( buffer, &dwCurOffset, bufferSize, bVersion,
												mp3CreateDateChunk4, strDate.c_str(), strDate.size() );
		} else {
			ID3_Support::AddXMPTagToID3Buffer ( buffer, &dwCurOffset, bufferSize, bVersion,
												mp3CreateDateChunk3, strDate.c_str(), strDate.size() );			
		}
		
		std::string strArtist;
		this->xmpObj.GetProperty ( kXMP_NS_DM, kArtist, &strArtist, 0 );
		ID3_Support::AddXMPTagToID3Buffer ( buffer, &dwCurOffset, bufferSize, bVersion, mp3ArtistChunk,
											strArtist.c_str(), strArtist.size() );

		std::string strAlbum;
		this->xmpObj.GetProperty ( kXMP_NS_DM, kAlbum, &strAlbum, 0 );
		ID3_Support::AddXMPTagToID3Buffer ( buffer, &dwCurOffset, bufferSize, bVersion,
											mp3AlbumChunk, strAlbum.c_str(), strAlbum.size() );

		std::string strGenre;
		this->xmpObj.GetProperty ( kXMP_NS_DM, kGenre, &strGenre, 0 );
		ID3_Support::AddXMPTagToID3Buffer ( buffer, &dwCurOffset, bufferSize, bVersion,
											mp3GenreChunk, strGenre.c_str(), strGenre.size() );

		std::string strComment;
		this->xmpObj.GetProperty ( kXMP_NS_DM, kLogComment, &strComment, 0 );
		ID3_Support::AddXMPTagToID3Buffer ( buffer, &dwCurOffset, bufferSize, bVersion,
											mp3CommentChunk, strComment.c_str(), strComment.size() );

		std::string strTrack;
		this->xmpObj.GetProperty ( kXMP_NS_DM, kTrack, &strTrack, 0 );
		ID3_Support::AddXMPTagToID3Buffer ( buffer, &dwCurOffset, bufferSize, bVersion,
											mp3TrackChunk, strTrack.c_str(), strTrack.size() );

	}

	// TODO id3v1 tags

	// Saving it all
	ID3_Support::SetMetaData ( fileRef, (char*)packetStr, packetLen, buffer, dwCurOffset, fReconciliate );

	this->needsUpdate = false;

}	// MP3_MetaHandler::UpdateFile

// =================================================================================================
// MP3_MetaHandler::WriteFile
// ==========================

void MP3_MetaHandler::WriteFile ( LFA_FileRef         sourceRef,
								  const std::string & sourcePath )
{
	IgnoreParam(sourceRef); IgnoreParam(sourcePath);
	
	XMP_Throw ( "MP3_MetaHandler::WriteFile: Not supported", kXMPErr_Unavailable );

}	// MP3_MetaHandler::WriteFile

// =================================================================================================
// MP3_MetaHandler::CacheFileData
// ==============================

void MP3_MetaHandler::CacheFileData()
{
	bool fReconciliate = ! ( this->parent->openFlags & kXMPFiles_OpenOnlyXMP );
	bool ok;

	this->containsXMP = false;

	// We asked the host to allow us to manage the opening of the file
	LFA_FileRef fileRef ( this->parent->fileRef );
	if ( fileRef == 0 ) return;

	// Determine the size of the metadata
	unsigned long bufferSize(0);
	ok = ID3_Support::GetMetaData ( fileRef, 0, &bufferSize, 0 );

	if ( ! ok ) {

		packetInfo.writeable = true;	// If no packet found, created packets will be writeable

	} else if ( bufferSize > 0 ) {

		// Allocate the buffer
		std::string buffer;
		buffer.reserve ( bufferSize );
		buffer.assign ( bufferSize, ' ' );

		// Get the metadata
		XMP_Int64 xmpOffset;
		ok = ID3_Support::GetMetaData ( fileRef, (char*)buffer.c_str(), &bufferSize, &xmpOffset );
		if ( ok ) {
			this->packetInfo.offset = xmpOffset;
			this->packetInfo.length = bufferSize;
			this->xmpPacket.assign(buffer.data(), bufferSize);
			this->xmpObj.ParseFromBuffer ( this->xmpPacket.c_str(), this->xmpPacket.size() );
			this->containsXMP = true;
		}

	}

	if ( fReconciliate ) {

		// ! Note that LoadPropertyFromID3 sets this->containsXMP, update this->processedXMP after!
		LoadPropertyFromID3 ( fileRef, mp3TitleChunk, kXMP_NS_DC, kTitle, true );
		LoadPropertyFromID3 ( fileRef, mp3CreateDateChunk3, kXMP_NS_XMP, kCreateDate );
		LoadPropertyFromID3 ( fileRef, mp3ArtistChunk, kXMP_NS_DM, kArtist );
		LoadPropertyFromID3 ( fileRef, mp3AlbumChunk, kXMP_NS_DM, kAlbum );
		LoadPropertyFromID3 ( fileRef, mp3GenreChunk, kXMP_NS_DM, kGenre );
		LoadPropertyFromID3 ( fileRef, mp3CommentChunk, kXMP_NS_DM, kLogComment );
		LoadPropertyFromID3 ( fileRef, mp3TrackChunk, kXMP_NS_DM, kTrack );

	}

	this->processedXMP = this->containsXMP;
	
}	// MP3_MetaHandler::CacheFileData

// =================================================================================================

bool MP3_MetaHandler::LoadPropertyFromID3 ( LFA_FileRef inFileRef, char * strFrame, char * strNameSpace, char * strXMPTag, bool fLocalText )
{

	// Allocate the temp buffer for the native frames we have to overwrite
	unsigned long bufferSize = TAG_MAX_SIZE;
	std::string buffer;
	buffer.reserve ( bufferSize );
	buffer.assign ( bufferSize, '\0' );

	// Get the old XMP tag
	std::string xmpString("");
	if ( fLocalText ) {
		this->xmpObj.GetLocalizedText ( strNameSpace, strXMPTag, "", "x-default", 0, &xmpString, 0 );
	} else {
		this->xmpObj.GetProperty ( strNameSpace, strXMPTag, &xmpString, 0 );
	}

	// Get the frame
	bool ok = ID3_Support::GetFrameData ( inFileRef, strFrame, (char*)buffer.c_str(), bufferSize );
	if ( ok ) {
		if ( ! buffer.empty() ) {

			if ( xmpString.compare ( buffer ) ) {
				if ( fLocalText ) {
					this->xmpObj.SetLocalizedText ( strNameSpace, strXMPTag, 0, "x-default", buffer );
				} else {
					this->xmpObj.SetProperty ( strNameSpace, strXMPTag, buffer, 0 );
				}
			}

			this->containsXMP = true;
			return true;

		}
	}

	// Couldn't find the frame, let's clean it on the XMP side if that tag exists.
	if ( xmpString.size() != 0 ) {

		buffer = "";

		if ( fLocalText ) {
			this->xmpObj.SetLocalizedText ( strNameSpace, strXMPTag, 0, "x-default", buffer );
		} else {
			this->xmpObj.SetProperty ( strNameSpace, strXMPTag, buffer, 0 );
		}

		this->containsXMP = true;
		return true;

	}

	return false;

} // WAV_MetaHandler::LoadPropertyFromID3
