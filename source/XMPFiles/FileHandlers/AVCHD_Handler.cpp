// =================================================================================================
// ADOBE SYSTEMS INCORPORATED
// Copyright 2002-2008 Adobe Systems Incorporated
// All Rights Reserved
//
// NOTICE: Adobe permits you to use, modify, and distribute this file in accordance with the terms
// of the Adobe license agreement accompanying it.
// =================================================================================================

#include <stdlib.h>

#include "AVCHD_Handler.hpp"

#include "MD5.h"

using namespace std;

// =================================================================================================
/// \file AVCHD_Handler.cpp
/// \brief Folder format handler for AVCHD.
///
/// This handler is for the AVCHD video format. 
///
/// A typical AVCHD layout looks like:
///
///     BDMV/
///         index.bdmv
///         MovieObject.bdmv
///         PLAYLIST/
///				00000.mpls
///             00001.mpls
///         STREAM/
///				00000.m2ts
///             00001.m2ts
///         CLIPINF/
///				00000.clpi
///             00001.clpi
///         BACKUP/
///
// =================================================================================================

// =================================================================================================

// AVCHD Format. Book 1: Playback System Basic Specifications V 1.01. p. 76

struct AVCHD_blkProgramInfo
{
	XMP_Uns32	mLength;
	XMP_Uns8	mReserved1[2];
	XMP_Uns32	mSPNProgramSequenceStart;
	XMP_Uns16	mProgramMapPID;
	XMP_Uns8	mNumberOfStreamsInPS;
	XMP_Uns8	mReserved2;

	// Video stream.
	struct
	{	
		XMP_Uns8	mPresent;
		XMP_Uns8	mVideoFormat;
		XMP_Uns8	mFrameRate;
		XMP_Uns8	mAspectRatio;
		XMP_Uns8	mCCFlag;
	} mVideoStream;

	// Audio stream.
	struct
	{
		XMP_Uns8	mPresent;
		XMP_Uns8	mAudioPresentationType;
		XMP_Uns8	mSamplingFrequency;
		XMP_Uns8	mAudioLanguageCode[4];
	} mAudioStream;

	// Pverlay bitmap stream.
	struct
	{
		XMP_Uns8	mPresent;
		XMP_Uns8	mOBLanguageCode[4];
	} mOverlayBitmapStream;

	// Menu bitmap stream.
	struct  
	{
		XMP_Uns8	mPresent;
		XMP_Uns8	mBMLanguageCode[4];
	} mMenuBitmapStream;

};

// =================================================================================================
// AVCHD_CheckFormat
// =================
//
// This version checks for the presence of a top level BPAV directory, and the required files and
// directories immediately within it. The CLIPINF, PLAYLIST, and STREAM subfolders are required, as
// are the index.bdmv and MovieObject.bdmv files.
//
// The state of the string parameters depends on the form of the path passed by the client. If the
// client passed a logical clip path, like ".../MyMovie/00001", the parameters are:
//   rootPath   - ".../MyMovie"
//   gpName     - empty
//   parentName - empty
//   leafName   - "00001"
// If the client passed a full file path, like ".../MyMovie/BDMV/CLIPINF/00001.clpi", they are:
//   rootPath   - ".../MyMovie"
//   gpName     - "BDMV"
//   parentName - "CLIPINF" or "PALYLIST" or "STREAM"
//   leafName   - "00001"

// ! The common code has shifted the gpName, parentName, and leafName strings to upper case. It has
// ! also made sure that for a logical clip path the rootPath is an existing folder, and that the
// ! file exists for a full file path.

// ! Using explicit '/' as a separator when creating paths, it works on Windows.

// ! Sample files show that the ".bdmv" extension can sometimes be ".bdm". Allow either.

