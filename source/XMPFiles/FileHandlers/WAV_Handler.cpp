// =================================================================================================
// ADOBE SYSTEMS INCORPORATED
// Copyright 2002-2007 Adobe Systems Incorporated
// All Rights Reserved
//
// NOTICE: Adobe permits you to use, modify, and distribute this file in accordance with the terms
// of the Adobe license agreement accompanying it.
// =================================================================================================

#if WIN_ENV
	#pragma warning ( disable : 4996 )	// '...' was declared deprecated
#endif

#include "WAV_Handler.hpp"
#include "RIFF_Support.hpp"
#include "Reconcile_Impl.hpp"
#include "XMP_Const.h"

using namespace std;

#define kXMPUserDataType MakeFourCC ( '_', 'P', 'M', 'X' )	/* Yes, backwards! */

#define	formtypeWAVE	MakeFourCC('W', 'A', 'V', 'E')


// -------------------------------------------------------------------------------------------------
// Premiere Pro specific info for reconciliation

// ! MakeFourCC warning: The MakeFourCC macro creates a 32 bit int with the letters "reversed". This
// ! is a leftover of the original Win-only code. It happens to work OK on little endian machines,
// ! when stored in memory the letters are in the expected order. To be safe, always use the XMP
// ! endian control macros when storing to memory or loading from memory.

// FourCC codes for the RIFF chunks
#define	wavWaveTitleChunk		MakeFourCC('D','I','S','P')
#define	wavInfoCreateDateChunk	MakeFourCC('I','C','R','D')
#define	wavInfoArtistChunk		MakeFourCC('I','A','R','T')
#define	wavInfoAlbumChunk		MakeFourCC('I','N','A','M')
#define	wavInfoGenreChunk		MakeFourCC('I','G','N','R')
#define	wavInfoCommentChunk		MakeFourCC('I','C','M','T')
#define wavInfoEngineerChunk	MakeFourCC('I','E','N','G')
#define wavInfoCopyrightChunk	MakeFourCC('I','C','O','P')
#define wavInfoSoftwareChunk	MakeFourCC('I','S','F','T')

#define	wavInfoTag		MakeFourCC('I','N','F','O')
#define	wavWaveTag		MakeFourCC('W','A','V','E')

// DC
#define kTitle		"title"
#define kCopyright	"rights"

// XMP
#define kCreateDate	"CreateDate"

// DM
#define kArtist		"artist"
#define kAlbum		"album"
#define kGenre		"genre"
#define kLogComment "logComment"
#define kEngineer	"engineer"
#define kSoftware	"CreatorTool"

// -------------------------------------------------------------------------------------------------
// Legacy digest info
// ------------------
//
// The original WAV handler code didn't keep a legacy digest, it imported the legacy on every open.
// Because local encoding is used for the legacy, this can cause loss in the XMP. (The use of local
// encoding itself is an issue, the AVI handler is using UTF-8.)
//
// The legacy digest for WAV is a list of chunk IDs and a 128-bit MD5 digest, formatted like:
//   DISP,IART,ICMT,ICOP,ICRD,IENG,IGNR,INAM,ISFT;012345678ABCDEF012345678ABCDEF
//
// The list of IDs are the recognized legacy chunks, in alphabetical order. This the full list that
// could be recognized, not restricted to those actually present in the specific file. When opening
// a file the new software's list is used to create the comparison digest, not the list from the
// file. So that changes to the recognized list will trigger an import.
//
// The MD5 digest is computed from the full legacy chunk, including the ID and length portions, not
// including any pad byte for odd data length. The length must be in file (little endian) order,
// not native machine order. The legacy chunks are added to the digest in list (alphabetical by ID)
// order.
//
// Legacy can be imported in 3 circumstances:
//
// 1. If the file does not yet have XMP, all of the legacy is imported.
//
// 2. If the file has XMP and a digest, and the recomputed digest differs from the saved digest, the
//    legacy is imported. The digest comparison is the full string, including the list of IDs. A
//    check is made for each legacy item:
//    2a. If the legacy item is missing or has an empty value, the corresponding XMP is deleted.
//    2b. If the corresponding XMP is missing, the legacy value is imported.
//    2c. If the new legacy value differs from a local encoding of the XMP value, the legacy value
//        is imported.
//
// 3. If the file has XMP but no digest, legacy is imported for items that have no corresponding XMP.
//    Any existing XMP is left alone. This is protection for tools that might be XMP aware but do
//    not provide a digest or any legacy reconciliation.

