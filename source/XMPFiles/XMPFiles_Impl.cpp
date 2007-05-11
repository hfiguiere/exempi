// =================================================================================================
// ADOBE SYSTEMS INCORPORATED
// Copyright 2002-2007 Adobe Systems Incorporated
// All Rights Reserved
//
// NOTICE: Adobe permits you to use, modify, and distribute this file in accordance with the terms
// of the Adobe license agreement accompanying it.
// =================================================================================================

#include "XMPFiles_Impl.hpp"

#if XMP_MacBuild
	#include <Files.h>
#elif XMP_WinBuild
	#include <Windows.h>
#elif XMP_UNIXBuild
	#include <fcntl.h>
	#include <sys/stat.h>
	#include <unistd.h>
#endif

using namespace std;

// Internal code should be using #if with XMP_MacBuild, XMP_WinBuild, or XMP_UNIXBuild.
// This is a sanity check in case of accidental use of *_ENV. Some clients use the poor
// practice of defining the *_ENV macro with an empty value.
#if defined ( MAC_ENV )
	#if ! MAC_ENV
		#error "MAC_ENV must be defined so that \"#if MAC_ENV\" is true"
	#endif
#elif defined ( WIN_ENV )
	#if ! WIN_ENV
		#error "WIN_ENV must be defined so that \"#if WIN_ENV\" is true"
	#endif
#elif defined ( UNIX_ENV )
	#if ! UNIX_ENV
		#error "UNIX_ENV must be defined so that \"#if UNIX_ENV\" is true"
	#endif
#endif

// =================================================================================================
/// \file XMPFiles_Impl.cpp
/// \brief ...
///
/// This file ...
///
// =================================================================================================

#if XMP_WinBuild
	#pragma warning ( disable : 4290 )	// C++ exception specification ignored except to indicate a function is not __declspec(nothrow)
	#pragma warning ( disable : 4800 )	// forcing value to bool 'true' or 'false' (performance warning)
#endif

XMP_FileFormat voidFileFormat = 0;	// Used as sink for unwanted output parameters.
XMP_Mutex sXMPFilesLock;
int sXMPFilesLockCount = 0;
XMP_VarString * sXMPFilesExceptionMessage = 0;

#if TraceXMPCalls
	FILE * xmpFilesOut = stderr;
#endif

// =================================================================================================

const FileExtMapping kFileExtMap[] =	// Add all known mappings, multiple mappings (tif, tiff) are OK.
	{ { "pdf",  kXMP_PDFFile },
	  { "ps",   kXMP_PostScriptFile },
	  { "eps",  kXMP_EPSFile },

	  { "jpg",  kXMP_JPEGFile },
	  { "jpeg", kXMP_JPEGFile },
	  { "jpx",  kXMP_JPEG2KFile },
	  { "tif",  kXMP_TIFFFile },
	  { "tiff", kXMP_TIFFFile },
	  { "dng",  kXMP_TIFFFile },	// DNG files are well behaved TIFF.
	  { "gif",  kXMP_GIFFile },
	  { "giff", kXMP_GIFFile },
	  { "png",  kXMP_PNGFile },

	  { "swf",  kXMP_SWFFile },
	  { "flv",  kXMP_FLVFile },

	  { "aif",  kXMP_AIFFFile },

	  { "mov",  kXMP_MOVFile },
	  { "avi",  kXMP_AVIFile },
	  { "cin",  kXMP_CINFile },
	  { "wav",  kXMP_WAVFile },
	  { "mp3",  kXMP_MP3File },
	  { "mp4",  kXMP_MPEG4File },
	  { "ses",  kXMP_SESFile },
	  { "cel",  kXMP_CELFile },
	  { "wma",  kXMP_WMAVFile },
	  { "wmv",  kXMP_WMAVFile },

	  { "mpg",  kXMP_MPEGFile },
	  { "mpeg", kXMP_MPEGFile },
	  { "mp2",  kXMP_MPEGFile },
	  { "mod",  kXMP_MPEGFile },
	  { "m2v",  kXMP_MPEGFile },
	  { "mpa",  kXMP_MPEGFile },
	  { "mpv",  kXMP_MPEGFile },
	  { "m2p",  kXMP_MPEGFile },
	  { "m2a",  kXMP_MPEGFile },
	  { "m2t",  kXMP_MPEGFile },
	  { "mpe",  kXMP_MPEGFile },
	  { "vob",  kXMP_MPEGFile },
	  { "ms-pvr", kXMP_MPEGFile },
	  { "dvr-ms", kXMP_MPEGFile },

	  { "html", kXMP_HTMLFile },
	  { "xml",  kXMP_XMLFile },
	  { "txt",  kXMP_TextFile },
	  { "text", kXMP_TextFile },

	  { "psd",  kXMP_PhotoshopFile },
	  { "ai",   kXMP_IllustratorFile },
	  { "indd", kXMP_InDesignFile },
	  { "indt", kXMP_InDesignFile },
	  { "aep",  kXMP_AEProjectFile },
	  { "aet",  kXMP_AEProjTemplateFile },
	  { "ffx",  kXMP_AEFilterPresetFile },
	  { "ncor", kXMP_EncoreProjectFile },
	  { "prproj", kXMP_PremiereProjectFile },
	  { "prtl", kXMP_PremiereTitleFile },

	  { "", 0 } };	// ! Must be last as a sentinel.

const char * kKnownScannedFiles[] =	// Files known to contain XMP but have no smart handling, here or elsewhere.
	{ "gif",	// GIF, public format but no smart handler.
	  "ai",		// Illustrator, actually a PDF file.
	  "ait",	// Illustrator template, actually a PDF file.
	  "svg",	// SVG, an XML file.
	  "aet",	// After Effects template project file.
	  "ffx",	// After Effects filter preset file.
	  "inx",	// InDesign interchange, an XML file.
	  "inds",	// InDesign snippet, an XML file.
	  "inpk",	// InDesign package for GoLive, a text file (not XML).
	  "incd",	// InCopy story, an XML file.
	  "inct",	// InCopy template, an XML file.
	  "incx",	// InCopy interchange, an XML file.
	  "fm",		// FrameMaker file, proprietary format.
	  "book",	// FrameMaker book, proprietary format.
	  0 };		// ! Keep a 0 sentinel at the end.