bool AVCHD_CheckFormat ( XMP_FileFormat format,
						 const std::string & rootPath,
						 const std::string & gpName,
						 const std::string & parentName,
						 const std::string & leafName,
						 XMPFiles * parent ) 
{
	if ( gpName.empty() != parentName.empty() ) return false;	// Must be both empty or both non-empty.
	
	if ( ! gpName.empty() ) {
		if ( gpName != "BDMV" ) return false;
		if ( (parentName != "CLIPINF") && (parentName != "PLAYLIST") && (parentName != "STREAM") ) return false;
	}
	
	// Check the rest of the required general structure. Look for both ".bdmv" and ".bmd" extensions.
	
	std::string bdmvPath ( rootPath );
	bdmvPath += kDirChar;
	bdmvPath += "BDMV";

	if ( GetChildMode ( bdmvPath, "CLIPINF" ) != kFMode_IsFolder ) return false;
	if ( GetChildMode ( bdmvPath, "PLAYLIST" ) != kFMode_IsFolder ) return false;
	if ( GetChildMode ( bdmvPath, "STREAM" ) != kFMode_IsFolder ) return false;

	if ( (GetChildMode ( bdmvPath, "index.bdmv" ) != kFMode_IsFile) &&
		 (GetChildMode ( bdmvPath, "index.bdm" ) != kFMode_IsFile) ) return false;

	if ( (GetChildMode ( bdmvPath, "MovieObject.bdmv" ) != kFMode_IsFile) &&
		 (GetChildMode ( bdmvPath, "MovieObj.bdm" ) != kFMode_IsFile) ) return false;

	
	// Make sure the .clpi file exists.
	std::string tempPath ( bdmvPath );
	tempPath += kDirChar;
	tempPath += "CLIPINF";
	tempPath += kDirChar;
	tempPath += leafName;
	tempPath += ".clpi";
	const bool foundClpi = ( GetFileMode ( tempPath.c_str() ) == kFMode_IsFile );
	
	// No .clpi -- check for .cpi instead.
	if ( ! foundClpi ) {
		tempPath.erase ( tempPath.size() - 5 );	// Remove the ".clpi" part.
		tempPath += ".cpi";
		if ( GetFileMode ( tempPath.c_str() ) != kFMode_IsFile ) return false;
	}

	// And now save the pseudo path for the handler object.
	tempPath = rootPath;
	tempPath += kDirChar;
	tempPath += leafName;
	size_t pathLen = tempPath.size() + 1;	// Include a terminating nul.
	parent->handlerTemp = malloc ( pathLen );
	if ( parent->handlerTemp == 0 ) XMP_Throw ( "No memory for AVCHD clip info", kXMPErr_NoMemory );
	memcpy ( parent->handlerTemp, tempPath.c_str(), pathLen );
	
	return true;
	
}	// AVCHD_CheckFormat

// =================================================================================================
// ReadAVCHDProgramInfo
// ====================

