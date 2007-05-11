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

#include "MOV_Handler.hpp"
#include "QuickTime_Support.hpp"

#include "QuickTimeComponents.h"

#if XMP_WinBuild
	#include "QTML.h"
	#include "Movies.h"
#endif

#if XMP_MacBuild
	#include <Movies.h>
#endif

using namespace std;

static	OSType	kXMPUserDataType = 'XMP_';
static	long	kXMPUserDataTypeIndex = 1;

// =================================================================================================
/// \file MOV_Handler.cpp
/// \brief File format handler for MOV.
///
/// This header ...
///
// =================================================================================================

// =================================================================================================
// MOV_MetaHandlerCTor
// ===================

XMPFileHandler * MOV_MetaHandlerCTor ( XMPFiles * parent )
{
	return new MOV_MetaHandler ( parent );

}	// MOV_MetaHandlerCTor


// =================================================================================================
// MOV_CheckFormat
// ===============

bool MOV_CheckFormat ( XMP_FileFormat format,
					   XMP_StringPtr  filePath,
                       LFA_FileRef    fileRef,
                       XMPFiles *     parent )
{
	IgnoreParam(format); IgnoreParam(fileRef);
	
	XMP_Assert ( format == kXMP_MOVFile );
	XMP_Assert ( fileRef == 0 );
	
	bool inBackground = XMP_OptionIsSet ( parent->openFlags, kXMPFiles_OpenInBackground );

	if ( parent->format != kXMP_MOVFile ) return false;	// Check the first call hint, QT is promiscuous.
	if ( ! QuickTime_Support::sMainInitOK ) return false;
	
	if ( inBackground ) {
		if ( ! QuickTime_Support::ThreadInitialize() ) return false;
	}

	bool   isMov = false;
	OSErr  err = noErr;

	Handle qtDataRef = 0;
	OSType qtRefType;
	Movie  tempMovie = 0;
	short  flags;

	CFStringRef cfsPath = CFStringCreateWithCString ( 0, filePath, kCFStringEncodingUTF8 );
	if ( cfsPath == 0 ) return false;	// ? Throw?
		
	err = QTNewDataReferenceFromFullPathCFString ( cfsPath, kQTNativeDefaultPathStyle, 0, &qtDataRef, &qtRefType );
	if ( err != noErr ) goto EXIT;
	
	flags = newMovieDontResolveDataRefs | newMovieDontAskUnresolvedDataRefs;
	err = NewMovieFromDataRef ( &tempMovie, flags, 0, qtDataRef, qtRefType );
	if ( err != noErr ) goto EXIT;
	
	isMov = true;

EXIT:

	if ( tempMovie != 0 ) DisposeMovie ( tempMovie );
	if ( qtDataRef != 0 ) DisposeHandle ( qtDataRef );
	if ( cfsPath != 0 ) CFRelease ( cfsPath );

	if ( inBackground && (! isMov) ) QuickTime_Support::ThreadTerminate();
	return isMov;	

}	// MOV_CheckFormat

// =================================================================================================
// MOV_MetaHandler::MOV_MetaHandler
// ================================

MOV_MetaHandler::MOV_MetaHandler ( XMPFiles * _parent )
	: mQTInit(false), mMovieDataRef(0), mMovieDataHandler(0), mMovie(NULL), mMovieResourceID(0), mFilePermission(0)
{

	this->parent = _parent;
	this->handlerFlags = kMOV_HandlerFlags;
	this->stdCharForm  = kXMP_Char8Bit;

}	// MOV_MetaHandler::MOV_MetaHandler

// =================================================================================================
// MOV_MetaHandler::~MOV_MetaHandler
// =================================

MOV_MetaHandler::~MOV_MetaHandler()
{
	bool inBackground = XMP_OptionIsSet ( this->parent->openFlags, kXMPFiles_OpenInBackground );

	this->CloseMovie();
	if ( inBackground ) QuickTime_Support::ThreadTerminate();

}	// MOV_MetaHandler::~MOV_MetaHandler

// =================================================================================================
// MOV_MetaHandler::UpdateFile
// ===========================

void MOV_MetaHandler::UpdateFile ( bool doSafeUpdate )
{
	if ( ! this->needsUpdate ) return;
	if ( doSafeUpdate ) XMP_Throw ( "MOV_MetaHandler::UpdateFile: Safe update not supported", kXMPErr_Unavailable );

	XMP_StringPtr packetStr = this->xmpPacket.c_str();
	XMP_StringLen packetLen = this->xmpPacket.size();

	if ( packetLen == 0 ) return;	// Bail if no XMP packet
	
	if ( this->OpenMovie ( (kDataHCanRead | kDataHCanWrite) ) ) {

		UserData movieUserData ( GetMovieUserData ( this->mMovie ) );
		if ( movieUserData != 0 ) {

			OSErr err;

			// Remove previous versions
			err = GetUserData ( movieUserData, NULL, kXMPUserDataType, kXMPUserDataTypeIndex );
			if ( err == noErr ) {
				RemoveUserData(movieUserData, kXMPUserDataType, kXMPUserDataTypeIndex);
			}

			// Add the new one
			Handle XMPdata ( NewHandle(packetLen) );
			if ( XMPdata != 0 ) {
				HLock ( XMPdata );
				strncpy ( *XMPdata, packetStr, packetLen );	// AUDIT: Handle created above using packetLen.
				HUnlock ( XMPdata );
				err = AddUserData ( movieUserData, XMPdata, kXMPUserDataType );
				DisposeHandle ( XMPdata );
			}

		}

	}

	this->needsUpdate = false;

}	// MOV_MetaHandler::UpdateFile

