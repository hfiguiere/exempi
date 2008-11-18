#ifndef __XMPFiles_Impl_hpp__
#define __XMPFiles_Impl_hpp__	1

// =================================================================================================
// ADOBE SYSTEMS INCORPORATED
// Copyright 2004-2008 Adobe Systems Incorporated
// All Rights Reserved
//
// NOTICE: Adobe permits you to use, modify, and distribute this file in accordance with the terms
// of the Adobe license agreement accompanying it.
// =================================================================================================

#include "XMP_Environment.h"	// ! Must be the first #include!
#include "XMP_Const.h"
#include "XMP_BuildInfo.h"
#include "EndianUtils.hpp"

#include <string>
#include <cstring>
#define TXMP_STRING_TYPE std::string
#define XMP_INCLUDE_XMPFILES 1
#include "XMP.hpp"

#include "XMPFiles.hpp"
#include "LargeFileAccess.hpp"

#include <vector>
#include <string>
#include <map>

#include <cassert>

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

// =================================================================================================
// General global variables and macros

extern long sXMPFilesInitCount;

#ifndef GatherPerformanceData
	#define GatherPerformanceData 0
#endif

#if ! GatherPerformanceData

	#define StartPerfCheck(proc,info)	/* do nothing */
	#define EndPerfCheck(proc)	/* do nothing */

#else

	#include "PerfUtils.hpp"

	enum {
		kAPIPerf_OpenFile,
		kAPIPerf_CloseFile,
		kAPIPerf_GetXMP,
		kAPIPerf_GetThumbnail,
		kAPIPerf_PutXMP,
		kAPIPerf_CanPutXMP,
		kAPIPerfProcCount	// Last, count of the procs.
	};
	
	static const char* kAPIPerfNames[] =
		{ "OpenFile", "CloseFile", "GetXMP", "GetThumbnail", "PutXMP", "CanPutXMP", 0 };
	
	struct APIPerfItem {
		XMP_Uns8    whichProc;
		double      elapsedTime;
		XMPFilesRef xmpFilesRef;
		std::string extraInfo;
		APIPerfItem ( XMP_Uns8 proc, double time, XMPFilesRef ref, const char * info )
			: whichProc(proc), elapsedTime(time), xmpFilesRef(ref), extraInfo(info) {};
	};
	
	typedef std::vector<APIPerfItem> APIPerfCollection;
	
	extern APIPerfCollection* sAPIPerf;

	#define StartPerfCheck(proc,info)	\
		sAPIPerf->push_back ( APIPerfItem ( proc, 0.0, xmpFilesRef, info ) );	\
		APIPerfItem & thisPerf = sAPIPerf->back();								\
		PerfUtils::MomentValue startTime, endTime;								\
		try {																	\
			startTime = PerfUtils::NoteThisMoment();

	#define EndPerfCheck(proc)	\
			endTime = PerfUtils::NoteThisMoment();										\
			thisPerf.elapsedTime = PerfUtils::GetElapsedSeconds ( startTime, endTime );	\
		} catch ( ... ) {																\
			endTime = PerfUtils::NoteThisMoment();										\
			thisPerf.elapsedTime = PerfUtils::GetElapsedSeconds ( startTime, endTime );	\
			thisPerf.extraInfo += "  ** THROW **";										\
			throw;																		\
		}

#endif

extern XMP_FileFormat voidFileFormat;	// Used as sink for unwanted output parameters.
extern XMP_PacketInfo voidPacketInfo;
extern void *         voidVoidPtr;
extern XMP_StringPtr  voidStringPtr;
extern XMP_StringLen  voidStringLen;
extern XMP_OptionBits voidOptionBits;

static const XMP_Uns8 * kUTF8_PacketStart = (const XMP_Uns8 *) "<?xpacket begin=";
static const XMP_Uns8 * kUTF8_PacketID    = (const XMP_Uns8 *) "W5M0MpCehiHzreSzNTczkc9d";
static const size_t     kUTF8_PacketHeaderLen = 51;	// ! strlen ( "<?xpacket begin='xxx' id='W5M0MpCehiHzreSzNTczkc9d'" )

