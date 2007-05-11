// =================================================================================================
// ADOBE SYSTEMS INCORPORATED
// Copyright 2002-2007 Adobe Systems Incorporated
// All Rights Reserved
//
// NOTICE: Adobe permits you to use, modify, and distribute this file in accordance with the terms
// of the Adobe license agreement accompanying it.
// =================================================================================================

#include <vector>
#include <string.h>

#include "XMPFiles_Impl.hpp"
#include "UnicodeConversions.hpp"
#include "QuickTime_Support.hpp"

// These are the official, fully supported handlers.
#include "FileHandlers/JPEG_Handler.hpp"
#include "FileHandlers/TIFF_Handler.hpp"
#include "FileHandlers/PSD_Handler.hpp"
#include "FileHandlers/InDesign_Handler.hpp"
#include "FileHandlers/PostScript_Handler.hpp"
#include "FileHandlers/Scanner_Handler.hpp"
#include "FileHandlers/MOV_Handler.hpp"
#include "FileHandlers/MPEG_Handler.hpp"
#include "FileHandlers/MP3_Handler.hpp"
#include "FileHandlers/PNG_Handler.hpp"
#include "FileHandlers/AVI_Handler.hpp"
#include "FileHandlers/WAV_Handler.hpp"

// =================================================================================================
/// \file XMPFiles.cpp
/// \brief High level support to access metadata in files of interest to Adobe applications.
///
/// This header ...
///
// =================================================================================================
 
// =================================================================================================

long sXMPFilesInitCount = 0;

#if GatherPerformanceData
	APIPerfCollection* sAPIPerf = 0;
#endif

// These are embedded version strings.

#if XMP_DebugBuild
	#define kXMPFiles_DebugFlag 1
#else
	#define kXMPFiles_DebugFlag 0
#endif

#define kXMPFiles_VersionNumber	( (kXMPFiles_DebugFlag << 31)    |	\
                                  (XMP_API_VERSION_MAJOR << 24) |	\
						          (XMP_API_VERSION_MINOR << 16) |	\
						          (XMP_API_VERSION_MICRO << 8) )

	#define kXMPFilesName "XMP Files"
	#define kXMPFiles_VersionMessage kXMPFilesName " " XMP_API_VERSION_STRING
const char * kXMPFiles_EmbeddedVersion   = kXMPFiles_VersionMessage;
const char * kXMPFiles_EmbeddedCopyright = kXMPFilesName " " kXMP_CopyrightStr;

// =================================================================================================

struct XMPFileHandlerInfo {
	XMP_FileFormat     format;
	XMP_OptionBits     flags;
	CheckFormatProc    checkProc;
	XMPFileHandlerCTor handlerCTor;
	XMPFileHandlerInfo() : format(0), flags(0), checkProc(0), handlerCTor(0) {};
	XMPFileHandlerInfo ( XMP_FileFormat _format, XMP_OptionBits _flags,
	                      CheckFormatProc _checkProc, XMPFileHandlerCTor _handlerCTor )
		: format(_format), flags(_flags), checkProc(_checkProc), handlerCTor(_handlerCTor) {};
};

typedef std::vector <XMPFileHandlerInfo> XMPFileHandlerTable;
typedef XMPFileHandlerTable::iterator XMPFileHandlerTablePos;

static XMPFileHandlerTable * sRegisteredHandlers = 0;	// ! Only smart handlers are registered!

// =================================================================================================

static XMPFileHandlerTablePos
FindHandler ( XMP_FileFormat format,
			  std::string &  fileExt )
{
	if ( (format == kXMP_UnknownFile) && (! fileExt.empty()) ) {
		for ( int i = 0; kFileExtMap[i].format != 0; ++i ) {
			if ( fileExt == kFileExtMap[i].ext ) {
				format = kFileExtMap[i].format;
				break;
			}
		}
	}
	
	if ( format == kXMP_UnknownFile ) return sRegisteredHandlers->end();

	XMPFileHandlerTablePos handlerPos = sRegisteredHandlers->begin();
	for ( ; handlerPos != sRegisteredHandlers->end(); ++handlerPos ) {
		if ( handlerPos->format == format ) return handlerPos;
	}
	
	return sRegisteredHandlers->end();
	
}	// FindHandler

// =================================================================================================

static void
RegisterXMPFileHandler ( XMP_FileFormat     format,
						 XMP_OptionBits     flags,
						 CheckFormatProc    checkProc,
						 XMPFileHandlerCTor handlerCTor )
{
	XMP_Assert ( format != kXMP_UnknownFile );
	std::string noExt;
	
	if ( FindHandler ( format, noExt ) != sRegisteredHandlers->end() ) {
		XMP_Throw ( "Duplicate handler registration", kXMPErr_InternalFailure );
	}
	
	if ( (flags & kXMPFiles_CanInjectXMP) && (! (flags & kXMPFiles_CanExpand)) ) {
		XMP_Throw ( "Inconsistent handler flags", kXMPErr_InternalFailure );
	}

	sRegisteredHandlers->push_back ( XMPFileHandlerInfo ( format, flags, checkProc, handlerCTor ) );
	
}	// RegisterXMPFileHandler

// =================================================================================================