// =================================================================================================
// MOV_MetaHandler::WriteFile
// ==========================

void MOV_MetaHandler::WriteFile ( LFA_FileRef         sourceRef,
								  const std::string & sourcePath )
{
	IgnoreParam(sourceRef); IgnoreParam(sourcePath);
	
	XMP_Throw ( "MOV_MetaHandler::WriteFile: Not supported", kXMPErr_Unavailable );

}	// MOV_MetaHandler::WriteFile

// =================================================================================================
// MOV_MetaHandler::OpenMovie
// ==========================

bool MOV_MetaHandler::OpenMovie ( long inPermission )
{
	// If the file is already opened with the correct permission, bail
	if ( (inPermission == this->mFilePermission) && (this->mMovie != 0) ) return true;

	// If already open, close to open with new permissions
	if ( (this->mMovieDataRef != 0) || (this->mMovieDataHandler != 0) || (this->mMovie != 0) ) this->CloseMovie();
	XMP_Assert ( (this->mMovieDataRef == 0) && (this->mMovieDataHandler == 0) && (this->mMovie == 0) );

	OSErr  err = noErr;
	OSType qtRefType;
	short  flags;

	CFStringRef cfsPath = CFStringCreateWithCString ( 0, this->parent->filePath.c_str(), kCFStringEncodingUTF8 );
	if ( cfsPath == 0 ) return false;	// ? Throw?
		
	err = QTNewDataReferenceFromFullPathCFString ( cfsPath, kQTNativeDefaultPathStyle, 0,
												   &this->mMovieDataRef, &qtRefType );
	if ( err != noErr ) goto FAILURE;
	
	this->mFilePermission = inPermission;
	err = OpenMovieStorage ( this->mMovieDataRef, qtRefType, this->mFilePermission, &this->mMovieDataHandler );
	if ( err != noErr ) goto FAILURE;
	
	flags = newMovieDontAskUnresolvedDataRefs;
	this->mMovieResourceID = 0;	// *** Is this the right input value?
	err = NewMovieFromDataRef ( &this->mMovie, flags, &this->mMovieResourceID, this->mMovieDataRef, qtRefType );
	if ( err != noErr ) goto FAILURE;

	if ( cfsPath != 0 ) CFRelease ( cfsPath );
	return true;

FAILURE:

	if ( this->mMovie != 0 ) DisposeMovie ( this->mMovie );
	if ( this->mMovieDataHandler != 0 ) CloseMovieStorage ( this->mMovieDataHandler );
	if ( this->mMovieDataRef != 0 ) DisposeHandle ( this->mMovieDataRef );
	
	this->mMovie = 0;
	this->mMovieDataHandler = 0;
	this->mMovieDataRef = 0;

	if ( cfsPath != 0 ) CFRelease ( cfsPath );	
	return false;

}	// MOV_MetaHandler::OpenMovie

// =================================================================================================
// MOV_MetaHandler::CloseMovie
// ===========================

void MOV_MetaHandler::CloseMovie()
{

	if ( this->mMovie != 0 ) {
		if ( HasMovieChanged ( this->mMovie ) ) {	// If the movie has been modified, write it to disk.
			(void) UpdateMovieInStorage ( this->mMovie, this->mMovieDataHandler );
		}
		DisposeMovie ( this->mMovie );
	}

	if ( this->mMovieDataHandler != 0 ) CloseMovieStorage ( this->mMovieDataHandler );
	if ( this->mMovieDataRef != 0 ) DisposeHandle ( this->mMovieDataRef );
	
	this->mMovie = 0;
	this->mMovieDataHandler = 0;
	this->mMovieDataRef = 0;

}	// MOV_MetaHandler::CloseMovie

// =================================================================================================
// MOV_MetaHandler::CacheFileData
// ==============================

void MOV_MetaHandler::CacheFileData()
{

	this->containsXMP = false;
	
	if ( this->OpenMovie ( kDataHCanRead ) ) {

		UserData movieUserData ( GetMovieUserData ( this->mMovie ) );
		if ( movieUserData != 0 ) {

			Handle XMPdataHandle ( NewHandle(0) );
			if ( XMPdataHandle != 0 ) {
			
				OSErr err = GetUserData ( movieUserData, XMPdataHandle, kXMPUserDataType, kXMPUserDataTypeIndex );
				if (err != noErr) {	// userDataItemNotFound = -2026

					packetInfo.writeable = true;	// If no packet found, created packets will be writeable

				} else {

					// Convert handles data, to std::string raw
					this->xmpPacket.clear();
					size_t dataSize = GetHandleSize ( XMPdataHandle );
					HLock(XMPdataHandle);
					this->xmpPacket.assign ( (const char*)(*XMPdataHandle), dataSize );
					HUnlock ( XMPdataHandle );

					this->packetInfo.offset = kXMPFiles_UnknownOffset;
					this->packetInfo.length = (XMP_Int32)dataSize;
					this->containsXMP = true;

				}
	
				DisposeHandle ( XMPdataHandle );

			}

		}

	}

}	// MOV_MetaHandler::CacheFileData

// =================================================================================================