#define TAG_MAX_SIZE 5024

// =================================================================================================

static inline int GetStringRiffSize ( const std::string & str )
{
	int l = strlen ( const_cast<char *> (str.data()) );
	if ( l & 1 ) ++l;
	return l;
}

// =================================================================================================
/// \file WAV_Handler.cpp
/// \brief File format handler for WAV.
///
/// This header ...
///
// =================================================================================================

// =================================================================================================
// WAV_MetaHandlerCTor
// ===================

XMPFileHandler * WAV_MetaHandlerCTor ( XMPFiles * parent )
{
	return new WAV_MetaHandler ( parent );

}	// WAV_MetaHandlerCTor

// =================================================================================================
// WAV_CheckFormat
// ===============
//
// A WAVE file must begin with "RIFF", a 4 byte little endian length, then "WAVE". The length should
// be fileSize-8, but we don't bother checking this here.

bool WAV_CheckFormat ( XMP_FileFormat format,
					   XMP_StringPtr  filePath,
			           LFA_FileRef    fileRef,
			           XMPFiles *     parent )
{
	IgnoreParam(format); IgnoreParam(parent);
	XMP_Assert ( format == kXMP_WAVFile );

	if ( fileRef == 0 ) return false;
	
	enum { kBufferSize = 12 };
	XMP_Uns8 buffer [kBufferSize];
	
	LFA_Seek ( fileRef, 0, SEEK_SET );
	LFA_Read ( fileRef, buffer, kBufferSize );
	
	// "RIFF" is 52 49 46 46, "WAVE" is 57 41 56 45
	if ( (! CheckBytes ( &buffer[0], "\x52\x49\x46\x46", 4 )) ||
		 (! CheckBytes ( &buffer[8], "\x57\x41\x56\x45", 4 )) ) return false;

	return true;
	
}	// WAV_CheckFormat

// =================================================================================================
// WAV_MetaHandler::WAV_MetaHandler
// ================================

WAV_MetaHandler::WAV_MetaHandler ( XMPFiles * _parent )
{
	this->parent = _parent;
	this->handlerFlags = kWAV_HandlerFlags;
	this->stdCharForm  = kXMP_Char8Bit;
	
}	// WAV_MetaHandler::WAV_MetaHandler

// =================================================================================================
// WAV_MetaHandler::~WAV_MetaHandler
// =================================

WAV_MetaHandler::~WAV_MetaHandler()
{
	// Nothing to do.
}	// WAV_MetaHandler::~WAV_MetaHandler



// =================================================================================================
// WAV_MetaHandler::UpdateFile
// ===========================