/* class-static */
void
XMPFiles::GetVersionInfo ( XMP_VersionInfo * info )
{

	memset ( info, 0, sizeof(XMP_VersionInfo) );
	
	info->major   = XMP_API_VERSION_MAJOR;
	info->minor   = XMP_API_VERSION_MINOR;
	info->micro   = XMP_API_VERSION_MICRO;
	info->isDebug = kXMPFiles_DebugFlag;
	info->flags   = 0;	// ! None defined yet.
	info->message = kXMPFiles_VersionMessage;
	
}	// XMPFiles::GetVersionInfo

// =================================================================================================

static bool sIgnoreQuickTime = false;

/* class static */
bool
XMPFiles::Initialize ( XMP_OptionBits options /* = 0 */ )
{
	++sXMPFilesInitCount;
	if ( sXMPFilesInitCount > 1 ) return true;
	
	SXMPMeta::Initialize();	// Just in case the client does not.
	
	#if GatherPerformanceData
		sAPIPerf = new APIPerfCollection;
	#endif

	XMP_InitMutex ( &sXMPFilesLock );
	
	XMP_Uns16 endianInt  = 0x00FF;
	XMP_Uns8  endianByte = *((XMP_Uns8*)&endianInt);
	if ( kBigEndianHost ) {
		if ( endianByte != 0 ) XMP_Throw ( "Big endian flag mismatch", kXMPErr_InternalFailure );
	} else {
		if ( endianByte != 0xFF ) XMP_Throw ( "Little endian flag mismatch", kXMPErr_InternalFailure );
	}
	
	XMP_Assert ( kUTF8_PacketHeaderLen == strlen ( "<?xpacket begin='xxx' id='W5M0MpCehiHzreSzNTczkc9d'" ) );
	XMP_Assert ( kUTF8_PacketTrailerLen == strlen ( (const char *) kUTF8_PacketTrailer ) );
	
	sRegisteredHandlers = new XMPFileHandlerTable;
	sXMPFilesExceptionMessage = new XMP_VarString;

	InitializeUnicodeConversions();
	
	sIgnoreQuickTime = XMP_OptionIsSet ( options, kXMPFiles_NoQuickTimeInit );
	if ( ! sIgnoreQuickTime ) {
		(void) QuickTime_Support::MainInitialize();	// Don't worry about failure, the MOV handler checks that.
	}
	
	// ----------------------------------------------------------------------------------
	// First register the handlers that don't want to open and close the file themselves.

	XMP_Assert ( ! (kJPEG_HandlerFlags & kXMPFiles_HandlerOwnsFile) );
	RegisterXMPFileHandler ( kXMP_JPEGFile, kJPEG_HandlerFlags, JPEG_CheckFormat, JPEG_MetaHandlerCTor );

	XMP_Assert ( ! (kTIFF_HandlerFlags & kXMPFiles_HandlerOwnsFile) );
	RegisterXMPFileHandler ( kXMP_TIFFFile, kTIFF_HandlerFlags, TIFF_CheckFormat, TIFF_MetaHandlerCTor );

	XMP_Assert ( ! (kPSD_HandlerFlags & kXMPFiles_HandlerOwnsFile) );
	RegisterXMPFileHandler ( kXMP_PhotoshopFile, kPSD_HandlerFlags, PSD_CheckFormat, PSD_MetaHandlerCTor );
	
	XMP_Assert ( ! (kInDesign_HandlerFlags & kXMPFiles_HandlerOwnsFile) );
	RegisterXMPFileHandler ( kXMP_InDesignFile, kInDesign_HandlerFlags, InDesign_CheckFormat, InDesign_MetaHandlerCTor );

	// ! EPS and PostScript have the same handler, EPS is a proper subset of PostScript.
	XMP_Assert ( ! (kPostScript_HandlerFlags & kXMPFiles_HandlerOwnsFile) );
	RegisterXMPFileHandler ( kXMP_EPSFile, kPostScript_HandlerFlags, PostScript_CheckFormat, PostScript_MetaHandlerCTor );
	RegisterXMPFileHandler ( kXMP_PostScriptFile, kPostScript_HandlerFlags, PostScript_CheckFormat, PostScript_MetaHandlerCTor );

	XMP_Assert ( ! (kPNG_HandlerFlags & kXMPFiles_HandlerOwnsFile) );
	RegisterXMPFileHandler ( kXMP_PNGFile, kPNG_HandlerFlags, PNG_CheckFormat, PNG_MetaHandlerCTor );

	XMP_Assert ( ! (kAVI_HandlerFlags & kXMPFiles_HandlerOwnsFile) );
	RegisterXMPFileHandler ( kXMP_AVIFile, kAVI_HandlerFlags, AVI_CheckFormat, AVI_MetaHandlerCTor );

	XMP_Assert ( ! (kWAV_HandlerFlags & kXMPFiles_HandlerOwnsFile) );
	RegisterXMPFileHandler ( kXMP_WAVFile, kWAV_HandlerFlags, WAV_CheckFormat, WAV_MetaHandlerCTor );

	XMP_Assert ( ! (kMP3_HandlerFlags & kXMPFiles_HandlerOwnsFile) );
	RegisterXMPFileHandler ( kXMP_MP3File, kMP3_HandlerFlags, MP3_CheckFormat, MP3_MetaHandlerCTor );

	#if XMP_WinBuild
	
		// Windows-only handlers.

	#endif

	// -----------------------------------------------------------------------------
	// Now register the handlers that do want to open and close the file themselves.

	XMP_Assert ( kMOV_HandlerFlags & kXMPFiles_HandlerOwnsFile );
	RegisterXMPFileHandler ( kXMP_MOVFile, kMOV_HandlerFlags, MOV_CheckFormat, MOV_MetaHandlerCTor );

	XMP_Assert ( kMPEG_HandlerFlags & kXMPFiles_HandlerOwnsFile );
	RegisterXMPFileHandler ( kXMP_MPEGFile, kMPEG_HandlerFlags, MPEG_CheckFormat, MPEG_MetaHandlerCTor );

	// Make sure the embedded info strings are referenced and kept.
	if ( (kXMPFiles_EmbeddedVersion[0] == 0) || (kXMPFiles_EmbeddedCopyright[0] == 0) ) return false;
	return true;

}	// XMPFiles::Initialize
 