// =================================================================================================

// =================================================================================================

// =================================================================================================
// LFA implementations for Macintosh
// =================================

#if XMP_MacBuild

	// ---------------------------------------------------------------------------------------------

	// ! Can't use Apple's 64 bit POSIX functions because frigging MSL has type clashes.

	LFA_FileRef LFA_Open ( const char * fileName, char mode )
	{
		XMP_Assert ( (mode == 'r') || (mode == 'w') );
		
		FSRef fileRef;
		SInt8 perm = ( (mode == 'r') ? fsRdPerm : fsRdWrPerm );
		HFSUniStr255 dataForkName;
		SInt16 refNum;
		
		OSErr err = FSGetDataForkName ( &dataForkName );
		if ( err != noErr ) XMP_Throw ( "LFA_Open: FSGetDataForkName failure", kXMPErr_ExternalFailure );
		
		err = FSPathMakeRef ( (XMP_Uns8*)fileName, &fileRef, 0 );
		if ( err != noErr ) XMP_Throw ( "LFA_Open: FSPathMakeRef failure", kXMPErr_ExternalFailure );
		
		err = FSOpenFork ( &fileRef, dataForkName.length, dataForkName.unicode, perm, &refNum );
		if ( err != noErr ) XMP_Throw ( "LFA_Open: FSOpenFork failure", kXMPErr_ExternalFailure );
		
		return (LFA_FileRef)refNum;

	}	// LFA_Open

	// ---------------------------------------------------------------------------------------------

	LFA_FileRef LFA_Create ( const char * fileName )
	{
		// *** Hack: Use fopen to avoid parent/child name separation needed by FSCreateFileUnicode.

		FILE * temp;
		temp = fopen ( fileName, "r" );	// Make sure the file does not exist.
		if ( temp != 0 ) {
			fclose ( temp );
			XMP_Throw ( "LFA_Create: file already exists", kXMPErr_ExternalFailure );
		}

		temp = fopen ( fileName, "w" );
		if ( temp == 0 ) XMP_Throw ( "LFA_Create: fopen failure", kXMPErr_ExternalFailure );
		fclose ( temp );
		
		return LFA_Open ( fileName, 'w' );

	}	// LFA_Create

	// ---------------------------------------------------------------------------------------------

	void LFA_Delete ( const char * fileName )
	{
		int err = remove ( fileName );	// *** Better to use an FS function.
		if ( err != 0 ) XMP_Throw ( "LFA_Delete: remove failure", kXMPErr_ExternalFailure );
		
	}	// LFA_Delete

	// ---------------------------------------------------------------------------------------------

	void LFA_Rename ( const char * oldName, const char * newName )
	{
		int err = rename ( oldName, newName );	// *** Better to use an FS function.
		if ( err != 0 ) XMP_Throw ( "LFA_Rename: rename failure", kXMPErr_ExternalFailure );
		
	}	// LFA_Rename

	// ---------------------------------------------------------------------------------------------

	LFA_FileRef LFA_OpenRsrc ( const char * fileName, char mode )
	{
		XMP_Assert ( (mode == 'r') || (mode == 'w') );
		
		FSRef fileRef;
		SInt8 perm = ( (mode == 'r') ? fsRdPerm : fsRdWrPerm );
		HFSUniStr255 rsrcForkName;
		SInt16 refNum;
		
		OSErr err = FSGetResourceForkName ( &rsrcForkName );
		if ( err != noErr ) XMP_Throw ( "LFA_Open: FSGetResourceForkName failure", kXMPErr_ExternalFailure );
		
		err = FSPathMakeRef ( (XMP_Uns8*)fileName, &fileRef, 0 );
		if ( err != noErr ) XMP_Throw ( "LFA_Open: FSPathMakeRef failure", kXMPErr_ExternalFailure );
		
		err = FSOpenFork ( &fileRef, rsrcForkName.length, rsrcForkName.unicode, perm, &refNum );
		if ( err != noErr ) XMP_Throw ( "LFA_Open: FSOpenFork failure", kXMPErr_ExternalFailure );
		
		return (LFA_FileRef)refNum;

	}	// LFA_OpenRsrc

	// ---------------------------------------------------------------------------------------------

	void LFA_Close ( LFA_FileRef file )
	{
		if ( file == 0 ) return;	// Can happen if LFA_Open throws an exception.
		long refNum = (long)file;	// ! Use long to avoid size warnings for SInt16 cast.

		OSErr err = FSCloseFork ( refNum );
		if ( err != noErr ) XMP_Throw ( "LFA_Close: FSCloseFork failure", kXMPErr_ExternalFailure );

	}	// LFA_Close

	// ---------------------------------------------------------------------------------------------

	XMP_Int64 LFA_Seek ( LFA_FileRef file, XMP_Int64 offset, int mode, bool * okPtr )
	{
		long refNum = (long)file;	// ! Use long to avoid size warnings for SInt16 cast.

		UInt16 posMode;
		switch ( mode ) {
			case SEEK_SET :
				posMode = fsFromStart;
				break;
			case SEEK_CUR :
				posMode = fsFromMark;
				break;
			case SEEK_END :
				posMode = fsFromLEOF;
				break;
			default :
				XMP_Throw ( "Invalid seek mode", kXMPErr_InternalFailure );
				break;
		}

		OSErr err;
		XMP_Int64 newPos;

		err = FSSetForkPosition ( refNum, posMode, offset );

		if ( err == eofErr ) {
			// FSSetForkPosition does not implicitly grow the file. Grow then seek to the new EOF.
			err = FSSetForkSize ( refNum, posMode, offset );
			if ( err == noErr ) err = FSSetForkPosition ( refNum, fsFromLEOF, 0 );
		}

		if ( err == noErr ) err = FSGetForkPosition ( refNum, &newPos );

		if ( okPtr != 0 ) {
			*okPtr = (err == noErr);
		} else {
			if ( err != noErr ) XMP_Throw ( "LFA_Seek: FSSetForkPosition failure", kXMPErr_ExternalFailure );
		}
		
		return newPos;

	}	// LFA_Seek

	// ---------------------------------------------------------------------------------------------

	XMP_Int32 LFA_Read ( LFA_FileRef file, void * buffer, XMP_Int32 bytes, bool requireAll )
	{
		long refNum = (long)file;	// ! Use long to avoid size warnings for SInt16 cast.
		ByteCount bytesRead;
		
		OSErr err = FSReadFork ( refNum, fsAtMark, 0, bytes, buffer, &bytesRead );
		if ( ((err != noErr) && (err != eofErr)) || (requireAll && (bytesRead != (ByteCount)bytes)) ) {
			// ! FSReadFork returns eofErr for a normal encounter with the end of file.
			XMP_Throw ( "LFA_Read: FSReadFork failure", kXMPErr_ExternalFailure );
		}
		
		return bytesRead;
		
	}	// LFA_Read

	// ---------------------------------------------------------------------------------------------

	void LFA_Write ( LFA_FileRef file, const void * buffer, XMP_Int32 bytes )
	{
		long refNum = (long)file;	// ! Use long to avoid size warnings for SInt16 cast.
		ByteCount bytesWritten;
		
		OSErr err = FSWriteFork ( refNum, fsAtMark, 0, bytes, buffer, &bytesWritten );
		if ( (err != noErr) | (bytesWritten != (ByteCount)bytes) ) XMP_Throw ( "LFA_Write: FSWriteFork failure", kXMPErr_ExternalFailure );

	}	// LFA_Write

	// ---------------------------------------------------------------------------------------------

	void LFA_Flush ( LFA_FileRef file )
	{
		long refNum = (long)file;	// ! Use long to avoid size warnings for SInt16 cast.

		OSErr err = FSFlushFork ( refNum );
		if ( err != noErr ) XMP_Throw ( "LFA_Flush: FSFlushFork failure", kXMPErr_ExternalFailure );

	}	// LFA_Flush

	// ---------------------------------------------------------------------------------------------

	XMP_Int64 LFA_Measure ( LFA_FileRef file )
	{
		long refNum = (long)file;	// ! Use long to avoid size warnings for SInt16 cast.
		XMP_Int64 length;
		
		OSErr err = FSGetForkSize ( refNum, &length );
		if ( err != noErr ) XMP_Throw ( "LFA_Measure: FSSetForkSize failure", kXMPErr_ExternalFailure );
		
		return length;
		
	}	// LFA_Measure

	// ---------------------------------------------------------------------------------------------

	void LFA_Extend ( LFA_FileRef file, XMP_Int64 length )
	{
		long refNum = (long)file;	// ! Use long to avoid size warnings for SInt16 cast.
		
		OSErr err = FSSetForkSize ( refNum, fsFromStart, length );
		if ( err != noErr ) XMP_Throw ( "LFA_Extend: FSSetForkSize failure", kXMPErr_ExternalFailure );
		
	}	// LFA_Extend

	// ---------------------------------------------------------------------------------------------

	void LFA_Truncate ( LFA_FileRef file, XMP_Int64 length )
	{
		long refNum = (long)file;	// ! Use long to avoid size warnings for SInt16 cast.
		
		OSErr err = FSSetForkSize ( refNum, fsFromStart, length );
		if ( err != noErr ) XMP_Throw ( "LFA_Truncate: FSSetForkSize failure", kXMPErr_ExternalFailure );
		
	}	// LFA_Truncate

	// ---------------------------------------------------------------------------------------------