void WAV_MetaHandler::UpdateFile ( bool doSafeUpdate )
{
	if ( ! this->needsUpdate ) return;
	if ( doSafeUpdate ) XMP_Throw ( "WAV_MetaHandler::UpdateFile: Safe update not supported", kXMPErr_Unavailable );

	bool fReconciliate = ! (this->parent->openFlags & kXMPFiles_OpenOnlyXMP);
	bool ok;

	std::string strTitle, strArtist, strComment, strCopyright, strCreateDate,
				strEngineer, strGenre, strAlbum, strSoftware;
	
	if ( fReconciliate ) {
	
		// Get the legacy item values, create the new digest, and add the digest to the XMP. The
		// legacy chunks in alphabetical ID order are:
		//   DISP - title
		//   IART - artist
		//   ICMT - log comment
		//   ICOP - copyright
		//   ICRD - create date
		//   IENG - engineer
		//   IGNR - genre
		//   INAM - album
		//   ISFT - software

		MD5_CTX md5Ctx;
		std::string digestStr;
		XMP_Uns8 md5Val[16];
		static char* hexDigits = "0123456789ABCDEF";
		
		MD5Init ( &md5Ctx );

		// Prepare the legacy values and compute the new digest.
		
		PrepareLegacyExport ( kXMP_NS_DC, kTitle, wavWaveTitleChunk, &strTitle, &digestStr, &md5Ctx, true /* LangAlt */ );
		PrepareLegacyExport ( kXMP_NS_DM, kArtist, wavInfoArtistChunk, &strArtist, &digestStr, &md5Ctx );
		PrepareLegacyExport ( kXMP_NS_DM, kLogComment, wavInfoCommentChunk, &strComment, &digestStr, &md5Ctx );
		PrepareLegacyExport ( kXMP_NS_DC, kCopyright, wavInfoCopyrightChunk, &strCopyright, &digestStr, &md5Ctx, true /* LangAlt */ );
		PrepareLegacyExport ( kXMP_NS_XMP, kCreateDate, wavInfoCreateDateChunk, &strCreateDate, &digestStr, &md5Ctx );
		PrepareLegacyExport ( kXMP_NS_DM, kEngineer, wavInfoEngineerChunk, &strEngineer, &digestStr, &md5Ctx );
		PrepareLegacyExport ( kXMP_NS_DM, kGenre, wavInfoGenreChunk, &strGenre, &digestStr, &md5Ctx );
		PrepareLegacyExport ( kXMP_NS_DM, kAlbum, wavInfoAlbumChunk, &strAlbum, &digestStr, &md5Ctx );
		PrepareLegacyExport ( kXMP_NS_XMP, kSoftware, wavInfoSoftwareChunk, &strSoftware, &digestStr, &md5Ctx );
		
		// Finish the digest and add it to the XMP.
		
		MD5Final ( md5Val, &md5Ctx );
		
		digestStr[digestStr.size()-1] = ';';
		for ( size_t i = 0; i < 16; ++i ) {
			XMP_Uns8 byte = md5Val[i];
			digestStr += hexDigits [byte >> 4];
			digestStr += hexDigits [byte & 0xF];
		}

		XMP_StringLen oldLen = this->xmpPacket.size();
		this->xmpObj.SetProperty ( kXMP_NS_WAV, "NativeDigest", digestStr.c_str() );
		try {
			this->xmpObj.SerializeToBuffer ( &this->xmpPacket, kXMP_ExactPacketLength, oldLen );
		} catch ( ... ) {
			this->xmpObj.SerializeToBuffer ( &this->xmpPacket, kXMP_UseCompactFormat );
		}
	
	}
	
	XMP_StringPtr packetStr = this->xmpPacket.c_str();
	XMP_StringLen packetLen = this->xmpPacket.size();
	if ( packetLen == 0 ) return;

	// Make sure we're writing an even number of bytes as required by the RIFF specification
	if ( (this->xmpPacket.size() & 1) == 1 ) this->xmpPacket.push_back (' ');
	XMP_Assert ( (this->xmpPacket.size() & 1) == 0 );
	packetStr = this->xmpPacket.c_str();	// ! Make sure they are current.
	packetLen = this->xmpPacket.size();
	
	LFA_FileRef fileRef ( this->parent->fileRef );
	if ( fileRef == 0 ) return;

	RIFF_Support::RiffState riffState;
	long numTags = RIFF_Support::OpenRIFF(fileRef, riffState);
	if ( numTags == 0 ) return;

	ok = RIFF_Support::PutChunk ( fileRef, riffState, formtypeWAVE, kXMPUserDataType, (char*)packetStr, packetLen );
	if ( ! ok ) return;

	// If needed, reconciliate the XMP data back into the native metadata.
	if ( fReconciliate ) {

		PutChunk ( fileRef, riffState, wavWaveTag, wavWaveTitleChunk, strTitle.c_str(), strTitle.size() );

		// Pad the old tags
		RIFF_Support::MarkChunkAsPadding ( fileRef, riffState, 0, wavInfoCreateDateChunk, 0 );
		RIFF_Support::MarkChunkAsPadding ( fileRef, riffState, 0, wavInfoArtistChunk, 0 );
		RIFF_Support::MarkChunkAsPadding ( fileRef, riffState, 0, wavInfoAlbumChunk, 0 );
		RIFF_Support::MarkChunkAsPadding ( fileRef, riffState, 0, wavInfoGenreChunk, 0 );
		RIFF_Support::MarkChunkAsPadding ( fileRef, riffState, 0, wavInfoCommentChunk, 0 );
		RIFF_Support::MarkChunkAsPadding ( fileRef, riffState, 0, wavInfoEngineerChunk, 0 );
		RIFF_Support::MarkChunkAsPadding ( fileRef, riffState, 0, wavInfoCopyrightChunk, 0 );
		RIFF_Support::MarkChunkAsPadding ( fileRef, riffState, 0, wavInfoSoftwareChunk, 0 );

		// Get the old INFO list
		std::string strOldInfo;
		unsigned long lOldSize = 0;
		bool ok = RIFF_Support::GetRIFFChunk ( fileRef, riffState, FOURCC_LIST, wavWaveTag, wavInfoTag, 0, &lOldSize );
		if ( ok ) {
			// We have to get rid of the "INFO" first.
			strOldInfo.reserve ( lOldSize );
			strOldInfo.assign ( lOldSize, ' ' );
			RIFF_Support::GetRIFFChunk ( fileRef, riffState, FOURCC_LIST, wavWaveTag, wavInfoTag, (char*)strOldInfo.c_str(), &lOldSize );
			// lOldSize -= 4;
		}

		// TODO: Cleaning up the OldInfo from the padding

		// Pad the old INFO list
		RIFF_Support::MarkChunkAsPadding ( fileRef, riffState, wavWaveTag, FOURCC_LIST, wavInfoTag );
		
		// Calculating the new INFO list size
		XMP_Int32 dwListSize = 4 + 8 * 8 +
						   GetStringRiffSize ( strCreateDate )  +
						   GetStringRiffSize ( strArtist ) +
						   GetStringRiffSize ( strAlbum ) +
						   GetStringRiffSize ( strGenre ) +
						   GetStringRiffSize ( strComment ) +
						   GetStringRiffSize ( strEngineer ) +
						   GetStringRiffSize ( strCopyright ) +
						   GetStringRiffSize ( strSoftware ) +
						   lOldSize;  // list id (4 bytes) + 8 tags hdrs (8 each)

		ok = MakeChunk ( fileRef, riffState, formtypeWAVE, dwListSize + 8 );
		if ( ! ok ) return; // If there's an error making a chunk, bail

		// Building the INFO list header
		RIFF_Support::ltag listtag;
		listtag.id = MakeUns32LE ( FOURCC_LIST );
		listtag.len = MakeUns32LE ( dwListSize );
		listtag.subid = MakeUns32LE ( wavInfoTag );
		LFA_Write ( fileRef, &listtag, 12 );

		// Writing all the chunks
		RIFF_Support::WriteChunk ( fileRef, wavInfoCreateDateChunk, strCreateDate.c_str(), GetStringRiffSize ( strCreateDate ) );
		RIFF_Support::WriteChunk ( fileRef, wavInfoArtistChunk, strArtist.c_str(), GetStringRiffSize ( strArtist ) );
		RIFF_Support::WriteChunk ( fileRef, wavInfoAlbumChunk, strAlbum.c_str(), GetStringRiffSize ( strAlbum ) );
		RIFF_Support::WriteChunk ( fileRef, wavInfoGenreChunk, strGenre.c_str(), GetStringRiffSize ( strGenre ) );
		RIFF_Support::WriteChunk ( fileRef, wavInfoCommentChunk, strComment.c_str(), GetStringRiffSize ( strComment ) );
		RIFF_Support::WriteChunk ( fileRef, wavInfoEngineerChunk, strEngineer.c_str(), GetStringRiffSize ( strEngineer ) );
		RIFF_Support::WriteChunk ( fileRef, wavInfoCopyrightChunk, strCopyright.c_str(), GetStringRiffSize ( strCopyright ) );
		RIFF_Support::WriteChunk ( fileRef, wavInfoSoftwareChunk, strSoftware.c_str(), GetStringRiffSize ( strSoftware ) );

		LFA_Write ( fileRef, strOldInfo.c_str(), lOldSize );

	}

	this->needsUpdate = false;

}	// WAV_MetaHandler::UpdateFile

