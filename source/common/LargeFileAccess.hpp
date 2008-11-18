#ifndef __LargeFileAccess_hpp__
#define __LargeFileAccess_hpp__	1

// =================================================================================================
// Copyright 2006 Adobe Systems Incorporated
// All Rights Reserved.
//
// NOTICE:  Adobe permits you to use, modify, and distribute this file in accordance with the terms
// of the Adobe license agreement accompanying it.
// =================================================================================================

#include <stdio.h>  // The assert macro needs printf.
#include "XMP_Environment.h"
#include "XMP_Const.h"
#include <string>

#include "EndianUtils.hpp"

#if XMP_UNIXBuild
typedef int LFA_FileRef;
#else
typedef void * LFA_FileRef;
#endif

enum { // used by LFA_Throw, re-route to whatever you need
	kLFAErr_InternalFailure = 1,
	kLFAErr_ExternalFailure = 2,
	kLFAErr_UserAbort = 3
	};

#define TXMP_STRING_TYPE std::string
#define XMP_INCLUDE_XMPFILES 1
#include "XMP.hpp"

// to use these routines, user must define LFA_Throw, kLFAErr_ExternalFailure, kLFAErr_UserAbort
// Note:LFA_Throw must be a function, not a macro (it may very well wrap a macro, i.e. 
//LFA_Throw(msg,id)	{ throw std::logic_error ( msg ); }

using namespace std;

#if XMP_WinBuild
	#include <Windows.h>
	#define snprintf _snprintf
#else
	#if XMP_MacBuild
		#include <Files.h>
	#endif
	// POSIX headers for both Mac and generic UNIX.
	#include <pthread.h>
	#include <fcntl.h>
	#include <unistd.h>
	#include <dirent.h>
	#include <sys/stat.h>
	#include <sys/types.h>
#endif

// *** Change the model of the LFA functions to not throw for "normal" open/create errors.
// *** Make sure the semantics of open/create/rename are consistent, e.g. about create of existing.

extern LFA_FileRef	LFA_Open     ( const char* fileName, char openMode ); // Mode is 'r' or 'w'.
extern LFA_FileRef	LFA_Create   ( const char* fileName );
extern void			LFA_Delete   ( const char* fileName );
extern void			LFA_Rename   ( const char* oldName, const char * newName );
extern void			LFA_Close    ( LFA_FileRef file );

// NOTE: unlike the fseek() 'original' LFA_Seek returns the new file position,
//       *NOT* 0 to indicate everything's fine
extern XMP_Int64	LFA_Seek     ( LFA_FileRef file, XMP_Int64 offset, int seekMode, bool* okPtr = 0 );
extern XMP_Int32	LFA_Read     ( LFA_FileRef file, void* buffer, XMP_Int32 bytes, bool requireAll = false );
extern void			LFA_Write    ( LFA_FileRef file, const void* buffer, XMP_Int32 bytes );
extern void			LFA_Flush    ( LFA_FileRef file );
extern XMP_Int64	LFA_Tell     ( LFA_FileRef file );
extern XMP_Int64	LFA_Measure  ( LFA_FileRef file );
extern void			LFA_Extend   ( LFA_FileRef file, XMP_Int64 length );
extern void			LFA_Truncate ( LFA_FileRef file, XMP_Int64 length );

extern void			LFA_Copy     ( LFA_FileRef sourceFile, LFA_FileRef destFile, XMP_Int64 length,	// Not a primitive.
					               XMP_AbortProc abortProc = 0, void * abortArg = 0 );

/* move stuff within a file (both upward and downward */
extern void			LFA_Move     ( LFA_FileRef srcFile, XMP_Int64 srcOffset, LFA_FileRef dstFile, XMP_Int64 dstOffset,
					               XMP_Int64 length, XMP_AbortProc abortProc = 0, void * abortArg = 0 );

extern bool			LFA_isEof	( LFA_FileRef file );
extern char			LFA_GetChar ( LFA_FileRef file );

enum { kLFA_RequireAll = true };	// Used for requireAll to LFA_Read.

// =================================================================================================
// Read and convert endianess in one go:

static inline XMP_Uns16 LFA_ReadUns16_BE ( LFA_FileRef file )
{
	XMP_Uns16 value;
	LFA_Read ( file, &value, 2, kLFA_RequireAll );
	return MakeUns16BE ( value );
}

static inline XMP_Uns16 LFA_ReadUns16_LE ( LFA_FileRef file )
{
	XMP_Uns16 value;
	LFA_Read ( file, &value, 2, kLFA_RequireAll );
	return MakeUns16LE ( value );
}

static inline XMP_Uns32 LFA_ReadUns32_BE ( LFA_FileRef file )
{
	XMP_Uns32 value;
	LFA_Read ( file, &value, 4, kLFA_RequireAll );
	return MakeUns32BE ( value );
}

static inline XMP_Uns32 LFA_ReadUns32_LE ( LFA_FileRef file )
{
	XMP_Uns32 value;
	LFA_Read ( file, &value, 4, kLFA_RequireAll );
	return MakeUns32LE ( value );
}

// new:
static inline XMP_Uns64 LFA_ReadUns64_BE ( LFA_FileRef file )
{
	XMP_Uns64 value;
	LFA_Read ( file, &value, 8, kLFA_RequireAll );
	return MakeUns64BE ( value );
}

static inline XMP_Uns64 LFA_ReadUns64_LE ( LFA_FileRef file )
{
	XMP_Uns64 value;
	LFA_Read ( file, &value, 8, kLFA_RequireAll );
	return MakeUns64LE ( value );
}

#define LFA_ReadInt16_BE(file) ((XMP_Int16) LFA_ReadUns16_BE ( file ))
#define LFA_ReadInt16_LE(file) ((XMP_Int16) LFA_ReadUns16_LE ( file ))
#define LFA_ReadInt32_BE(file) ((XMP_Int32) LFA_ReadUns32_BE ( file ))
#define LFA_ReadInt32_LE(file) ((XMP_Int32) LFA_ReadUns32_LE ( file ))
#define LFA_ReadInt64_BE(file) ((XMP_Int64) LFA_ReadUns64_BE ( file ))
#define LFA_ReadInt64_LE(file) ((XMP_Int64) LFA_ReadUns64_LE ( file ))


#endif	// __LargeFileAccess_hpp__