#endif	// XMP_MacBuild

// =================================================================================================
// LFA implementations for Windows
// ===============================

#if XMP_WinBuild

	// ---------------------------------------------------------------------------------------------

	LFA_FileRef LFA_Open ( const char * fileName, char mode )
	{
		XMP_Assert ( (mode == 'r') || (mode == 'w') );
		
		DWORD access = GENERIC_READ;	// Assume read mode.
		DWORD share  = FILE_SHARE_READ;
		
		if ( mode == 'w' ) {
			access |= GENERIC_WRITE;
			share  = 0;
		}

		std::string wideName;
		const size_t utf8Len = strlen(fileName);
		const size_t maxLen = 2 * (utf8Len+1);

		wideName.reserve ( maxLen );
		wideName.assign ( maxLen, ' ' );
		int wideLen = MultiByteToWideChar ( CP_UTF8, 0, fileName, -1, (LPWSTR)wideName.data(), maxLen );
		if ( wideLen == 0 ) XMP_Throw ( "LFA_Open: MultiByteToWideChar failure", kXMPErr_ExternalFailure );

		HANDLE fileHandle = CreateFileW ( (LPCWSTR)wideName.data(), access, share, 0, OPEN_EXISTING,
										  (FILE_ATTRIBUTE_NORMAL | FILE_FLAG_RANDOM_ACCESS), 0 );
		if ( fileHandle == INVALID_HANDLE_VALUE ) XMP_Throw ( "LFA_Open: CreateFileW failure", kXMPErr_ExternalFailure );
		
		return (LFA_FileRef)fileHandle;

	}	// LFA_Open

	// ---------------------------------------------------------------------------------------------

	LFA_FileRef LFA_Create ( const char * fileName )
	{
		std::string wideName;
		const size_t utf8Len = strlen(fileName);
		const size_t maxLen = 2 * (utf8Len+1);

		wideName.reserve ( maxLen );
		wideName.assign ( maxLen, ' ' );
		int wideLen = MultiByteToWideChar ( CP_UTF8, 0, fileName, -1, (LPWSTR)wideName.data(), maxLen );
		if ( wideLen == 0 ) XMP_Throw ( "LFA_Create: MultiByteToWideChar failure", kXMPErr_ExternalFailure );

		HANDLE fileHandle;
		
		fileHandle = CreateFileW ( (LPCWSTR)wideName.data(), GENERIC_READ, FILE_SHARE_READ, 0, OPEN_EXISTING,
								   (FILE_ATTRIBUTE_NORMAL | FILE_FLAG_RANDOM_ACCESS), 0 );
		if ( fileHandle != INVALID_HANDLE_VALUE ) {
			CloseHandle ( fileHandle );
			XMP_Throw ( "LFA_Create: file already exists", kXMPErr_ExternalFailure );
		}

		fileHandle = CreateFileW ( (LPCWSTR)wideName.data(), (GENERIC_READ | GENERIC_WRITE), 0, 0, CREATE_ALWAYS,
								   (FILE_ATTRIBUTE_NORMAL | FILE_FLAG_RANDOM_ACCESS), 0 );
		if ( fileHandle == INVALID_HANDLE_VALUE ) XMP_Throw ( "LFA_Create: CreateFileW failure", kXMPErr_ExternalFailure );
		
		return (LFA_FileRef)fileHandle;

	}	// LFA_Create

	// ---------------------------------------------------------------------------------------------

	void LFA_Delete ( const char * fileName )
	{
		std::string wideName;
		const size_t utf8Len = strlen(fileName);
		const size_t maxLen = 2 * (utf8Len+1);

		wideName.reserve ( maxLen );
		wideName.assign ( maxLen, ' ' );
		int wideLen = MultiByteToWideChar ( CP_UTF8, 0, fileName, -1, (LPWSTR)wideName.data(), maxLen );
		if ( wideLen == 0 ) XMP_Throw ( "LFA_Delete: MultiByteToWideChar failure", kXMPErr_ExternalFailure );

		BOOL ok = DeleteFileW ( (LPCWSTR)wideName.data() );
		if ( ! ok ) XMP_Throw ( "LFA_Delete: DeleteFileW failure", kXMPErr_ExternalFailure );

	}	// LFA_Delete

	// ---------------------------------------------------------------------------------------------

	void LFA_Rename ( const char * oldName, const char * newName )
	{
		std::string wideOldName, wideNewName;
		size_t utf8Len = strlen(oldName);
		if ( utf8Len < strlen(newName) ) utf8Len = strlen(newName);
		const size_t maxLen = 2 * (utf8Len+1);
		int wideLen;

		wideOldName.reserve ( maxLen );
		wideOldName.assign ( maxLen, ' ' );
		wideLen = MultiByteToWideChar ( CP_UTF8, 0, oldName, -1, (LPWSTR)wideOldName.data(), maxLen );
		if ( wideLen == 0 ) XMP_Throw ( "LFA_Rename: MultiByteToWideChar failure", kXMPErr_ExternalFailure );

		wideNewName.reserve ( maxLen );
		wideNewName.assign ( maxLen, ' ' );
		wideLen = MultiByteToWideChar ( CP_UTF8, 0, newName, -1, (LPWSTR)wideNewName.data(), maxLen );
		if ( wideLen == 0 ) XMP_Throw ( "LFA_Rename: MultiByteToWideChar failure", kXMPErr_ExternalFailure );
		
		BOOL ok = MoveFileW ( (LPCWSTR)wideOldName.data(), (LPCWSTR)wideNewName.data() );
		if ( ! ok ) XMP_Throw ( "LFA_Rename: MoveFileW failure", kXMPErr_ExternalFailure );

	}	// LFA_Rename

	// ---------------------------------------------------------------------------------------------

	void LFA_Close ( LFA_FileRef file )
	{
		if ( file == 0 ) return;	// Can happen if LFA_Open throws an exception.
		HANDLE fileHandle = (HANDLE)file;

		BOOL ok = CloseHandle ( fileHandle );
		if ( ! ok ) XMP_Throw ( "LFA_Close: CloseHandle failure", kXMPErr_ExternalFailure );

	}	// LFA_Close

	// ---------------------------------------------------------------------------------------------

	XMP_Int64 LFA_Seek ( LFA_FileRef file, XMP_Int64 offset, int mode, bool * okPtr )
	{
		HANDLE fileHandle = (HANDLE)file;

		DWORD method;
		switch ( mode ) {
			case SEEK_SET :
				method = FILE_BEGIN;
				break;
			case SEEK_CUR :
				method = FILE_CURRENT;
				break;
			case SEEK_END :
				method = FILE_END;
				break;
			default :
				XMP_Throw ( "Invalid seek mode", kXMPErr_InternalFailure );
				break;
		}

		LARGE_INTEGER seekOffset, newPos;
		seekOffset.QuadPart = offset;
		
		BOOL ok = SetFilePointerEx ( fileHandle, seekOffset, &newPos, method );
		if ( okPtr != 0 ) {
			*okPtr = ok;
		} else {
			if ( ! ok ) XMP_Throw ( "LFA_Seek: SetFilePointerEx failure", kXMPErr_ExternalFailure );
		}
		
		return newPos.QuadPart;

	}	// LFA_Seek

	// ---------------------------------------------------------------------------------------------

	XMP_Int32 LFA_Read ( LFA_FileRef file, void * buffer, XMP_Int32 bytes, bool requireAll )
	{
		HANDLE fileHandle = (HANDLE)file;
		DWORD  bytesRead;
		
		BOOL ok = ReadFile ( fileHandle, buffer, bytes, &bytesRead, 0 );
		if ( (! ok) || (requireAll && (bytesRead != bytes)) ) XMP_Throw ( "LFA_Read: ReadFile failure", kXMPErr_ExternalFailure );
		
		return bytesRead;

	}	// LFA_Read

	// ---------------------------------------------------------------------------------------------

	void LFA_Write ( LFA_FileRef file, const void * buffer, XMP_Int32 bytes )
	{
		HANDLE fileHandle = (HANDLE)file;
		DWORD  bytesWritten;
		
		BOOL ok = WriteFile ( fileHandle, buffer, bytes, &bytesWritten, 0 );
		if ( (! ok) || (bytesWritten != bytes) ) XMP_Throw ( "LFA_Write: WriteFile failure", kXMPErr_ExternalFailure );

	}	// LFA_Write

	// ---------------------------------------------------------------------------------------------

	void LFA_Flush ( LFA_FileRef file )
	{
		HANDLE fileHandle = (HANDLE)file;

		BOOL ok = FlushFileBuffers ( fileHandle );
		if ( ! ok ) XMP_Throw ( "LFA_Flush: FlushFileBuffers failure", kXMPErr_ExternalFailure );

	}	// LFA_Flush

	// ---------------------------------------------------------------------------------------------

	XMP_Int64 LFA_Measure ( LFA_FileRef file )
	{
		HANDLE fileHandle = (HANDLE)file;
		LARGE_INTEGER length;
		
		BOOL ok = GetFileSizeEx ( fileHandle, &length );
		if ( ! ok ) XMP_Throw ( "LFA_Measure: GetFileSizeEx failure", kXMPErr_ExternalFailure );
		
		return length.QuadPart;
		
	}	// LFA_Measure

	// ---------------------------------------------------------------------------------------------

	void LFA_Extend ( LFA_FileRef file, XMP_Int64 length )
	{
		HANDLE fileHandle = (HANDLE)file;

		LARGE_INTEGER winLength;
		winLength.QuadPart = length;

		BOOL ok = SetFilePointerEx ( fileHandle, winLength, 0, FILE_BEGIN );
		if ( ! ok ) XMP_Throw ( "LFA_Extend: SetFilePointerEx failure", kXMPErr_ExternalFailure );
		ok = SetEndOfFile ( fileHandle );
		if ( ! ok ) XMP_Throw ( "LFA_Extend: SetEndOfFile failure", kXMPErr_ExternalFailure );

	}	// LFA_Extend

	// ---------------------------------------------------------------------------------------------

	void LFA_Truncate ( LFA_FileRef file, XMP_Int64 length )
	{
		HANDLE fileHandle = (HANDLE)file;

		LARGE_INTEGER winLength;
		winLength.QuadPart = length;

		BOOL ok = SetFilePointerEx ( fileHandle, winLength, 0, FILE_BEGIN );
		if ( ! ok ) XMP_Throw ( "LFA_Truncate: SetFilePointerEx failure", kXMPErr_ExternalFailure );
		ok = SetEndOfFile ( fileHandle );
		if ( ! ok ) XMP_Throw ( "LFA_Truncate: SetEndOfFile failure", kXMPErr_ExternalFailure );

	}	// LFA_Truncate

	// ---------------------------------------------------------------------------------------------