// =================================================================================================
// WAV_MetaHandler::WriteFile
// ==========================

void WAV_MetaHandler::WriteFile ( LFA_FileRef         sourceRef,
								  const std::string & sourcePath )
{
	IgnoreParam(sourceRef); IgnoreParam(sourcePath);
	
	XMP_Throw ( "WAV_MetaHandler::WriteFile: Not supported", kXMPErr_Unavailable );

}	// WAV_MetaHandler::WriteFile

// =================================================================================================

static void AddDigestItem ( XMP_Uns32 legacyID, std::string & legacyStr, std::string * digestStr, MD5_CTX * md5 )
{
	
	XMP_Uns32 leID  = MakeUns32LE ( legacyID );
	XMP_Uns32 leLen = MakeUns32LE (legacyStr.size());
	
	digestStr->append ( (char*)(&leID), 4 );
	digestStr->append ( "," );
	
	MD5Update ( md5, (XMP_Uns8*)&leID, 4 );
	MD5Update ( md5, (XMP_Uns8*)&leLen, 4 );
	MD5Update ( md5, (XMP_Uns8*)legacyStr.c_str(), legacyStr.size() );

}	// AddDigestItem

// =================================================================================================

static void AddCurrentDigestItem ( LFA_FileRef fileRef, RIFF_Support::RiffState riffState, 
								   XMP_Uns32 tagID, XMP_Uns32 parentID, std::string * digestStr, MD5_CTX * md5 )
{
	unsigned long legacySize;
	std::string legacyStr;

	bool found = RIFF_Support::GetRIFFChunk ( fileRef, riffState, tagID, parentID, 0, 0, &legacySize );
	if ( found ) {
		legacyStr.reserve ( legacySize );
		legacyStr.assign ( legacySize, ' ' );
		(void) RIFF_Support::GetRIFFChunk ( fileRef, riffState, tagID, parentID, 0, (char*)legacyStr.c_str(), &legacySize );
	}

	AddDigestItem ( tagID, legacyStr, digestStr, md5 );

}	// AddCurrentDigestItem