static const XMP_Uns8 * kUTF8_PacketTrailer    = (const XMP_Uns8 *) "<?xpacket end=\"w\"?>";
static const size_t     kUTF8_PacketTrailerLen = 19;	// ! strlen ( kUTF8_PacketTrailer )

struct FileExtMapping {
	XMP_StringPtr  ext;
	XMP_FileFormat format;
};

extern const FileExtMapping kFileExtMap[];
extern const char * kKnownScannedFiles[];
extern const char * kKnownRejectedFiles[];

#define Uns8Ptr(p) ((XMP_Uns8 *) (p))

#define kTab ((char)0x09)
#define kLF ((char)0x0A)
#define kCR ((char)0x0D)

#define IsNewline( ch )    ( ((ch) == kLF) || ((ch) == kCR) )
#define IsSpaceOrTab( ch ) ( ((ch) == ' ') || ((ch) == kTab) )
#define IsWhitespace( ch ) ( IsSpaceOrTab ( ch ) || IsNewline ( ch ) )

static inline void MakeLowerCase ( std::string * str )
{
	for ( size_t i = 0, limit = str->size(); i < limit; ++i ) {
		char ch = (*str)[i];
		if ( ('A' <= ch) && (ch <= 'Z') ) (*str)[i] += 0x20;
	}
}

static inline void MakeUpperCase ( std::string * str )
{
	for ( size_t i = 0, limit = str->size(); i < limit; ++i ) {
		char ch = (*str)[i];
		if ( ('a' <= ch) && (ch <= 'z') ) (*str)[i] -= 0x20;
	}
}

#define XMP_LitMatch(s,l)		(std::strcmp((s),(l)) == 0)
#define XMP_LitNMatch(s,l,n)	(std::strncmp((s),(l),(n)) == 0)

#define IgnoreParam(p)	voidVoidPtr = (void*)&p

// =================================================================================================
// Support for asserts

#define _MakeStr(p)			#p
#define _NotifyMsg(n,c,f,l)	#n " failed: " #c " in " f " at line " _MakeStr(l)
#define _NotifyMsg2(msg,c,e) #e " " #msg ": " #c

#if ! XMP_DebugBuild
	#define XMP_Assert(c)	((void) 0)
#else
		#define XMP_Assert(c)	assert ( c )
#endif

	#define XMP_Enforce(c)																			\
		if ( ! (c) ) {																				\
			const char * assert_msg = _NotifyMsg ( XMP_Enforce, (c), __FILE__, __LINE__ );			\
			XMP_Throw ( assert_msg , kXMPErr_EnforceFailure );										\
		}
#define XMP_Validate(c,msg,e)																	\
	if ( ! (c) ) {																				\
		const char * enforce_msg = _NotifyMsg2(msg,c,e);										\
		XMP_Throw ( enforce_msg , e );															\
	}

// =================================================================================================
// Support for memory leak tracking

#ifndef TrackMallocAndFree
	#define TrackMallocAndFree 0
#endif

#if TrackMallocAndFree

	static void* ChattyMalloc ( size_t size )
	{
		void* ptr = malloc ( size );
		fprintf ( stderr, "Malloc %d bytes @ %.8X\n", size, ptr );
		return ptr;
	}

	static void ChattyFree ( void* ptr )
	{
		fprintf ( stderr, "Free @ %.8X\n", ptr );
		free ( ptr );
	}
	
	#define malloc(s) ChattyMalloc ( s )
	#define free(p) ChattyFree ( p )

#endif

// =================================================================================================
// Support for exceptions and thread locking

// *** Local copies of threading and exception macros from XMP_Impl.hpp. XMPFiles needs to use a
// *** separate thread lock from XMPCore. Eventually this could benefit from being recast into an
// *** XMPToolkit_Impl that supports separate locks.

typedef std::string	XMP_VarString;