// =================================================================================================

#if GatherPerformanceData

#if XMP_WinBuild
	#pragma warning ( disable : 4996 )	// '...' was declared deprecated
#endif

#include "PerfUtils.cpp"

static void ReportPerformanceData()
{
	struct SummaryInfo {
		size_t callCount;
		double totalTime;
		SummaryInfo() : callCount(0), totalTime(0.0) {};
	};
	
	SummaryInfo perfSummary [kAPIPerfProcCount];
	
	XMP_DateTime now;
	SXMPUtils::CurrentDateTime ( &now );
	std::string nowStr;
	SXMPUtils::ConvertFromDate ( now, &nowStr );
	
	#if XMP_WinBuild
		#define kPerfLogPath "C:\\XMPFilesPerformanceLog.txt"
	#else
		#define kPerfLogPath "/XMPFilesPerformanceLog.txt"
	#endif
	FILE * perfLog = fopen ( kPerfLogPath, "ab" );
	if ( perfLog == 0 ) return;
	
	fprintf ( perfLog, "\n\n// =================================================================================================\n\n" );
	fprintf ( perfLog, "XMPFiles performance data\n" );
	fprintf ( perfLog, "   %s\n", kXMPFiles_VersionMessage );
	fprintf ( perfLog, "   Reported at %s\n", nowStr.c_str() );
	fprintf ( perfLog, "   %s\n", PerfUtils::GetTimerInfo() );
	
	// Gather and report the summary info.
	
	for ( size_t i = 0; i < sAPIPerf->size(); ++i ) {
		SummaryInfo& summaryItem = perfSummary [(*sAPIPerf)[i].whichProc];
		++summaryItem.callCount;
		summaryItem.totalTime += (*sAPIPerf)[i].elapsedTime;
	}
	
	fprintf ( perfLog, "\nSummary data:\n" );
	
	for ( size_t i = 0; i < kAPIPerfProcCount; ++i ) {
		long averageTime = 0;
		if ( perfSummary[i].callCount != 0 ) {
			averageTime = (long) (((perfSummary[i].totalTime/perfSummary[i].callCount) * 1000.0*1000.0) + 0.5);
		}
		fprintf ( perfLog, "   %s, %d total calls, %d average microseconds per call\n",
				  kAPIPerfNames[i], perfSummary[i].callCount, averageTime );
	}

	fprintf ( perfLog, "\nPer-call data:\n" );
	
	// Report the info for each call.
	
	for ( size_t i = 0; i < sAPIPerf->size(); ++i ) {
		long time = (long) (((*sAPIPerf)[i].elapsedTime * 1000.0*1000.0) + 0.5);
		fprintf ( perfLog, "   %s, %d microSec, ref %.8X, \"%s\"\n",
				  kAPIPerfNames[(*sAPIPerf)[i].whichProc], time,
				  (*sAPIPerf)[i].xmpFilesRef, (*sAPIPerf)[i].extraInfo.c_str() );
	}
	
	fclose ( perfLog );
	
}	// ReportAReportPerformanceDataPIPerformance

#endif

// =================================================================================================

#define EliminateGlobal(g) delete ( g ); g = 0

/* class static */
void
XMPFiles::Terminate()
{
	--sXMPFilesInitCount;
	if ( sXMPFilesInitCount != 0 ) return;

	if ( ! sIgnoreQuickTime ) QuickTime_Support::MainTerminate();
	
	#if GatherPerformanceData
		ReportPerformanceData();
		EliminateGlobal ( sAPIPerf );
	#endif
	
	EliminateGlobal ( sRegisteredHandlers );
	EliminateGlobal ( sXMPFilesExceptionMessage );
	
	XMP_TermMutex ( sXMPFilesLock );
	
	SXMPMeta::Terminate();	// Just in case the client does not.

}	// XMPFiles::Terminate

// =================================================================================================

XMPFiles::XMPFiles() :
	clientRefs(0),
	format(kXMP_UnknownFile),
	fileRef(0),
	openFlags(0),
	abortProc(0),
	abortArg(0),
	handler(0),
	handlerTemp(0)
{
	// Nothing more to do, clientRefs is incremented in wrapper.

}	// XMPFiles::XMPFiles
 
// =================================================================================================