#endif	// XMP_WinBuild

// =================================================================================================
// LFA implementations for POSIX
// =============================

#if XMP_UNIXBuild

	// ---------------------------------------------------------------------------------------------

	// Make sure off_t is 64 bits and signed.
	static char check_off_t_size [ (sizeof(off_t) == 8) ? 1 : -1 ];
	// *** No std::numeric_limits?  static char check_off_t_sign [ std::numeric_limits<off_t>::is_signed ? -1 : 1 ];

	// ---------------------------------------------------------------------------------------------

	LFA_FileRef LFA_Open ( const char * fileName, char mode )
	{
		XMP_Assert ( (mode == 'r') || (mode == 'w') );
		
		int flags = ((mode == 'r') ? O_RDONLY : O_RDWR);	// *** Include O_EXLOCK?
		
		int descr = open ( fileName, flags, 0 );
		if ( descr == -1 ) XMP_Throw ( "LFA_Open: open failure", kXMPErr_ExternalFailure );
		
		return (LFA_FileRef)descr;

	}	// LFA_Open

	// ---------------------------------------------------------------------------------------------

	LFA_FileRef LFA_Create ( const char * fileName )
	{
		int descr;
		
		descr = open ( fileName, O_RDONLY, 0 );	// Make sure the file does not exist yet.
		if ( descr != -1 ) {
			close ( descr );
			XMP_Throw ( "LFA_Create: file already exists", kXMPErr_ExternalFailure );
		}

		descr = open ( fileName, (O_CREAT | O_RDWR), 0 );	// *** Include O_EXCL? O_EXLOCK?
		if ( descr == -1 ) XMP_Throw ( "LFA_Create: open failure", kXMPErr_ExternalFailure );
		
		return (LFA_FileRef)descr;

	}	// LFA_Create

	// ---------------------------------------------------------------------------------------------

	void LFA_Delete ( const char * fileName )
	{
		int err = unlink ( fileName );
		if ( err != 0 ) XMP_Throw ( "LFA_Delete: unlink failure", kXMPErr_ExternalFailure );
		
	}	// LFA_Delete

	// ---------------------------------------------------------------------------------------------

	void LFA_Rename ( const char * oldName, const char * newName )
	{
		int err = rename ( oldName, newName );	// *** POSIX rename clobbers existing destination!
		if ( err != 0 ) XMP_Throw ( "LFA_Rename: rename failure", kXMPErr_ExternalFailure );
		
	}	// LFA_Rename

	// ---------------------------------------------------------------------------------------------

	void LFA_Close ( LFA_FileRef file )
	{
		if ( file == 0 ) return;	// Can happen if LFA_Open throws an exception.
		int descr = (int)file;

		int err = close ( descr );
		if ( err != 0 ) XMP_Throw ( "LFA_Close: close failure", kXMPErr_ExternalFailure );

	}	// LFA_Close

	// ---------------------------------------------------------------------------------------------

	XMP_Int64 LFA_Seek ( LFA_FileRef file, XMP_Int64 offset, int mode, bool * okPtr )
	{
		int descr = (int)file;
		
		off_t newPos = lseek ( descr, offset, mode );
		if ( okPtr != 0 ) {
			*okPtr = (newPos != -1);
		} else {
			if ( newPos == -1 ) XMP_Throw ( "LFA_Seek: lseek failure", kXMPErr_ExternalFailure );
		}
		
		return newPos;

	}	// LFA_Seek

	// ---------------------------------------------------------------------------------------------

	XMP_Int32 LFA_Read ( LFA_FileRef file, void * buffer, XMP_Int32 bytes, bool requireAll )
	{
		int descr = (int)file;
		
		ssize_t bytesRead = read ( descr, buffer, bytes );
		if ( (bytesRead == -1) || (requireAll && (bytesRead != bytes)) ) XMP_Throw ( "LFA_Read: read failure", kXMPErr_ExternalFailure );
		
		return bytesRead;

	}	// LFA_Read

	// ---------------------------------------------------------------------------------------------

	void LFA_Write ( LFA_FileRef file, const void * buffer, XMP_Int32 bytes )
	{
		int descr = (int)file;

		ssize_t bytesWritten = write ( descr, buffer, bytes );
		if ( bytesWritten != bytes ) XMP_Throw ( "LFA_Write: write failure", kXMPErr_ExternalFailure );

	}	// LFA_Write

	// ---------------------------------------------------------------------------------------------

	void LFA_Flush ( LFA_FileRef file )
	{
		int descr = (int)file;

		int err = fsync ( descr );
		if ( err != 0 ) XMP_Throw ( "LFA_Flush: fsync failure", kXMPErr_ExternalFailure );

	}	// LFA_Flush

	// ---------------------------------------------------------------------------------------------

	XMP_Int64 LFA_Measure ( LFA_FileRef file )
	{
		int descr = (int)file;
		
		off_t currPos = lseek ( descr, 0, SEEK_CUR );
		off_t length  = lseek ( descr, 0, SEEK_END );
		if ( (currPos == -1) || (length == -1) ) XMP_Throw ( "LFA_Measure: lseek failure", kXMPErr_ExternalFailure );
		(void) lseek ( descr, currPos, SEEK_SET );
		
		return length;
		
	}	// LFA_Measure

	// ---------------------------------------------------------------------------------------------

	void LFA_Extend ( LFA_FileRef file, XMP_Int64 length )
	{
		int descr = (int)file;
		
		int err = ftruncate ( descr, length );
		if ( err != 0 ) XMP_Throw ( "LFA_Extend: ftruncate failure", kXMPErr_ExternalFailure );

	}	// LFA_Extend

	// ---------------------------------------------------------------------------------------------

	void LFA_Truncate ( LFA_FileRef file, XMP_Int64 length )
	{
		int descr = (int)file;
		
		int err = ftruncate ( descr, length );
		if ( err != 0 ) XMP_Throw ( "LFA_Truncate: ftruncate failure", kXMPErr_ExternalFailure );

	}	// LFA_Truncate

	// ---------------------------------------------------------------------------------------------