#ifndef TraceXMPCalls
	#define TraceXMPCalls	0
#endif

#if ! TraceXMPCalls

	#define AnnounceThrow(msg)		/* Do nothing. */
	#define AnnounceCatch(msg)		/* Do nothing. */

	#define AnnounceEntry(proc)		/* Do nothing. */
	#define AnnounceNoLock(proc)	/* Do nothing. */
	#define AnnounceExit()			/* Do nothing. */

	#define ReportLock()			++sXMPFilesLockCount
	#define ReportUnlock()			--sXMPFilesLockCount
	#define ReportKeepLock()		/* Do nothing. */

#else

	extern FILE * xmpFilesOut;

	#define AnnounceThrow(msg)	\
		fprintf ( xmpFilesOut, "XMP_Throw: %s\n", msg ); fflush ( xmpFilesOut )
	#define AnnounceCatch(msg)	\
		fprintf ( xmpFilesOut, "Catch in %s: %s\n", procName, msg ); fflush ( xmpFilesOut )

	#define AnnounceEntry(proc)			\
		const char * procName = proc;	\
		fprintf ( xmpFilesOut, "Entering %s\n", procName ); fflush ( xmpFilesOut )
	#define AnnounceNoLock(proc)		\
		const char * procName = proc;	\
		fprintf ( xmpFilesOut, "Entering %s (no lock)\n", procName ); fflush ( xmpFilesOut )
	#define AnnounceExit()	\
		fprintf ( xmpFilesOut, "Exiting %s\n", procName ); fflush ( xmpFilesOut )

	#define ReportLock()	\
		++sXMPFilesLockCount; fprintf ( xmpFilesOut, "  Auto lock, count = %d\n", sXMPFilesLockCount ); fflush ( xmpFilesOut )
	#define ReportUnlock()	\
		--sXMPFilesLockCount; fprintf ( xmpFilesOut, "  Auto unlock, count = %d\n", sXMPFilesLockCount ); fflush ( xmpFilesOut )
	#define ReportKeepLock()	\
		fprintf ( xmpFilesOut, "  Keeping lock, count = %d\n", sXMPFilesLockCount ); fflush ( xmpFilesOut )

#endif

#define XMP_Throw(msg,id)	{ AnnounceThrow ( msg ); throw XMP_Error ( id, msg ); }

// -------------------------------------------------------------------------------------------------

#if XMP_WinBuild
	typedef CRITICAL_SECTION XMP_Mutex;
#else
	// Use pthread for both Mac and generic UNIX.
	typedef pthread_mutex_t XMP_Mutex;
#endif

extern XMP_Mutex sXMPFilesLock;
extern int sXMPFilesLockCount;	// Keep signed to catch unlock errors.
extern XMP_VarString * sXMPFilesExceptionMessage;

extern bool XMP_InitMutex ( XMP_Mutex * mutex );
extern void XMP_TermMutex ( XMP_Mutex & mutex );

extern void XMP_EnterCriticalRegion ( XMP_Mutex & mutex );
extern void XMP_ExitCriticalRegion ( XMP_Mutex & mutex );

class XMPFiles_AutoMutex {
public:
	XMPFiles_AutoMutex() : mutex(&sXMPFilesLock) { XMP_EnterCriticalRegion ( *mutex ); ReportLock(); };
	~XMPFiles_AutoMutex() { if ( mutex != 0 ) { ReportUnlock(); XMP_ExitCriticalRegion ( *mutex ); mutex = 0; } };
	void KeepLock() { ReportKeepLock(); mutex = 0; };
private:
	XMP_Mutex * mutex;
};

// *** Switch to XMPEnterObjectWrapper & XMPEnterStaticWrapper, to allow for per-object locks.

// ! Don't do the initialization check (sXMP_InitCount > 0) for the no-lock case. That macro is used
// ! by WXMPMeta_Initialize_1.