// =================================================================================================

static void CreateCurrentDigest ( LFA_FileRef fileRef, RIFF_Support::RiffState riffState, std::string * digestStr )
{
	MD5_CTX md5Ctx;
	XMP_Uns8 md5Val[16];
	static char* hexDigits = "0123456789ABCDEF";
	
	MD5Init ( &md5Ctx );
	
	AddCurrentDigestItem ( fileRef, riffState, wavWaveTitleChunk, wavWaveTag, digestStr, &md5Ctx );
	AddCurrentDigestItem ( fileRef, riffState, wavInfoArtistChunk, wavInfoTag, digestStr, &md5Ctx );
	AddCurrentDigestItem ( fileRef, riffState, wavInfoCommentChunk, wavInfoTag, digestStr, &md5Ctx );
	AddCurrentDigestItem ( fileRef, riffState, wavInfoCopyrightChunk, wavInfoTag, digestStr, &md5Ctx);
	AddCurrentDigestItem ( fileRef, riffState, wavInfoCreateDateChunk, wavInfoTag, digestStr, &md5Ctx );
	AddCurrentDigestItem ( fileRef, riffState, wavInfoEngineerChunk, wavInfoTag, digestStr, &md5Ctx );
	AddCurrentDigestItem ( fileRef, riffState, wavInfoGenreChunk, wavInfoTag, digestStr, &md5Ctx );
	AddCurrentDigestItem ( fileRef, riffState, wavInfoAlbumChunk, wavInfoTag, digestStr, &md5Ctx );
	AddCurrentDigestItem ( fileRef, riffState, wavInfoSoftwareChunk, wavInfoTag, digestStr, &md5Ctx );
		
	MD5Final ( md5Val, &md5Ctx );
	
	(*digestStr)[digestStr->size()-1] = ';';
	for ( size_t i = 0; i < 16; ++i ) {
		XMP_Uns8 byte = md5Val[i];
		(*digestStr) += hexDigits [byte >> 4];
		(*digestStr) += hexDigits [byte & 0xF];
	}

}	// CreateCurrentDigest