#endif	// XMP_UNIXBuild

// =================================================================================================

void LFA_Copy ( LFA_FileRef sourceFile, LFA_FileRef destFile, XMP_Int64 length,
                XMP_AbortProc abortProc /* = 0 */, void * abortArg /* = 0 */ )
{
	enum { kBufferLen = 64*1024 };
	XMP_Uns8 buffer [kBufferLen];
	
	const bool checkAbort = (abortProc != 0);
	
	while ( length > 0 ) {
		if ( checkAbort && abortProc(abortArg) ) {
			XMP_Throw ( "LFA_Copy - User abort", kXMPErr_UserAbort );
		}
		XMP_Int32 ioCount = kBufferLen;
		if ( length < kBufferLen ) ioCount = (XMP_Int32)length;
		LFA_Read ( sourceFile, buffer, ioCount, kLFA_RequireAll );
		LFA_Write ( destFile, buffer, ioCount );
		length -= ioCount;
	}

}	// LFA_Copy

// =================================================================================================

static bool CreateNewFile ( const char * newPath, const char * origPath, size_t filePos, bool copyMacRsrc )
{
	// Try to create a new file with the same ownership and permissions as some other file.
	// *** The ownership and permissions are not handled on all platforms.

	#if XMP_MacBuild | XMP_UNIXBuild
	{
		FILE * temp = fopen ( newPath, "r" );	// Make sure the file does not exist.
		if ( temp != 0 ) {
			fclose ( temp );
			return false;
		}
	}
	#elif XMP_WinBuild
	{
		std::string wideName;
		const size_t utf8Len = strlen(newPath);
		const size_t maxLen = 2 * (utf8Len+1);
		wideName.reserve ( maxLen );
		wideName.assign ( maxLen, ' ' );
		int wideLen = MultiByteToWideChar ( CP_UTF8, 0, newPath, -1, (LPWSTR)wideName.data(), maxLen );
		if ( wideLen == 0 ) return false;
		HANDLE temp = CreateFileW ( (LPCWSTR)wideName.data(), GENERIC_READ, FILE_SHARE_READ, 0, OPEN_EXISTING,
								    (FILE_ATTRIBUTE_NORMAL | FILE_FLAG_RANDOM_ACCESS), 0 );
		if ( temp != INVALID_HANDLE_VALUE ) {
			CloseHandle ( temp );
			return false;
		}
	}
	#endif
	
	try {
		LFA_FileRef newFile = LFA_Create ( newPath );
		LFA_Close ( newFile );
	} catch ( ... ) {
		// *** Unfortunate that LFA_Create throws for an existing file.
		return false;
	}

	#if XMP_WinBuild

		IgnoreParam(origPath); IgnoreParam(filePos); IgnoreParam(copyMacRsrc);

		// *** Don't handle Windows specific info yet.

	#elif XMP_MacBuild

		IgnoreParam(filePos);

		OSStatus err;
		FSRef newFSRef, origFSRef;	// Copy the "copyable" catalog info, includes the Finder info.
		
		err = FSPathMakeRef ( (XMP_Uns8*)origPath, &origFSRef, 0 );
		if ( err != noErr ) XMP_Throw ( "CreateNewFile: FSPathMakeRef failure", kXMPErr_ExternalFailure );
		err = FSPathMakeRef ( (XMP_Uns8*)newPath, &newFSRef, 0 );
		if ( err != noErr ) XMP_Throw ( "CreateNewFile: FSPathMakeRef failure", kXMPErr_ExternalFailure );

		FSCatalogInfo catInfo;	// *** What about the GetInfo comment? The Finder label?
		memset ( &catInfo, 0, sizeof(FSCatalogInfo) );

		err = FSGetCatalogInfo ( &origFSRef, kFSCatInfoGettableInfo, &catInfo, 0, 0, 0 );
		if ( err != noErr ) XMP_Throw ( "CreateNewFile: FSGetCatalogInfo failure", kXMPErr_ExternalFailure );
		err = FSSetCatalogInfo ( &newFSRef, kFSCatInfoSettableInfo, &catInfo );
		if ( err != noErr ) XMP_Throw ( "CreateNewFile: FSSetCatalogInfo failure", kXMPErr_ExternalFailure );
		
		if ( copyMacRsrc ) {	// Copy the resource fork as a byte stream.
			LFA_FileRef origRsrcRef = 0;
			LFA_FileRef copyRsrcRef = 0;
			try {
				origRsrcRef = LFA_OpenRsrc ( origPath, 'r' );
				XMP_Int64 rsrcSize = LFA_Measure ( origRsrcRef );
				if ( rsrcSize > 0 ) {
					copyRsrcRef = LFA_OpenRsrc ( newPath, 'w' );
					LFA_Copy ( origRsrcRef, copyRsrcRef, rsrcSize, 0, 0 );	// ! Resource fork small enough to not need abort checking.
					LFA_Close ( copyRsrcRef );
				}
				LFA_Close ( origRsrcRef );
			} catch ( ... ) {
				if ( origRsrcRef != 0 ) LFA_Close ( origRsrcRef );
				if ( copyRsrcRef != 0 ) LFA_Close ( copyRsrcRef );
				throw;
			}
		}
		

	#elif XMP_UNIXBuild
	
		IgnoreParam(filePos); IgnoreParam(copyMacRsrc);
		// *** Can't use on Mac because of frigging CW POSIX header problems!
		
		int err, newRef;
		struct stat origInfo;
		err = stat ( origPath, &origInfo );
		if ( err != 0 ) XMP_Throw ( "CreateNewFile: stat failure", kXMPErr_ExternalFailure );

		newRef = open ( newPath, (O_CREAT | O_EXCL | O_RDWR), origInfo.st_mode );
		if ( newRef != -1 ) {
			(void) fchown ( newRef, origInfo.st_uid, origInfo.st_gid );	// Tolerate failure.
			close ( newRef );
		}
	
	#endif
	
	return true;

}	// CreateNewFile