#define XMP_ENTER_WRAPPER_NO_LOCK(proc)						\
	AnnounceNoLock ( proc );								\
	XMP_Assert ( (0 <= sXMPFilesLockCount) && (sXMPFilesLockCount <= 1) );	\
	try {													\
		wResult->errMessage = 0;

#define XMP_ENTER_WRAPPER(proc)								\
	AnnounceEntry ( proc );									\
	XMP_Assert ( sXMPFilesInitCount > 0 );					\
	XMP_Assert ( (0 <= sXMPFilesLockCount) && (sXMPFilesLockCount <= 1) );	\
	try {													\
		XMPFiles_AutoMutex mutex;							\
		wResult->errMessage = 0;

#define XMP_EXIT_WRAPPER	\
	XMP_CATCH_EXCEPTIONS	\
	AnnounceExit();

#define XMP_EXIT_WRAPPER_KEEP_LOCK(keep)	\
		if ( keep ) mutex.KeepLock();		\
	XMP_CATCH_EXCEPTIONS					\
	AnnounceExit();

#define XMP_EXIT_WRAPPER_NO_THROW				\
	} catch ( ... )	{							\
		AnnounceCatch ( "no-throw catch-all" );	\
		/* Do nothing. */						\
	}											\
	AnnounceExit();

#define XMP_CATCH_EXCEPTIONS										\
	} catch ( XMP_Error & xmpErr ) {								\
		wResult->int32Result = xmpErr.GetID(); 						\
		wResult->ptrResult   = (void*)"XMP";						\
		wResult->errMessage  = xmpErr.GetErrMsg();					\
		if ( wResult->errMessage == 0 ) wResult->errMessage = "";	\
		AnnounceCatch ( wResult->errMessage );						\
	} catch ( std::exception & stdErr ) {							\
		wResult->int32Result = kXMPErr_StdException; 				\
		wResult->errMessage  = stdErr.what(); 						\
		if ( wResult->errMessage == 0 ) wResult->errMessage = "";	\
		AnnounceCatch ( wResult->errMessage );						\
	} catch ( ... ) {												\
		wResult->int32Result = kXMPErr_UnknownException; 			\
		wResult->errMessage  = "Caught unknown exception";			\
		AnnounceCatch ( wResult->errMessage );						\
	}

#if XMP_DebugBuild
	#define RELEASE_NO_THROW	/* empty */
#else
	#define RELEASE_NO_THROW	throw()
#endif

// =================================================================================================
// FileHandler declarations

extern void ReadXMPPacket ( XMPFileHandler * handler );

extern void FillPacketInfo ( const XMP_VarString & packet, XMP_PacketInfo * info );

class XMPFileHandler {	// See XMPFiles.hpp for usage notes.
public:
    
    #define DefaultCTorPresets							\
    	handlerFlags(0), stdCharForm(kXMP_CharUnknown),	\
    	containsTNail(false), processedTNail(false),	\
    	containsXMP(false), processedXMP(false), needsUpdate(false)

    XMPFileHandler() : parent(0), DefaultCTorPresets {};
    XMPFileHandler (XMPFiles * _parent) : parent(_parent), DefaultCTorPresets {};
    
	virtual ~XMPFileHandler() {};	// ! The specific handler is responsible for tnailInfo.tnailImage.
    
	virtual void CacheFileData() = 0;
	virtual void ProcessTNail();	// The default implementation just sets processedTNail to true.
	virtual void ProcessXMP();		// The default implementation just parses the XMP.
	
	virtual XMP_OptionBits GetSerializeOptions();	// The default is compact.

	virtual void UpdateFile ( bool doSafeUpdate ) = 0;
    virtual void WriteFile ( LFA_FileRef sourceRef, const std::string & sourcePath ) = 0;

	// ! Leave the data members public so common code can see them.
	
	XMPFiles *     parent;			// Let's the handler see the file info.
	XMP_OptionBits handlerFlags;	// Capabilities of this handler.
	XMP_Uns8       stdCharForm;		// The standard character form for output.