XMPFiles::~XMPFiles()
{
	XMP_Assert ( this->clientRefs <= 0 );

	if ( this->handler != 0 ) {
		delete this->handler;
		this->handler = 0;
	}
	if ( this->fileRef != 0 ) {
		LFA_Close ( this->fileRef );
		this->fileRef = 0;
	}

}	// XMPFiles::~XMPFiles
 
// =================================================================================================

/* class static */
void
XMPFiles::UnlockLib()
{

	// *** Would be better to have the count in an object with the mutex.
    #if TraceXMPLocking
    	fprintf ( xmpOut, "  Unlocking XMPFiles, count = %d\n", sXMPFilesLockCount ); fflush ( xmpOut );
	#endif
    --sXMPFilesLockCount;
    XMP_Assert ( sXMPFilesLockCount == 0 );
	XMP_ExitCriticalRegion ( sXMPFilesLock );

}	// XMPFiles::UnlockLib
 
// =================================================================================================

void
XMPFiles::UnlockObj()
{

	// *** Would be better to have the count in an object with the mutex.
    #if TraceXMPLocking
    	fprintf ( xmpOut, "  Unlocking XMPFiles, count = %d\n", sXMPFilesLockCount ); fflush ( xmpOut );
	#endif
    --sXMPFilesLockCount;
    XMP_Assert ( sXMPFilesLockCount == 0 );
	XMP_ExitCriticalRegion ( sXMPFilesLock );

}	// XMPFiles::UnlockObj
 
// =================================================================================================

/* class static */
bool
XMPFiles::GetFormatInfo ( XMP_FileFormat   format,
                          XMP_OptionBits * flags /* = 0 */ )
{
	if ( flags == 0 ) flags = &voidOptionBits;
	
	XMPFileHandlerTablePos handlerPos = sRegisteredHandlers->begin();
	for ( ; handlerPos != sRegisteredHandlers->end(); ++handlerPos ) {
		if ( handlerPos->format == format ) {
			*flags = handlerPos->flags;
			return true;
		}
	}
	
	return false;
	
}	// XMPFiles::GetFormatInfo
 
// =================================================================================================