// =================================================================================================

void CreateTempFile ( const std::string & origPath, std::string * tempPath, bool copyMacRsrc )
{
	// Create an empty temp file next to the source file with the same ownership and permissions.
	// The temp file has "._nn_" added as a prefix to the file name, where "nn" is a unique
	// sequence number. The "._" start is important for Bridge, telling it to ignore the file.
	
	// *** The ownership and permissions are not yet handled.
	
	#if XMP_WinBuild
		#define kUseBS	true
	#else
		#define kUseBS	false
	#endif

	// Break the full path into folder path and file name portions.

	size_t namePos;	// The origPath index of the first byte of the file name part.

	for ( namePos = origPath.size(); namePos > 0; --namePos ) {
		if ( (origPath[namePos] == '/') || (kUseBS && (origPath[namePos] == '\\')) ) {
			++namePos;
			break;
		}
	}
	if ( (origPath[namePos] == '/') || (kUseBS && (origPath[namePos] == '\\')) ) ++namePos;
	if ( namePos == origPath.size() ) XMP_Throw ( "CreateTempFile: Empty file name part", kXMPErr_InternalFailure );
	
	std::string folderPath ( origPath, 0, namePos );
	std::string origName ( origPath, namePos );
	
	// First try to create a file with "._nn_" added as a file name prefix.

	char tempPrefix[6] = "._nn_";
	
	tempPath->reserve ( origPath.size() + 5 );
	tempPath->assign ( origPath, 0, namePos );
	tempPath->append ( tempPrefix, 5 );
	tempPath->append ( origName );
	
	for ( char n1 = '0'; n1 <= '9'; ++n1 ) {
		(*tempPath)[namePos+2] = n1;
		for ( char n2 = '0'; n2 <= '9'; ++n2 ) {
			(*tempPath)[namePos+3] = n2;
			if ( CreateNewFile ( tempPath->c_str(), origPath.c_str(), namePos, copyMacRsrc ) ) return;
		}
	}
	
	// Now try to create a file with the name "._nn_XMPFilesTemp"
	
	tempPath->assign ( origPath, 0, namePos );
	tempPath->append ( tempPrefix, 5 );
	tempPath->append ( "XMPFilesTemp" );
	
	for ( char n1 = '0'; n1 <= '9'; ++n1 ) {
		(*tempPath)[namePos+2] = n1;
		for ( char n2 = '0'; n2 <= '9'; ++n2 ) {
			(*tempPath)[namePos+3] = n2;
			if ( CreateNewFile ( tempPath->c_str(), origPath.c_str(), namePos, copyMacRsrc ) ) return;
		}
	}
	
	XMP_Throw ( "CreateTempFile: Can't find unique name", kXMPErr_InternalFailure );
	
}	// CreateTempFile