	bool containsTNail;		// True if the file has a native thumbnail.
	bool processedTNail;	// True if the cached thumbnail data has been processed.
	bool containsXMP;		// True if the file has XMP or PutXMP has been called.
	bool processedXMP;		// True if the XMP is parsed and reconciled.
	bool needsUpdate;		// True if the file needs to be updated.

	XMP_PacketInfo packetInfo;	// ! This is always info about the packet in the file, if any!
	std::string    xmpPacket;	// ! This is the current XMP, updated by XMPFiles::PutXMP.
	SXMPMeta       xmpObj;

	XMP_ThumbnailInfo tnailInfo;

};	// XMPFileHandler

typedef XMPFileHandler * (* XMPFileHandlerCTor) ( XMPFiles * parent );

typedef bool (* CheckFileFormatProc ) ( XMP_FileFormat format,
										XMP_StringPtr  filePath,
										LFA_FileRef    fileRef,
										XMPFiles *     parent );

typedef bool (*CheckFolderFormatProc ) ( XMP_FileFormat format,
										 const std::string & rootPath,
										 const std::string & gpName,
										 const std::string & parentName,
										 const std::string & leafName,
										 XMPFiles * parent );

// =================================================================================================

#if XMP_MacBuild
	extern LFA_FileRef LFA_OpenRsrc ( const char * fileName, char openMode );	// Open the Mac resource fork.
#endif

extern void CreateTempFile ( const std::string & origPath, std::string * tempPath, bool copyMacRsrc = false );
enum { kCopyMacRsrc = true };

struct AutoFile {	// Provides auto close of files on exit or exception.
	LFA_FileRef fileRef;
	AutoFile() : fileRef(0) {};
	~AutoFile() { if ( fileRef != 0 ) LFA_Close ( fileRef ); };
};

enum { kFMode_DoesNotExist, kFMode_IsFile, kFMode_IsFolder, kFMode_IsOther };
typedef XMP_Uns8 FileMode;

#if XMP_WinBuild
	#define kDirChar '\\'
#else
	#define kDirChar '/'
#endif

class XMP_FolderInfo {
public:

	XMP_FolderInfo() : dirRef(0) {};
	~XMP_FolderInfo() { if ( this->dirRef != 0 ) this->Close(); };
	
	void Open ( const char * folderPath );
	void Close();
	
	bool GetFolderPath ( XMP_VarString * folderPath );
	bool GetNextChild ( XMP_VarString * childName );

private:

	std::string folderPath;

	#if XMP_WinBuild
		HANDLE dirRef;	// Windows uses FindFirstFile and FindNextFile.
	#else
		DIR * dirRef;	// Mac and UNIX use the POSIX opendir/readdir/closedir functions.
	#endif
	
};

extern FileMode GetFileMode ( const char * path );

static inline FileMode GetChildMode ( std::string & path, XMP_StringPtr childName )
{
	size_t pathLen = path.size();
	path += kDirChar;
	path += childName;
	FileMode mode = GetFileMode ( path.c_str() );
	path.erase ( pathLen );
	return mode;
}

static inline void SplitLeafName ( std::string * path, std::string * leafName )
{
	size_t dirPos = path->size();
	if ( dirPos == 0 ) {
		leafName->erase();
		return;
	}
	
	for ( --dirPos; dirPos > 0; --dirPos ) {
		#if XMP_WinBuild
			if ( (*path)[dirPos] == '/' ) (*path)[dirPos] = kDirChar;	// Tolerate both '\' and '/'.
		#endif
		if ( (*path)[dirPos] == kDirChar ) break;
	}
	if ( (*path)[dirPos] == kDirChar ) {
		leafName->assign ( &(*path)[dirPos+1] );
		path->erase ( dirPos );
	} else if ( dirPos == 0 ) {
		leafName->erase();
		leafName->swap ( *path );
	}

}

// -------------------------------------------------------------------------------------------------

static inline bool CheckBytes ( const void * left, const void * right, size_t length )
{
	return (std::memcmp ( left, right, length ) == 0);
}

// -------------------------------------------------------------------------------------------------