bool
XMPFiles::OpenFile ( XMP_StringPtr  filePath,
	                 XMP_FileFormat format /* = kXMP_UnknownFile */,
	                 XMP_OptionBits openFlags /* = 0 */ )
{
	// *** Check consistency of the option bits, use of kXMPFiles_OpenUseSmartHandler.
	
	if ( this->handler != 0 ) XMP_Throw ( "File already open", kXMPErr_BadParam );
	
	// Select the file handler. Don't set the XMPFiles member variables until the end, leave them
	// clean in case something fails along the way. Use separate handler search loops, filtering on
	// whether the handler wants to open the file itself.
	
	// An initial handler is tried based on the format parameter, or the file extension if the
	// parameter is kXMP_UnknownFile. If that fails, all of the handlers are tried in registration
	// order. The CheckProc should do as little as possible to determine the format, based on the
	// actual file content. If that is not possible, use the format hint. The initial CheckProc call
	// has the presumed format in this->format, the later calls have kXMP_UnknownFile there.
	
	bool foundHandler = false;
	LFA_FileRef fileRef = 0;
	
	char openMode = 'r';
	if ( openFlags & kXMPFiles_OpenForUpdate ) openMode = 'w';
	
	XMPFileHandlerTablePos handlerPos = sRegisteredHandlers->end();
	
	std::string fileExt;
	size_t extPos = strlen ( filePath );
	for ( --extPos; extPos > 0; --extPos ) if ( filePath[extPos] == '.' ) break;
	if ( filePath[extPos] == '.' ) {
		++extPos;
		fileExt.assign ( &filePath[extPos] );
		for ( size_t i = 0; i < fileExt.size(); ++i ) {
			if ( ('A' <= fileExt[i]) && (fileExt[i] <= 'Z') ) fileExt[i] += 0x20;
		}
	}
	
	this->format = kXMP_UnknownFile;	// Make sure it is preset for later check.
	this->openFlags = openFlags;		// ! The QuickTime support needs the InBackground bit.
	
	if ( ! (openFlags & kXMPFiles_OpenUsePacketScanning) ) {

		// Try an initial handler based on the format or file extension.
		handlerPos = FindHandler ( format, fileExt );
		if ( handlerPos != sRegisteredHandlers->end() ) {
			if ( ! (handlerPos->flags & kXMPFiles_HandlerOwnsFile) ) fileRef = LFA_Open ( filePath, openMode );
			this->format = handlerPos->format;	// ! Hack to tell the CheckProc this is the first call.
			foundHandler = handlerPos->checkProc ( handlerPos->format, filePath, fileRef, this );
		}
		
		if ( (openFlags & kXMPFiles_OpenStrictly) && (format != kXMP_UnknownFile) && (! foundHandler) ) {
			return false;
		}
		
		// Search the handlers that don't want to open the file themselves. They must be registered first.
		if ( ! foundHandler ) {
			if ( fileRef == 0 ) fileRef = LFA_Open ( filePath, openMode );
			XMP_Assert ( fileRef != 0 ); // LFA_Open must either succeed or throw.
			for ( handlerPos = sRegisteredHandlers->begin(); handlerPos != sRegisteredHandlers->end(); ++handlerPos ) {
				if ( handlerPos->flags & kXMPFiles_HandlerOwnsFile ) break;
				this->format = kXMP_UnknownFile;	// ! Hack to tell the CheckProc this is not the first call.
				foundHandler = handlerPos->checkProc ( handlerPos->format, filePath, fileRef, this );
				if ( foundHandler ) break;	// ! Exit before incrementing handlerPos.
			}
		}

		// Search the handlers that do want to open the file themselves. They must be registered last.
		if ( ! foundHandler ) {
			LFA_Close ( fileRef );
			fileRef = 0;
			for ( ; handlerPos != sRegisteredHandlers->end(); ++handlerPos ) {
				XMP_Assert ( handlerPos->flags & kXMPFiles_HandlerOwnsFile );
				this->format = kXMP_UnknownFile;	// ! Hack to tell the CheckProc this is not the first call.
				foundHandler = handlerPos->checkProc ( handlerPos->format, filePath, 0, this );
				if ( foundHandler ) break;	// ! Exit before incrementing handlerPos.
			}
		}
		
		if ( (! foundHandler) && (openFlags & kXMPFiles_OpenUseSmartHandler) ) {
			return false;
		}

	}
	
	// Create the handler object, fill in the XMPFiles member variables, cache the desired file data.
	
	XMPFileHandlerCTor handlerCTor = 0;
	XMP_OptionBits     handlerFlags = 0;
	
	if ( foundHandler ) {
	
		// Found a smart handler.
		format = handlerPos->format;
		handlerCTor  = handlerPos->handlerCTor;
		handlerFlags = handlerPos->flags;

	} else {

		// No smart handler, packet scan if appropriate.
		if ( openFlags & kXMPFiles_OpenLimitedScanning ) {
			size_t i;
			for ( i = 0; kKnownScannedFiles[i] != 0; ++i ) {
				if ( fileExt == kKnownScannedFiles[i] ) break;
			}
			if ( kKnownScannedFiles[i] == 0 ) return false;
		}
		format = kXMP_UnknownFile;
		handlerCTor  = Scanner_MetaHandlerCTor;
		handlerFlags = kScanner_HandlerFlags;
		if ( fileRef == 0 ) fileRef = LFA_Open ( filePath, openMode );

	}
	
	this->fileRef   = fileRef;
	this->filePath  = filePath;
	
	XMPFileHandler * handler = (*handlerCTor) ( this );
	XMP_Assert ( handlerFlags == handler->handlerFlags );
	
	this->handler = handler;
	if ( this->format == kXMP_UnknownFile ) this->format = format;	// ! The CheckProc might have set it.
	
	handler->CacheFileData();
	
	if ( ! (openFlags & kXMPFiles_OpenCacheTNail) ) {
		handler->containsTNail = false;	// Make sure GetThumbnail will cleanly return false.
		handler->processedTNail = true;
	}
	
	if ( handler->containsXMP ) {
		XMP_StringPtr packetStr = handler->xmpPacket.c_str();
		XMP_StringLen packetLen = handler->xmpPacket.size();
		if ( packetLen > 0 ) {
			handler->packetInfo.charForm  = GetPacketCharForm ( packetStr, packetLen );
			handler->packetInfo.padSize   = GetPacketPadSize ( packetStr, packetLen );
			handler->packetInfo.writeable = GetPacketRWMode ( packetStr, packetLen, XMP_GetCharSize ( handler->packetInfo.charForm ) );
		}
	}

	if ( (! (openFlags & kXMPFiles_OpenForUpdate)) && (! (handlerFlags & kXMPFiles_HandlerOwnsFile)) ) {
		// Close the disk file now if opened for read-only access.
		LFA_Close ( this->fileRef );
		this->fileRef = 0;
	}
	
	return true;

}	// XMPFiles::OpenFile
 
// =================================================================================================