// =================================================================================================
// WAV_MetaHandler::CacheFileData
// ==============================

void WAV_MetaHandler::CacheFileData()
{

	this->containsXMP = false;
	bool fReconciliate = ! (this->parent->openFlags & kXMPFiles_OpenOnlyXMP);
	bool keepExistingXMP = false;	// By default an import will replace existing XMP.
	bool haveLegacyItem, haveXMPItem;

	LFA_FileRef fileRef ( this->parent->fileRef );

	RIFF_Support::RiffState riffState;
	long numTags = RIFF_Support::OpenRIFF ( fileRef, riffState );
	if ( numTags == 0 ) return;

	// Determine the size of the metadata
	unsigned long bufferSize(0);
	haveLegacyItem = RIFF_Support::GetRIFFChunk ( fileRef, riffState, kXMPUserDataType, 0, 0, 0, &bufferSize );

	if ( ! haveLegacyItem ) {

		packetInfo.writeable = true;	// If no packet found, created packets will be writeable

	} else if ( bufferSize > 0 ) {

		// Size and clear the buffer
		this->xmpPacket.reserve(bufferSize);
		this->xmpPacket.assign(bufferSize, ' ');

		// Get the metadata
		haveLegacyItem = RIFF_Support::GetRIFFChunk ( fileRef, riffState, kXMPUserDataType, 0, 0,
													  const_cast<char *>(this->xmpPacket.data()), &bufferSize );
		if ( haveLegacyItem ) {
			this->packetInfo.offset = kXMPFiles_UnknownOffset;
			this->packetInfo.length = bufferSize;
			this->xmpObj.ParseFromBuffer ( this->xmpPacket.c_str(), this->xmpPacket.size() );
			this->containsXMP = true;
		}

	}

	// Figure out what to do overall. If there is no XMP, everything gets imported. If there is XMP
	// but no digest, only import if there is no corresponding XMP. If there is XMP and a digest
	// that matches, nothing is imported. If there is XMP and a digest that does not match, import
	// everything that provides "new info".
	
	if ( fReconciliate && this->containsXMP ) {
	
		std::string savedDigest;
		haveXMPItem = this->xmpObj.GetProperty ( kXMP_NS_WAV, "NativeDigest", &savedDigest, 0 );
		
		if ( ! haveXMPItem ) {
			keepExistingXMP = true;
		} else {
			std::string currDigest;
			CreateCurrentDigest ( fileRef, riffState, &currDigest );
			if ( currDigest == savedDigest ) fReconciliate = false;
		}
	
	}
	
	// Now import the individual legacy items.
	
	if ( fReconciliate ) {

		ImportLegacyItem ( riffState, wavWaveTitleChunk, wavWaveTag, kXMP_NS_DC, kTitle, keepExistingXMP, true /* LangAlt */ );
		ImportLegacyItem ( riffState, wavInfoCreateDateChunk, wavInfoTag, kXMP_NS_XMP, kCreateDate, keepExistingXMP );
		ImportLegacyItem ( riffState, wavInfoArtistChunk, wavInfoTag, kXMP_NS_DM, kArtist, keepExistingXMP );
		ImportLegacyItem ( riffState, wavInfoAlbumChunk, wavInfoTag, kXMP_NS_DM, kAlbum, keepExistingXMP );
		ImportLegacyItem ( riffState, wavInfoGenreChunk, wavInfoTag, kXMP_NS_DM, kGenre, keepExistingXMP );
		ImportLegacyItem ( riffState, wavInfoCommentChunk, wavInfoTag, kXMP_NS_DM, kLogComment, keepExistingXMP );
		ImportLegacyItem ( riffState, wavInfoEngineerChunk, wavInfoTag, kXMP_NS_DM, kEngineer, keepExistingXMP );
		ImportLegacyItem ( riffState, wavInfoCopyrightChunk, wavInfoTag, kXMP_NS_DC, kCopyright, keepExistingXMP, true /* LangAlt */ );
		ImportLegacyItem ( riffState, wavInfoSoftwareChunk, wavInfoTag, kXMP_NS_XMP, kSoftware, keepExistingXMP );

	}

	// Update the xmpPacket, as the xmpObj might have been updated with legacy info
	this->xmpObj.SerializeToBuffer ( &this->xmpPacket, kXMP_UseCompactFormat );
	this->packetInfo.offset = kXMPFiles_UnknownOffset;
	this->packetInfo.length = this->xmpPacket.size();

	this->processedXMP = this->containsXMP;
	
}	// WAV_MetaHandler::CacheFileData