static inline bool CheckCString ( const void * left, const void * right )
{
	return (std::strcmp ( (char*)left, (char*)right ) == 0);
}

// -------------------------------------------------------------------------------------------------
// CheckFileSpace and RefillBuffer
// -------------------------------
//
// There is always a problem in file scanning of managing what you want to check against what is
// available in a buffer, trying to keep the logic understandable and minimize data movement. The
// CheckFileSpace and RefillBuffer functions are used here for a standard scanning model.
//
// The format scanning routines have an outer, "infinite" loop that looks for file markers. There
// is a local (on stack) buffer, a pointer to the current position in the buffer, and a pointer for
// the end of the buffer. The end pointer is just past the end of the buffer, "bufPtr == bufLimit"
// means you are out of data. The outer loop ends when the necessary markers are found or we reach
// the end of the file.
//
// The filePos is the file offset of the start of the current buffer. This is maintained so that
// we can tell where the packet is in the file, part of the info returned to the client.
//
// At each check CheckFileSpace is used to make sure there is enough data in the buffer for the
// check to be made. It refills the buffer if necessary, preserving the unprocessed data, setting
// bufPtr and bufLimit appropriately. If we are too close to the end of the file to make the check
// a failure status is returned.

enum { kIOBufferSize = 128*1024 };

struct IOBuffer {
	XMP_Int64 filePos;
	XMP_Uns8* ptr;
	XMP_Uns8* limit;
	size_t    len;
	XMP_Uns8  data [kIOBufferSize];
	IOBuffer() : filePos(0), ptr(&data[0]), limit(ptr), len(0) {};
};

static inline void FillBuffer ( LFA_FileRef fileRef, XMP_Int64 fileOffset, IOBuffer* ioBuf )
{
	ioBuf->filePos = LFA_Seek ( fileRef, fileOffset, SEEK_SET );
	if ( ioBuf->filePos != fileOffset ) XMP_Throw ( "Seek failure in FillBuffer", kXMPErr_ExternalFailure );
	ioBuf->len = LFA_Read ( fileRef, &ioBuf->data[0], kIOBufferSize );
	ioBuf->ptr = &ioBuf->data[0];
	ioBuf->limit = ioBuf->ptr + ioBuf->len;
}

static inline void MoveToOffset ( LFA_FileRef fileRef, XMP_Int64 fileOffset, IOBuffer* ioBuf )
{
	if ( (ioBuf->filePos <= fileOffset) && (fileOffset < (XMP_Int64)(ioBuf->filePos + ioBuf->len)) ) {
		size_t bufOffset = (size_t)(fileOffset - ioBuf->filePos);
		ioBuf->ptr = &ioBuf->data[bufOffset];
	} else {
		FillBuffer ( fileRef, fileOffset, ioBuf );
	}
}

static inline void RefillBuffer ( LFA_FileRef fileRef, IOBuffer* ioBuf )
{
	ioBuf->filePos += (ioBuf->ptr - &ioBuf->data[0]);	// ! Increment before the read.
	size_t bufTail = ioBuf->limit - ioBuf->ptr;	// We'll re-read the tail portion of the buffer.
	if ( bufTail > 0 ) ioBuf->filePos = LFA_Seek ( fileRef, -((XMP_Int64)bufTail), SEEK_CUR );
	ioBuf->len = LFA_Read ( fileRef, &ioBuf->data[0], kIOBufferSize );
	ioBuf->ptr = &ioBuf->data[0];
	ioBuf->limit = ioBuf->ptr + ioBuf->len;
}

static inline bool CheckFileSpace ( LFA_FileRef fileRef, IOBuffer* ioBuf, size_t neededLen )
{
	if ( size_t(ioBuf->limit - ioBuf->ptr) < size_t(neededLen) ) {	// ! Avoid VS.Net compare warnings.
		RefillBuffer ( fileRef, ioBuf );
	}
	return (size_t(ioBuf->limit - ioBuf->ptr) >= size_t(neededLen));
}

#endif /* __XMPFiles_Impl_hpp__ */