void
XMPFiles::CloseFile ( XMP_OptionBits closeFlags /* = 0 */ )
{
	if ( this->handler == 0 ) return;	// Return if there is no open file (not an error).

	bool needsUpdate = this->handler->needsUpdate;
	XMP_OptionBits handlerFlags = this->handler->handlerFlags;
	
	// Decide if we're doing a safe update. If so, make sure the handler supports it. All handlers
	// that don't own the file tolerate safe update using common code below.
	
	bool doSafeUpdate = XMP_OptionIsSet ( closeFlags, kXMPFiles_UpdateSafely );
	#if GatherPerformanceData
		if ( doSafeUpdate ) sAPIPerf->back().extraInfo += ", safe update";	// Client wants safe update.
	#endif

	if ( ! (this->openFlags & kXMPFiles_OpenForUpdate) ) doSafeUpdate = false;
	if ( ! needsUpdate ) doSafeUpdate = false;
	
	bool safeUpdateOK = ( (handlerFlags & kXMPFiles_AllowsSafeUpdate) ||
						  (! (handlerFlags & kXMPFiles_HandlerOwnsFile)) );
	if ( doSafeUpdate && (! safeUpdateOK) ) {
		XMP_Throw ( "XMPFiles::CloseFile - Safe update not supported", kXMPErr_Unavailable );
	}

	// Try really hard to make sure the file is closed and the handler is deleted.

	LFA_FileRef origFileRef = this->fileRef;	// Used during crash-safe saves.
	std::string origFilePath ( this->filePath );

	LFA_FileRef tempFileRef = 0;
	std::string tempFilePath;

	LFA_FileRef copyFileRef = 0;
	std::string copyFilePath;

	try {
	
		if ( (! doSafeUpdate) || (handlerFlags & kXMPFiles_HandlerOwnsFile) ) {
		
			// Close the file without doing common crash-safe writing. The handler might do it.

			#if GatherPerformanceData
				if ( needsUpdate ) sAPIPerf->back().extraInfo += ", direct update";
			#endif

			if ( needsUpdate ) this->handler->UpdateFile ( doSafeUpdate );
			delete this->handler;
			this->handler = 0;
			if ( this->fileRef != 0 ) LFA_Close ( this->fileRef );
			this->fileRef = 0;

		} else {
		
			// Update the file in a crash-safe manner. This somewhat convoluted approach preserves
			// the ownership, permissions, and Mac resources of the final file - at a slightly
			// higher risk of confusion in the event of a midstream crash.
						
			if ( handlerFlags & kXMPFiles_CanRewrite ) {

				// The handler can rewrite an entire file based on the original. Do this into a temp
				// file next to the original, with the same ownership and permissions if possible.

				#if GatherPerformanceData
					sAPIPerf->back().extraInfo += ", file rewrite";
				#endif
				
				CreateTempFile ( origFilePath, &tempFilePath, kCopyMacRsrc );
				tempFileRef = LFA_Open ( tempFilePath.c_str(), 'w' );
				this->fileRef = tempFileRef;
				this->filePath = tempFilePath;
				this->handler->WriteFile ( origFileRef, origFilePath );

			} else {

				// The handler can only update an existing file. Do a little dance so the final file
				// is the original, thus preserving ownership, permissions, etc. This does have the
				// risk that the interim copy under the original name has "current" ownership and
				// permissions. The dance steps:
				//  - Copy the original file to a temp name.
				//  - Rename the original file to a different temp name.
				//  - Rename the copy file back to the original name.
				//  - Call the handler's UpdateFile method for the "original as temp" file.
				
				// *** A user abort might leave the copy file under the original name! Need better
				// *** duplicate code that handles all parts of a file, and for CreateTempFile to
				// *** preserve ownership and permissions. Then the original can stay put until
				// *** the final delete/rename.

				#if GatherPerformanceData
					sAPIPerf->back().extraInfo += ", copy update";
				#endif
				
				CreateTempFile ( origFilePath, &copyFilePath, kCopyMacRsrc );
				copyFileRef = LFA_Open ( copyFilePath.c_str(), 'w' );
				XMP_Int64 fileSize = LFA_Measure ( origFileRef );
				LFA_Seek ( origFileRef, 0, SEEK_SET );
				LFA_Copy ( origFileRef, copyFileRef, fileSize, this->abortProc, this->abortArg );

				LFA_Close ( origFileRef );
				origFileRef = this->fileRef = 0;
				LFA_Close ( copyFileRef );
				copyFileRef = 0;

				CreateTempFile ( origFilePath, &tempFilePath );
				LFA_Delete ( tempFilePath.c_str() );	// ! Slight risk of name being grabbed before rename.
				LFA_Rename ( origFilePath.c_str(), tempFilePath.c_str() );

				tempFileRef = LFA_Open ( tempFilePath.c_str(), 'w' );
				this->fileRef = tempFileRef;

				try {
					LFA_Rename ( copyFilePath.c_str(), origFilePath.c_str() );
				} catch ( ... ) {
					this->fileRef = 0;
					LFA_Close ( tempFileRef );
					LFA_Rename ( tempFilePath.c_str(), origFilePath.c_str() );
					throw;
				}

				XMP_Assert ( (tempFileRef != 0) && (tempFileRef == this->fileRef) );
				this->filePath = tempFilePath;
				this->handler->UpdateFile ( false );	// We're doing the safe update, not the handler.

			}

			delete this->handler;
			this->handler = 0;

			if ( this->fileRef != 0 ) LFA_Close ( this->fileRef );
			if ( origFileRef != 0 ) LFA_Close ( origFileRef );

			this->fileRef = 0;
			origFileRef = 0;
			tempFileRef = 0;
			
			LFA_Delete ( origFilePath.c_str() );
			LFA_Rename ( tempFilePath.c_str(), origFilePath.c_str() );

		}

	} catch ( ... ) {
	
		// *** Don't delete the temp or copy files, not sure which is best.

		try {
			if ( this->fileRef != 0 ) LFA_Close ( this->fileRef );
		} catch ( ... ) { /*Do nothing, throw the outer exception later. */ }
		try {
			if ( origFileRef != 0 ) LFA_Close ( origFileRef );
		} catch ( ... ) { /*Do nothing, throw the outer exception later. */ }
		try {
			if ( tempFileRef != 0 ) LFA_Close ( tempFileRef );
		} catch ( ... ) { /*Do nothing, throw the outer exception later. */ }
		try {
			if ( copyFileRef != 0 ) LFA_Close ( copyFileRef );
		} catch ( ... ) { /*Do nothing, throw the outer exception later. */ }
		try {
			if ( this->handler != 0 ) delete this->handler;
		} catch ( ... ) { /*Do nothing, throw the outer exception later. */ }
	
		this->handler   = 0;
		this->format    = kXMP_UnknownFile;
		this->fileRef   = 0;
		this->filePath.clear();
		this->openFlags = 0;

		throw;
	
	}
	
	// Clear the XMPFiles member variables.
	
	this->handler   = 0;
	this->format    = kXMP_UnknownFile;
	this->fileRef   = 0;
	this->filePath.clear();
	this->openFlags = 0;
	
}	// XMPFiles::CloseFile
 