// =================================================================================================
// GetPacketCharForm
// =================
//
// The first character must be U+FEFF or ASCII, typically '<' for an outermost element, initial
// processing instruction, or XML declaration. The second character can't be U+0000.
// The possible input sequences are:
//   Cases with U+FEFF
//      EF BB BF -- - UTF-8
//      FE FF -- -- - Big endian UTF-16
//      00 00 FE FF - Big endian UTF 32
//      FF FE 00 00 - Little endian UTF-32
//      FF FE -- -- - Little endian UTF-16
//   Cases with ASCII
//      nn mm -- -- - UTF-8 -
//      00 00 00 nn - Big endian UTF-32
//      00 nn -- -- - Big endian UTF-16
//      nn 00 00 00 - Little endian UTF-32
//      nn 00 -- -- - Little endian UTF-16

XMP_Uns8 GetPacketCharForm ( XMP_StringPtr packetStr, XMP_StringLen packetLen )
{
	XMP_Uns8   charForm = kXMP_CharUnknown;
	XMP_Uns8 * unsBytes = (XMP_Uns8*)packetStr;	// ! Make sure comparisons are unsigned.

	if ( packetLen < 4 ) XMP_Throw ( "Can't determine character encoding", kXMPErr_BadXMP );
	
	if ( unsBytes[0] == 0 ) {
	
		// These cases are:
		//   00 nn -- -- - Big endian UTF-16
		//   00 00 00 nn - Big endian UTF-32
		//   00 00 FE FF - Big endian UTF 32
		
		if ( unsBytes[1] != 0 ) {
			charForm = kXMP_Char16BitBig;			// 00 nn
		} else {
			if ( (unsBytes[2] == 0) && (unsBytes[3] != 0) ) {
				charForm = kXMP_Char32BitBig;		// 00 00 00 nn
			} else if ( (unsBytes[2] == 0xFE) && (unsBytes[3] == 0xFF) ) {
				charForm = kXMP_Char32BitBig;		// 00 00 FE FF
			}
		}
		
	} else {
	
		// These cases are:
		//   FE FF -- -- - Big endian UTF-16, FE isn't valid UTF-8
		//   FF FE 00 00 - Little endian UTF-32, FF isn't valid UTF-8
		//   FF FE -- -- - Little endian UTF-16
		//   nn mm -- -- - UTF-8, includes EF BB BF case
		//   nn 00 00 00 - Little endian UTF-32
		//   nn 00 -- -- - Little endian UTF-16
		
		if ( unsBytes[0] == 0xFE ) {
			if ( unsBytes[1] == 0xFF ) charForm = kXMP_Char16BitBig;	// FE FF
		} else if ( unsBytes[0] == 0xFF ) {
			if ( unsBytes[1] == 0xFE ) {
				if ( (unsBytes[2] == 0) && (unsBytes[3] == 0) ) {
					charForm = kXMP_Char32BitLittle;	// FF FE 00 00
				} else {
					charForm = kXMP_Char16BitLittle;	// FF FE
				}
			}
		} else if ( unsBytes[1] != 0 ) {
			charForm = kXMP_Char8Bit;					// nn mm
		} else {
			if ( (unsBytes[2] == 0) && (unsBytes[3] == 0) ) {
				charForm = kXMP_Char32BitLittle;		// nn 00 00 00
			} else {
				charForm = kXMP_Char16BitLittle;		// nn 00
			}
		}

	}
	
	if ( charForm == kXMP_CharUnknown ) XMP_Throw ( "Unknown character encoding", kXMPErr_BadXMP );
	return charForm;

}	// GetPacketCharForm