// =================================================================================================

void WAV_MetaHandler::UTF8ToMBCS ( std::string * inoutStr )
{
	std::string localStr;

	try {
		ReconcileUtils::UTF8ToLocal ( inoutStr->c_str(), inoutStr->size(), &localStr );
		inoutStr->swap ( localStr );
	} catch ( ... ) {
		inoutStr->erase();
	}

}

// =================================================================================================

void WAV_MetaHandler::MBCSToUTF8 ( std::string * inoutStr )
{
	std::string utf8Str;

	try {
		ReconcileUtils::LocalToUTF8 ( inoutStr->c_str(), inoutStr->size(), &utf8Str );
		inoutStr->swap ( utf8Str );
	} catch ( ... ) {
		inoutStr->erase();
	}

}

// =================================================================================================

void WAV_MetaHandler::PrepareLegacyExport ( XMP_StringPtr xmpNS, XMP_StringPtr xmpProp, XMP_Uns32 legacyID, std::string * legacyStr,
											std::string * digestStr, MD5_CTX * md5, bool langAlt /* = false */ )
{
	if ( ! langAlt ) {
		this->xmpObj.GetProperty ( xmpNS, xmpProp, legacyStr, 0 );
	} else {
		this->xmpObj.GetLocalizedText ( xmpNS, xmpProp, "", "x-default", 0, legacyStr, 0 );
	}
	UTF8ToMBCS ( legacyStr );	// Convert the XMP value to local encoding.
	
	// ! Add a 0 pad byte if the value has an odd length. This would be better done inside RIFF_Support,
	// ! but that means changing too much at this moment.
	
	if ( (legacyStr->size() & 1) == 1 ) {
		(*legacyStr) += " ";
		(*legacyStr)[legacyStr->size()-1] = 0;
	}
	
	if ( legacyID == wavWaveTitleChunk ) {	// ! The title gets 32 bit LE type code of 1 inserted.
		legacyStr->insert ( 0, "1234" );
		PutUns32LE ( 1, (void*)legacyStr->c_str() );
	}
	
	AddDigestItem ( legacyID, *legacyStr, digestStr, md5 );

}	// WAV_MetaHandler::PrepareLegacyExport