// =================================================================================================

bool
XMPFiles::GetFileInfo ( XMP_StringPtr *  filePath /* = 0 */,
                        XMP_StringLen *  pathLen /* = 0 */,
	                    XMP_OptionBits * openFlags /* = 0 */,
	                    XMP_FileFormat * format /* = 0 */,
	                    XMP_OptionBits * handlerFlags /* = 0 */ )
{
	if ( this->handler == 0 ) return false;
	
	if ( filePath == 0 ) filePath = &voidStringPtr;
	if ( pathLen == 0 ) pathLen = &voidStringLen;
	if ( openFlags == 0 ) openFlags = &voidOptionBits;
	if ( format == 0 ) format = &voidFileFormat;
	if ( handlerFlags == 0 ) handlerFlags = &voidOptionBits;

	*filePath     = this->filePath.c_str();
	*pathLen      = this->filePath.size();
	*openFlags    = this->openFlags;
	*format       = this->format;
	*handlerFlags = this->handler->handlerFlags;
	
	return true;
	
}	// XMPFiles::GetFileInfo
 
// =================================================================================================

void
XMPFiles::SetAbortProc ( XMP_AbortProc abortProc,
						 void *        abortArg )
{
	this->abortProc = abortProc;
	this->abortArg  = abortArg;
	
	XMP_Assert ( (abortProc != (XMP_AbortProc)0) || (abortArg != (void*)0xDEADBEEF) );	// Hack to test the assert callback.
	
}	// XMPFiles::SetAbortProc
 
// =================================================================================================

bool
XMPFiles::GetXMP ( SXMPMeta *       xmpObj /* = 0 */,
                   XMP_StringPtr *  xmpPacket /* = 0 */,
                   XMP_StringLen *  xmpPacketLen /* = 0 */,
                   XMP_PacketInfo * packetInfo /* = 0 */ )
{
	if ( this->handler == 0 ) XMP_Throw ( "XMPFiles::GetXMP - No open file", kXMPErr_BadObject );
	
	if ( ! this->handler->processedXMP ) {
		try {
			this->handler->ProcessXMP();
		} catch ( ... ) {
			// Return the outputs then rethrow the exception.
			if ( xmpObj != 0 ) {
				SXMPUtils::RemoveProperties ( xmpObj, 0, 0, kXMPUtil_DoAllProperties );
				SXMPUtils::AppendProperties ( this->handler->xmpObj, xmpObj, kXMPUtil_DoAllProperties );
			}
			if ( xmpPacket != 0 ) *xmpPacket = this->handler->xmpPacket.c_str();
			if ( xmpPacketLen != 0 ) *xmpPacketLen = this->handler->xmpPacket.size();
			if ( packetInfo != 0 ) *packetInfo = this->handler->packetInfo;
			throw;
		}
	}

	if ( ! this->handler->containsXMP ) return false;

	#if 0	// *** See bug 1131815. A better way might be to pass the ref up from here.
		if ( xmpObj != 0 ) *xmpObj = this->handler->xmpObj.Clone();
	#else
		if ( xmpObj != 0 ) {
			SXMPUtils::RemoveProperties ( xmpObj, 0, 0, kXMPUtil_DoAllProperties );
			SXMPUtils::AppendProperties ( this->handler->xmpObj, xmpObj, kXMPUtil_DoAllProperties );
		}
	#endif

	if ( xmpPacket != 0 ) *xmpPacket = this->handler->xmpPacket.c_str();
	if ( xmpPacketLen != 0 ) *xmpPacketLen = this->handler->xmpPacket.size();
	if ( packetInfo != 0 ) *packetInfo = this->handler->packetInfo;

	return true;

}	// XMPFiles::GetXMP
 
// =================================================================================================

bool
XMPFiles::GetThumbnail ( XMP_ThumbnailInfo * tnailInfo )
{
	if ( this->handler == 0 ) XMP_Throw ( "XMPFiles::GetThumbnail - No open file", kXMPErr_BadObject );
	
	if ( ! (this->handler->handlerFlags & kXMPFiles_ReturnsTNail) ) return false;
	
	if ( ! this->handler->processedTNail ) this->handler->ProcessTNail();
	if ( ! this->handler->containsTNail ) return false;
	if ( tnailInfo != 0 ) *tnailInfo = this->handler->tnailInfo;

	return true;

}	// XMPFiles::GetThumbnail
 
// =================================================================================================