// =================================================================================================
// GetPacketRWMode
// ===============

bool GetPacketRWMode ( XMP_StringPtr packetStr, XMP_StringLen packetLen, size_t charSize )
{
	bool isWriteable = false;
	XMP_StringPtr packetLim = packetStr + packetLen;
	XMP_StringPtr rwPtr = packetStr + packetLen - 1;

	for ( ; rwPtr >= packetStr; --rwPtr ) if ( *rwPtr == '<' ) break;	// Byte at a time, don't know endianness.
	if ( *rwPtr != '<' ) XMP_Throw ( "Bad packet trailer", kXMPErr_BadXMP );

	for ( ; rwPtr < packetLim; rwPtr += charSize ) if ( *rwPtr == '=' ) break;
	if ( *rwPtr != '=' ) XMP_Throw ( "Bad packet trailer", kXMPErr_BadXMP );

	rwPtr += (2 * charSize);
	if ( rwPtr >= packetLim ) XMP_Throw ( "Bad packet trailer", kXMPErr_BadXMP );

	if ( *rwPtr == 'r' ) {
		isWriteable = false;
	} else if ( *rwPtr == 'w' ) {
		isWriteable = true;
	} else {
		XMP_Throw ( "Bad packet trailer", kXMPErr_BadXMP );
	}
	
	return isWriteable;

}	// GetPacketRWMode

// =================================================================================================
// GetPacketPadSize
// ================

size_t GetPacketPadSize ( XMP_StringPtr packetStr, XMP_StringLen packetLen )
{
	XMP_Int32 padEnd = packetLen - 1;	// ! Must be signed.
	for ( ; padEnd > 0; --padEnd ) if ( packetStr[padEnd] == '<' ) break;
	if ( padEnd == 0 ) return 0;
	
	XMP_Int32 padStart = padEnd;	// ! Must be signed.
	for ( ; padStart > 0; --padStart ) if ( packetStr[padStart] == '>' ) break;
	if ( padStart == 0 ) return 0;
	++padStart;
	
	return (padEnd - padStart);
		
}	// GetPacketPadSize

// =================================================================================================
// ReadXMPPacket
// =============

void ReadXMPPacket ( XMPFileHandler * handler )
{
	LFA_FileRef   fileRef   = handler->parent->fileRef;
	std::string & xmpPacket = handler->xmpPacket;
	XMP_StringLen packetLen = handler->packetInfo.length;

	if ( packetLen == 0 ) XMP_Throw ( "ReadXMPPacket - No XMP packet", kXMPErr_BadXMP );
	
	xmpPacket.erase();
	xmpPacket.reserve ( packetLen );
	xmpPacket.append ( packetLen, ' ' );

	XMP_StringPtr packetStr = XMP_StringPtr ( xmpPacket.c_str() );	// Don't set until after reserving the space!

	LFA_Seek ( fileRef, handler->packetInfo.offset, SEEK_SET );
	LFA_Read ( fileRef, (char*)packetStr, packetLen, kLFA_RequireAll );
	
}	// ReadXMPPacket

// =================================================================================================
// XMPFileHandler::ProcessTNail
// ============================

void XMPFileHandler::ProcessTNail()
{

	this->processedTNail = true;	// ! Must be overridden by handlers that support thumbnails.

}	// XMPFileHandler::ProcessTNail

// =================================================================================================
// XMPFileHandler::ProcessXMP
// ==========================
//
// This base implementation just parses the XMP. If the derived handler does reconciliation then it
// must have its own implementation of ProcessXMP.

void XMPFileHandler::ProcessXMP()
{
	
	if ( (!this->containsXMP) || this->processedXMP ) return;

	if ( this->handlerFlags & kXMPFiles_CanReconcile ) {
		XMP_Throw ( "Reconciling file handlers must implement ProcessXMP", kXMPErr_InternalFailure );
	}

	SXMPUtils::RemoveProperties ( &this->xmpObj, 0, 0, kXMPUtil_DoAllProperties );
	this->xmpObj.ParseFromBuffer ( this->xmpPacket.c_str(), this->xmpPacket.size() );
	this->processedXMP = true;
	
}	// XMPFileHandler::ProcessXMP

// =================================================================================================

#if TrackMallocFree

	#undef malloc
	#undef free
	
	#define kTrackAddr 0x509DF6
	
	void* XMPFiles_Malloc ( size_t size )
	{
		void* addr = malloc ( size );
		if ( addr == (void*)kTrackAddr ) {
			printf ( "XMPFiles_Malloc: allocated %.8X\n", kTrackAddr );
		}
		return addr;
	}
	
	void  XMPFiles_Free ( void* addr )
	{
		if ( addr == (void*)kTrackAddr ) {
			printf ( "XMPFiles_Free: deallocating %.8X\n", kTrackAddr );
		}
		free ( addr );
	}

#endif

// =================================================================================================