// =================================================================================================

void WAV_MetaHandler::ImportLegacyItem ( RIFF_Support::RiffState & inOutRiffState,
										 XMP_Uns32 tagID,
										 XMP_Uns32 parentID,
										 XMP_StringPtr xmpNS,
										 XMP_StringPtr xmpProp,
										 bool keepExistingXMP,
										 bool langAlt /* = false */ )
{
	LFA_FileRef fileRef ( this->parent->fileRef );

	bool haveLegacyItem, haveXMPItem;
	std::string legacyStr, xmpStr;
	unsigned long legacySize;

	if ( ! langAlt ) {
		haveXMPItem = this->xmpObj.GetProperty ( xmpNS, xmpProp, &xmpStr, 0 );
	} else {
		haveXMPItem = this->xmpObj.GetLocalizedText ( xmpNS, xmpProp, "", "x-default", 0, &xmpStr, 0 );
	}

	haveLegacyItem = RIFF_Support::GetRIFFChunk ( fileRef, inOutRiffState, tagID, parentID, 0, 0, &legacySize );
	if ( (legacySize == 0) || ((tagID == wavWaveTitleChunk) && (legacySize <= 4)) ) haveLegacyItem = false;
	if ( haveXMPItem && keepExistingXMP ) haveLegacyItem = false;	// Simplify following checks.

	if ( ! haveLegacyItem ) {
	
		// No legacy item, delete the corresponding XMP if we're not keeping existing XMP.
	
		if ( haveXMPItem && (! keepExistingXMP) ) {
			if ( ! langAlt ) {
				this->xmpObj.DeleteProperty ( xmpNS, xmpProp );
			} else {
				std::string xdPath;
				SXMPUtils::ComposeLangSelector ( xmpNS, xmpProp, "x-default", &xdPath );
				this->xmpObj.DeleteProperty ( xmpNS, xdPath.c_str() );
				if ( this->xmpObj.CountArrayItems ( xmpNS, xmpProp ) == 0 ) this->xmpObj.DeleteProperty ( xmpNS, xmpProp );
			}
		}
	
	} else {
	
		// Have a legacy Item, update the XMP as appropriate.
	
		XMP_Assert ( (! haveXMPItem) || (! keepExistingXMP) );

		legacyStr.reserve ( legacySize );
		legacyStr.assign ( legacySize, ' ' );
		haveLegacyItem = RIFF_Support::GetRIFFChunk ( fileRef, inOutRiffState, tagID, parentID, 0, (char*)legacyStr.c_str(), &legacySize );
		XMP_Assert ( haveLegacyItem );
		
		if ( tagID == wavWaveTitleChunk ) {	// Check and strip the type code from the title.
			XMP_Assert ( legacySize > 4 );
			XMP_Uns32 typeCode = GetUns32LE ( legacyStr.data() );
			if( typeCode != 1 ) return;	// Bail if the type code isn't 1.
			legacyStr.erase ( 0, 4 );	// Reduce the value to just the text part.
		}
		
		if ( haveXMPItem ) {
			// Don't update the XMP if the legacy value matches the localized XMP value.
			UTF8ToMBCS ( &xmpStr );
			if ( xmpStr == legacyStr ) return;
		}
		
		MBCSToUTF8 ( &legacyStr );
		if ( ! langAlt ) {
			this->xmpObj.SetProperty ( xmpNS, xmpProp, legacyStr.c_str(), 0 );
		} else {
			this->xmpObj.SetLocalizedText ( xmpNS, xmpProp, "", "x-default", legacyStr.c_str(), 0 );
		}
		this->containsXMP = true;

	}

} // WAV_MetaHandler::LoadPropertyFromRIFF