static bool
DoPutXMP ( XMPFiles * thiz, const SXMPMeta & xmpObj, const bool doIt )
{
	// Check some basic conditions to see if the Put should be attempted.
	
	if ( thiz->handler == 0 ) XMP_Throw ( "XMPFiles::PutXMP - No open file", kXMPErr_BadObject );
	if ( ! (thiz->openFlags & kXMPFiles_OpenForUpdate) ) {
		XMP_Throw ( "XMPFiles::PutXMP - Not open for update", kXMPErr_BadObject );
	}

	XMPFileHandler * handler      = thiz->handler;
	XMP_OptionBits   handlerFlags = handler->handlerFlags;
	XMP_PacketInfo & packetInfo   = handler->packetInfo;
	std::string &    xmpPacket    = handler->xmpPacket;
	
	if ( ! handler->processedXMP ) handler->ProcessXMP();	// Might have Open/Put with no GetXMP.
	
	size_t oldPacketOffset = (size_t)packetInfo.offset;
	size_t oldPacketLength = packetInfo.length;
	
	if ( oldPacketOffset == (size_t)kXMPFiles_UnknownOffset ) oldPacketOffset = 0;	// ! Simplify checks.
	if ( oldPacketLength == (size_t)kXMPFiles_UnknownLength ) oldPacketLength = 0;
	
	bool fileHasPacket = (oldPacketOffset != 0) && (oldPacketLength != 0);

	if ( ! fileHasPacket ) {
		if ( ! (handlerFlags & kXMPFiles_CanInjectXMP) ) {
			XMP_Throw ( "XMPFiles::PutXMP - Can't inject XMP", kXMPErr_Unavailable );
		}
		if ( handler->stdCharForm == kXMP_CharUnknown ) {
			XMP_Throw ( "XMPFiles::PutXMP - No standard character form", kXMPErr_InternalFailure );
		}
	}
	
	// Serialize the XMP and update the handler's info.
	
	XMP_Uns8 charForm = handler->stdCharForm;
	if ( charForm == kXMP_CharUnknown ) charForm = packetInfo.charForm;

	XMP_OptionBits options = kXMP_UseCompactFormat | XMP_CharToSerializeForm ( charForm );
	if ( handlerFlags & kXMPFiles_NeedsReadOnlyPacket ) options |= kXMP_ReadOnlyPacket;
	if ( fileHasPacket && (thiz->format == kXMP_UnknownFile) && (! packetInfo.writeable) ) options |= kXMP_ReadOnlyPacket;
	
	bool preferInPlace = ((handlerFlags & kXMPFiles_PrefersInPlace) != 0);
	bool tryInPlace    = (fileHasPacket & preferInPlace) || (! (handlerFlags & kXMPFiles_CanExpand));
	
	if ( handlerFlags & kXMPFiles_UsesSidecarXMP ) tryInPlace = false;
		
	if ( tryInPlace ) {
		XMP_Assert ( handler->containsXMP && (oldPacketLength == xmpPacket.size()) );
		try {
			xmpObj.SerializeToBuffer ( &xmpPacket, (options | kXMP_ExactPacketLength), oldPacketLength );
			XMP_Assert ( xmpPacket.size() == oldPacketLength );
		} catch ( ... ) {
			if ( preferInPlace ) {
				tryInPlace = false;	// ! Try again, out of place this time.
			} else {
				if ( ! doIt ) return false;
				throw;
			}
		}
	}
	
	if ( ! tryInPlace ) {
		try {
			xmpObj.SerializeToBuffer ( &xmpPacket, options );
		} catch ( ... ) {
			if ( ! doIt ) return false;
			throw;
		}
	}
	
	if ( doIt ) {
		if ( ! tryInPlace ) packetInfo.offset = kXMPFiles_UnknownOffset;
		packetInfo.length = xmpPacket.size();
		packetInfo.padSize = GetPacketPadSize ( xmpPacket.c_str(), xmpPacket.size() );
		packetInfo.charForm = charForm;
		handler->xmpObj = xmpObj.Clone();
		handler->containsXMP = true;
		handler->processedXMP = true;
		handler->needsUpdate = true;
	}
	
	return true;
	
}	// DoPutXMP
 
// =================================================================================================

void
XMPFiles::PutXMP ( const SXMPMeta & xmpObj )
{
	(void) DoPutXMP ( this, xmpObj, true );
	
}	// XMPFiles::PutXMP
 
// =================================================================================================

void
XMPFiles::PutXMP ( XMP_StringPtr xmpPacket,
                   XMP_StringLen xmpPacketLen /* = kXMP_UseNullTermination */ )
{
	SXMPMeta xmpObj ( xmpPacket, xmpPacketLen );
	this->PutXMP ( xmpObj );
	
}	// XMPFiles::PutXMP
 
// =================================================================================================

bool
XMPFiles::CanPutXMP ( const SXMPMeta & xmpObj )
{
	if ( this->handler == 0 ) XMP_Throw ( "XMPFiles::CanPutXMP - No open file", kXMPErr_BadObject );
	if ( ! (this->openFlags & kXMPFiles_OpenForUpdate) ) return false;

	if ( this->handler->handlerFlags & kXMPFiles_CanInjectXMP ) return true;
	if ( ! this->handler->containsXMP ) return false;
	if ( this->handler->handlerFlags & kXMPFiles_CanExpand ) return true;

	return DoPutXMP ( this, xmpObj, false );

}	// XMPFiles::CanPutXMP
 
// =================================================================================================

bool
XMPFiles::CanPutXMP ( XMP_StringPtr xmpPacket,
                      XMP_StringLen xmpPacketLen /* = kXMP_UseNullTermination */ )
{
	SXMPMeta xmpObj ( xmpPacket, xmpPacketLen );
	return this->CanPutXMP ( xmpObj );
	
}	// XMPFiles::CanPutXMP
 
// =================================================================================================