static bool ReadAVCHDProgramInfo ( const std::string& strPath, AVCHD_blkProgramInfo& avchdProgramInfo ) 
{
	try {

		AutoFile idxFile;
		idxFile.fileRef = LFA_Open ( strPath.c_str(), 'r' );
		if ( idxFile.fileRef == 0 ) return false;	// The open failed.

		memset ( &avchdProgramInfo, 0, sizeof(AVCHD_blkProgramInfo) );

		// Read clip header. (AVCHD Format. Book1. v. 1.01. p 64 ) 
		struct AVCHD_ClipInfoHeader
		{
			XMP_Uns8	mTypeIndicator[4];
			XMP_Uns8	mTypeIndicator2[4];
			XMP_Uns32	mSequenceInfoStartAddress;
			XMP_Uns32	mProgramInfoStartAddress;
			XMP_Uns32	mCPIStartAddress; 
			XMP_Uns32	mExtensionDataStartAddress;
			XMP_Uns8	mReserved[12];
		};

		// Read the AVCHD header.
		AVCHD_ClipInfoHeader avchdHeader;
		LFA_Read ( idxFile.fileRef, avchdHeader.mTypeIndicator,  4 );
		LFA_Read ( idxFile.fileRef, avchdHeader.mTypeIndicator2, 4 );
		avchdHeader.mSequenceInfoStartAddress	= LFA_ReadUns32_BE ( idxFile.fileRef );
		avchdHeader.mProgramInfoStartAddress	= LFA_ReadUns32_BE ( idxFile.fileRef );
		avchdHeader.mCPIStartAddress			= LFA_ReadUns32_BE ( idxFile.fileRef );
		avchdHeader.mExtensionDataStartAddress	= LFA_ReadUns32_BE ( idxFile.fileRef );
		LFA_Read ( idxFile.fileRef, avchdHeader.mReserved, 12 );

		// Seek to the program header. (AVCHD Format. Book1. v. 1.01. p 77 ) 
		LFA_Seek ( idxFile.fileRef, avchdHeader.mProgramInfoStartAddress, SEEK_SET );

		avchdProgramInfo.mLength = LFA_ReadUns32_BE ( idxFile.fileRef );
		LFA_Read ( idxFile.fileRef, avchdProgramInfo.mReserved1, 2 );
		avchdProgramInfo.mSPNProgramSequenceStart = LFA_ReadUns32_BE ( idxFile.fileRef );
		avchdProgramInfo.mProgramMapPID = LFA_ReadUns16_BE ( idxFile.fileRef ); 
		LFA_Read ( idxFile.fileRef, &avchdProgramInfo.mNumberOfStreamsInPS, 1 );
		LFA_Read ( idxFile.fileRef, &avchdProgramInfo.mReserved2, 1 );

		XMP_Uns16 streamPID = 0;
		for ( int i=0; i<avchdProgramInfo.mNumberOfStreamsInPS; ++i ) {

			XMP_Uns8	length = 0;
			XMP_Uns8	streamCodingType = 0;

			streamPID = LFA_ReadUns16_BE ( idxFile.fileRef );
			LFA_Read ( idxFile.fileRef, &length, 1 );

			XMP_Int64 pos = LFA_Tell ( idxFile.fileRef );

			LFA_Read ( idxFile.fileRef, &streamCodingType, 1 );

			switch ( streamCodingType ) {

				case 0x1B	: // Video stream case.
					{ 
						XMP_Uns8 videoFormatAndFrameRate;
						LFA_Read ( idxFile.fileRef, &videoFormatAndFrameRate, 1 );
						avchdProgramInfo.mVideoStream.mVideoFormat	= videoFormatAndFrameRate >> 4;    // hi 4 bits
						avchdProgramInfo.mVideoStream.mFrameRate	= videoFormatAndFrameRate & 0x0f;  // lo 4 bits
	
						XMP_Uns8 aspectRatioAndReserved = 0;
						LFA_Read ( idxFile.fileRef, &aspectRatioAndReserved, 1 );
						avchdProgramInfo.mVideoStream.mAspectRatio	 = aspectRatioAndReserved >> 4; // hi 4 bits
	
						XMP_Uns8 ccFlag = 0;
						LFA_Read ( idxFile.fileRef, &ccFlag, 1 );
						avchdProgramInfo.mVideoStream.mCCFlag = ccFlag;
	
						avchdProgramInfo.mVideoStream.mPresent = 1;
					}
					break;
	
				case 0x80   : // Fall through.
				case 0x81	: // Audio stream case.
					{
						XMP_Uns8 audioPresentationTypeAndFrequency = 0;
						LFA_Read ( idxFile.fileRef, &audioPresentationTypeAndFrequency, 1 );
	
						avchdProgramInfo.mAudioStream.mAudioPresentationType = audioPresentationTypeAndFrequency >> 4;    // hi 4 bits
						avchdProgramInfo.mAudioStream.mSamplingFrequency	 = audioPresentationTypeAndFrequency & 0x0f;  // lo 4 bits
	
						LFA_Read ( idxFile.fileRef, avchdProgramInfo.mAudioStream.mAudioLanguageCode, 3 );
						avchdProgramInfo.mAudioStream.mAudioLanguageCode[3] = 0;
	
						avchdProgramInfo.mAudioStream.mPresent = 1;
					}
					break;
	
				case 0x90	: // Overlay bitmap stream case.
					LFA_Read ( idxFile.fileRef, &avchdProgramInfo.mOverlayBitmapStream.mOBLanguageCode, 3 );
					avchdProgramInfo.mOverlayBitmapStream.mOBLanguageCode[3] = 0;
					avchdProgramInfo.mOverlayBitmapStream.mPresent = 1;
					break;
	
				case 0x91   : // Menu bitmap stream.
					LFA_Read ( idxFile.fileRef, &avchdProgramInfo.mMenuBitmapStream.mBMLanguageCode, 3 );
					avchdProgramInfo.mMenuBitmapStream.mBMLanguageCode[3] = 0;
					avchdProgramInfo.mMenuBitmapStream.mPresent = 1;
					break;
	
				default :
					break;

			}

			LFA_Seek ( idxFile.fileRef, pos + length, SEEK_SET );

		}

	} catch ( ... ) {

		return false;

	}

	return true;

}	// ReadAVCHDProgramInfo

// =================================================================================================
// AVCHD_MetaHandlerCTor
// =====================

XMPFileHandler * AVCHD_MetaHandlerCTor ( XMPFiles * parent ) 
{
	return new AVCHD_MetaHandler ( parent );
	
}	// AVCHD_MetaHandlerCTor

// =================================================================================================
// AVCHD_MetaHandler::AVCHD_MetaHandler
// ====================================

AVCHD_MetaHandler::AVCHD_MetaHandler ( XMPFiles * _parent )
{
	this->parent = _parent;	// Inherited, can't set in the prefix.
	this->handlerFlags = kAVCHD_HandlerFlags;
	this->stdCharForm  = kXMP_Char8Bit;
	
	// Extract the root path and clip name.
	
	XMP_Assert ( this->parent->handlerTemp != 0 );
	
	this->rootPath.assign ( (char*) this->parent->handlerTemp );
	free ( this->parent->handlerTemp );
	this->parent->handlerTemp = 0;

	SplitLeafName ( &this->rootPath, &this->clipName );
	
}	// AVCHD_MetaHandler::AVCHD_MetaHandler

// =================================================================================================
// AVCHD_MetaHandler::~AVCHD_MetaHandler
// =====================================

AVCHD_MetaHandler::~AVCHD_MetaHandler()
{

	if ( this->parent->handlerTemp != 0 ) {
		free ( this->parent->handlerTemp );
		this->parent->handlerTemp = 0;
	}

}	// AVCHD_MetaHandler::~AVCHD_MetaHandler

// =================================================================================================
// AVCHD_MetaHandler::MakeClipInfoPath
// ===================================

void AVCHD_MetaHandler::MakeClipInfoPath ( std::string * path, XMP_StringPtr suffix ) 
{

	*path = this->rootPath;
	*path += kDirChar;
	*path += "BDMV";
	*path += kDirChar;
	*path += "CLIPINF";
	*path += kDirChar;
	*path += this->clipName;
	*path += suffix;

}	// AVCHD_MetaHandler::MakeClipInfoPath

// =================================================================================================
// AVCHD_MetaHandler::MakeClipStreamPath
// =====================================

void AVCHD_MetaHandler::MakeClipStreamPath ( std::string * path, XMP_StringPtr suffix ) 
{

	*path = this->rootPath;
	*path += kDirChar;
	*path += "BDMV";
	*path += kDirChar;
	*path += "STREAM";
	*path += kDirChar;
	*path += this->clipName;
	*path += suffix;

}	// AVCHD_MetaHandler::MakeClipStreamPath

// =================================================================================================
// AVCHD_MetaHandler::MakeLegacyDigest 
// ===================================

#define kHexDigits "0123456789ABCDEF"

void AVCHD_MetaHandler::MakeLegacyDigest ( std::string * digestStr )
{
	AVCHD_blkProgramInfo avchdProgramInfo;
	std::string strPath;
	this->MakeClipInfoPath ( &strPath, ".clpi" );

	if ( ! ReadAVCHDProgramInfo ( strPath, avchdProgramInfo ) ) {
		this->MakeClipInfoPath ( &strPath, ".cpi" );
		if ( ! ReadAVCHDProgramInfo ( strPath, avchdProgramInfo ) ) return;
	}

	MD5_CTX context;
	unsigned char digestBin [16];

	MD5Init ( &context );
	MD5Update ( &context, (XMP_Uns8*)&avchdProgramInfo, (unsigned int) sizeof(avchdProgramInfo) );
	MD5Final ( digestBin, &context );

	char buffer [40];
	for ( int in = 0, out = 0; in < 16; in += 1, out += 2 ) {
		XMP_Uns8 byte = digestBin[in];
		buffer[out]   = kHexDigits [ byte >> 4 ];
		buffer[out+1] = kHexDigits [ byte & 0xF ];
	}
	buffer[32] = 0;
	digestStr->erase();
	digestStr->append ( buffer, 32 );

}	// AVCHD_MetaHandler::MakeLegacyDigest

// =================================================================================================
// AVCHD_MetaHandler::CacheFileData
// ================================

void AVCHD_MetaHandler::CacheFileData() 
{
	XMP_Assert ( (! this->containsXMP) && (! this->containsTNail) );
	
	// See if the clip's .XMP file exists.
	
	std::string xmpPath;
	this->MakeClipStreamPath ( &xmpPath, ".xmp" );
	if ( GetFileMode ( xmpPath.c_str() ) != kFMode_IsFile ) return;	// No XMP.
	
	// Read the entire .XMP file.
	
	char openMode = 'r';
	if ( this->parent->openFlags & kXMPFiles_OpenForUpdate ) openMode = 'w';
	
	LFA_FileRef xmpFile = LFA_Open ( xmpPath.c_str(), openMode );
	if ( xmpFile == 0 ) return;	// The open failed.
	
	XMP_Int64 xmpLen = LFA_Measure ( xmpFile );
	if ( xmpLen > 100*1024*1024 ) {
		XMP_Throw ( "AVCHD XMP is outrageously large", kXMPErr_InternalFailure );	// Sanity check.
	}
	
	this->xmpPacket.erase();
	this->xmpPacket.reserve ( (size_t ) xmpLen );
	this->xmpPacket.append ( (size_t ) xmpLen, ' ' );

	XMP_Int32 ioCount = LFA_Read ( xmpFile, (void*)this->xmpPacket.data(), (XMP_Int32)xmpLen, kLFA_RequireAll );
	XMP_Assert ( ioCount == xmpLen );
	
	this->packetInfo.offset = 0;
	this->packetInfo.length = (XMP_Int32)xmpLen;
	FillPacketInfo ( this->xmpPacket, &this->packetInfo );
	
	XMP_Assert ( this->parent->fileRef == 0 );
	if ( openMode == 'r' ) {
		LFA_Close ( xmpFile );
	} else {
		this->parent->fileRef = xmpFile;
	}
	
	this->containsXMP = true;
	
}	// AVCHD_MetaHandler::CacheFileData

// =================================================================================================
// AVCHD_MetaHandler::ProcessXMP
// =============================

void AVCHD_MetaHandler::ProcessXMP() 
{
	if ( this->processedXMP ) return;
	this->processedXMP = true;	// Make sure only called once.

	if ( this->containsXMP ) {
		this->xmpObj.ParseFromBuffer ( this->xmpPacket.c_str(), (XMP_StringLen)this->xmpPacket.size() );
	}

	// read clip info
	AVCHD_blkProgramInfo avchdProgramInfo;
	std::string strPath;
	this->MakeClipInfoPath ( &strPath, ".clpi" );

	if ( ! ReadAVCHDProgramInfo ( strPath, avchdProgramInfo ) ) {
		this->MakeClipInfoPath ( &strPath, ".cpi" );
		if ( ! ReadAVCHDProgramInfo ( strPath, avchdProgramInfo ) ) return;
	}

	// Video Stream. AVCHD Format v. 1.01 p. 78

	XMP_StringPtr xmpValue = 0;

	if ( avchdProgramInfo.mVideoStream.mPresent ) {

		// XMP videoFrameRate.
		xmpValue = 0;
		switch ( avchdProgramInfo.mVideoStream.mFrameRate ) {
			case 1 : xmpValue = "23.98p";	break;   // "23.976"
			case 2 : xmpValue = "24p";		break;   // "24"	
			case 3 : xmpValue = "25p";		break;   // "25"	
			case 4 : xmpValue = "29.97p";	break;   // "29.97"
			case 6 : xmpValue = "50i";		break;   // "50"	
			case 7 : xmpValue = "59.94i";	break;   // "59.94"
			default: break;
		}
		if ( xmpValue != 0 ) {
			this->xmpObj.SetProperty ( kXMP_NS_DM, "videoFrameRate", xmpValue, kXMP_DeleteExisting );
		}

		// XMP videoFrameSize.
		xmpValue = 0;
		int frameIndex = -1;
		const char* frameWidth[4]	= { "720", "720", "1280", "1920" };
		const char* frameHeight[4]	= { "480", "576", "720",  "1080" };
		switch ( avchdProgramInfo.mVideoStream.mVideoFormat ) {
			case 1 : frameIndex = 0; break; // 480i
			case 2 : frameIndex = 1; break; // 576i
			case 3 : frameIndex = 0; break; // 480i
			case 4 : frameIndex = 3; break; // 1080i
			case 5 : frameIndex = 2; break; // 720p
			case 6 : frameIndex = 3; break; // 1080p
			default: break;
		}
		if ( frameIndex != -1 ) {
			xmpValue = frameWidth[frameIndex];
			this->xmpObj.SetStructField ( kXMP_NS_DM, "videoFrameSize", kXMP_NS_XMP_Dimensions, "w", xmpValue, 0 );
			xmpValue = frameHeight[frameIndex];
			this->xmpObj.SetStructField ( kXMP_NS_DM, "videoFrameSize", kXMP_NS_XMP_Dimensions, "h", xmpValue, 0 );
			xmpValue = "pixels";
			this->xmpObj.SetStructField ( kXMP_NS_DM, "videoFrameSize", kXMP_NS_XMP_Dimensions, "unit", xmpValue, 0 );
		}

		this->containsXMP = true;

	}
	
	// Audio Stream.
	if ( avchdProgramInfo.mAudioStream.mPresent ) {

		xmpValue = 0;
		switch ( avchdProgramInfo.mAudioStream.mAudioPresentationType ) {
			case 1  : xmpValue = "Mono"; break;
			case 3  : xmpValue = "Stereo"; break;
			default : break;
		}
		if ( xmpValue != 0 ) {
			this->xmpObj.SetProperty ( kXMP_NS_DM, "audioChannelType", xmpValue, kXMP_DeleteExisting );
		}

		xmpValue = 0;
		switch ( avchdProgramInfo.mAudioStream.mSamplingFrequency ) {
			case 1  : xmpValue = "48000"; break;
			case 4  : xmpValue = "96000"; break;
			case 5  : xmpValue = "192000"; break;
			default : break;
		}
		if ( xmpValue != 0 ) {
			this->xmpObj.SetProperty ( kXMP_NS_DM, "audioSampleRate", xmpValue, kXMP_DeleteExisting );
		}

		this->containsXMP = true;

	}

}	// AVCHD_MetaHandler::ProcessXMP

// =================================================================================================
// AVCHD_MetaHandler::UpdateFile
// =============================
//
// Note that UpdateFile is only called from XMPFiles::CloseFile, so it is OK to close the file here.

void AVCHD_MetaHandler::UpdateFile ( bool doSafeUpdate ) 
{
	if ( ! this->needsUpdate ) return;
	this->needsUpdate = false;	// Make sure only called once.

	std::string newDigest;
	this->MakeLegacyDigest ( &newDigest );
	this->xmpObj.SetStructField ( kXMP_NS_XMP, "NativeDigests", kXMP_NS_XMP, "AVCHD", newDigest.c_str(), kXMP_DeleteExisting );

	LFA_FileRef oldFile = this->parent->fileRef;
	
	this->xmpObj.SerializeToBuffer ( &this->xmpPacket, this->GetSerializeOptions() );

	if ( oldFile == 0 ) {
	
		// The XMP does not exist yet.

		std::string xmpPath;
		this->MakeClipStreamPath ( &xmpPath, ".xmp" );
		
		LFA_FileRef xmpFile = LFA_Create ( xmpPath.c_str() );
		if ( xmpFile == 0 ) XMP_Throw ( "Failure creating AVCHD XMP file", kXMPErr_ExternalFailure );
		LFA_Write ( xmpFile, this->xmpPacket.data(), (XMP_StringLen)this->xmpPacket.size() );
		LFA_Close ( xmpFile );
	
	} else if ( ! doSafeUpdate ) {

		// Over write the existing XMP file.
		
		LFA_Seek ( oldFile, 0, SEEK_SET );
		LFA_Truncate ( oldFile, 0 );
		LFA_Write ( oldFile, this->xmpPacket.data(), (XMP_StringLen)this->xmpPacket.size() );
		LFA_Close ( oldFile );

	} else {

		// Do a safe update.
		
		// *** We really need an LFA_SwapFiles utility.
		
		std::string xmpPath, tempPath;
		
		this->MakeClipStreamPath ( &xmpPath, ".xmp" );
		
		CreateTempFile ( xmpPath, &tempPath );
		LFA_FileRef tempFile = LFA_Open ( tempPath.c_str(), 'w' );
		LFA_Write ( tempFile, this->xmpPacket.data(), (XMP_StringLen)this->xmpPacket.size() );
		LFA_Close ( tempFile );
		
		LFA_Close ( oldFile );
		LFA_Delete ( xmpPath.c_str() );
		LFA_Rename ( tempPath.c_str(), xmpPath.c_str() );

	}

	this->parent->fileRef = 0;
	
}	// AVCHD_MetaHandler::UpdateFile

// =================================================================================================
// AVCHD_MetaHandler::WriteFile
// ============================

void AVCHD_MetaHandler::WriteFile ( LFA_FileRef sourceRef, const std::string & sourcePath ) 
{

	// ! WriteFile is not supposed to be called for handlers that own the file.
	XMP_Throw ( "AVCHD_MetaHandler::WriteFile should not be called", kXMPErr_InternalFailure );
	
}	// AVCHD_MetaHandler::WriteFile

// =================================================================================================
