// =================================================================================================
// ADOBE SYSTEMS INCORPORATED
// Copyright 2002-2007 Adobe Systems Incorporated
// All Rights Reserved
//
// NOTICE: Adobe permits you to use, modify, and distribute this file in accordance with the terms
// of the Adobe license agreement accompanying it.
// =================================================================================================

#include "TIFF_Handler.hpp"

#include "TIFF_Support.hpp"
#include "PSIR_Support.hpp"
#include "IPTC_Support.hpp"
#include "ReconcileLegacy.hpp"

#include "MD5.h"

using namespace std;

// =================================================================================================
/// \file TIFF_Handler.cpp
/// \brief File format handler for TIFF.
///
/// This handler ...
///
// =================================================================================================

// =================================================================================================
// TIFF_CheckFormat
// ================

// For TIFF we just check for the II/42 or MM/42 in the first 4 bytes and that there are at least
// 26 bytes of data (4+4+2+12+4).
//
// ! The CheckXyzFormat routines don't track the filePos, that is left to ScanXyzFile.

bool TIFF_CheckFormat ( XMP_FileFormat format,
	                    XMP_StringPtr  filePath,
                        LFA_FileRef    fileRef,
                        XMPFiles *     parent )
{
	IgnoreParam(format); IgnoreParam(filePath); IgnoreParam(parent);
	XMP_Assert ( format == kXMP_TIFFFile );
	
	enum { kMinimalTIFFSize = 4+4+2+12+4 };	// Header plus IFD with 1 entry.

	IOBuffer ioBuf;
	
	LFA_Seek ( fileRef, 0, SEEK_SET );
	if ( ! CheckFileSpace ( fileRef, &ioBuf, kMinimalTIFFSize ) ) return false;
	
	bool leTIFF = CheckBytes ( ioBuf.ptr, "\x49\x49\x2A\x00", 4 );
	bool beTIFF = CheckBytes ( ioBuf.ptr, "\x4D\x4D\x00\x2A", 4 );
	
	return (leTIFF | beTIFF);
	
}	// TIFF_CheckFormat

// =================================================================================================
// TIFF_MetaHandlerCTor
// ====================

XMPFileHandler * TIFF_MetaHandlerCTor ( XMPFiles * parent )
{
	return new TIFF_MetaHandler ( parent );

}	// TIFF_MetaHandlerCTor

// =================================================================================================
// TIFF_MetaHandler::TIFF_MetaHandler
// ==================================

TIFF_MetaHandler::TIFF_MetaHandler ( XMPFiles * _parent ) : psirMgr(0), iptcMgr(0)
{
	this->parent = _parent;
	this->handlerFlags = kTIFF_HandlerFlags;
	this->stdCharForm  = kXMP_Char8Bit;

}	// TIFF_MetaHandler::TIFF_MetaHandler

// =================================================================================================
// TIFF_MetaHandler::~TIFF_MetaHandler
// ===================================

TIFF_MetaHandler::~TIFF_MetaHandler()
{

	if ( this->psirMgr != 0 ) delete ( this->psirMgr );
	if ( this->iptcMgr != 0 ) delete ( this->iptcMgr );

}	// TIFF_MetaHandler::~TIFF_MetaHandler

// =================================================================================================
// TIFF_MetaHandler::CacheFileData
// ===============================
//
// The data caching for TIFF is easy to explain and implement, but does more processing than one
// might at first expect. This seems unavoidable given the need to close the disk file after calling
// CacheFileData. We parse the TIFF stream and cache the values for all tags of interest, and note
// whether XMP is present. We do not parse the XMP, Photoshop image resources, or IPTC datasets.

// *** This implementation simply returns when invalid TIFF is encountered. Should we throw instead?

void TIFF_MetaHandler::CacheFileData()
{
	LFA_FileRef      fileRef    = this->parent->fileRef;
	XMP_PacketInfo & packetInfo = this->packetInfo;
	
	XMP_AbortProc abortProc  = this->parent->abortProc;
	void *        abortArg   = this->parent->abortArg;
	const bool    checkAbort = (abortProc != 0);
	
	XMP_Assert ( (! this->containsXMP) && (! this->containsTNail) );
	// Set containsXMP to true here only if the XMP tag is found.
	
	if ( checkAbort && abortProc(abortArg) ) {
		XMP_Throw ( "TIFF_MetaHandler::CacheFileData - User abort", kXMPErr_UserAbort );
	}
	
	this->tiffMgr.ParseFileStream ( fileRef );
	
	TIFF_Manager::TagInfo xmpInfo;
	bool found = this->tiffMgr.GetTag ( kTIFF_PrimaryIFD, kTIFF_XMP, &xmpInfo );
	
	if ( found ) {

		this->packetInfo.offset    = this->tiffMgr.GetValueOffset ( kTIFF_PrimaryIFD, kTIFF_XMP );
		this->packetInfo.length    = xmpInfo.dataLen;
		this->packetInfo.padSize   = 0;				// Assume for now, set these properly in ProcessXMP.
		this->packetInfo.charForm  = kXMP_CharUnknown;
		this->packetInfo.writeable = true;

		this->xmpPacket.assign ( (XMP_StringPtr)xmpInfo.dataPtr, xmpInfo.dataLen );

		this->containsXMP = true;

	}
	
}	// TIFF_MetaHandler::CacheFileData

// =================================================================================================
// TIFF_MetaHandler::ProcessTNail
// ==============================
//
// Do the same processing as JPEG, even though Exif says that uncompressed images can only have 
// uncompressed thumbnails. Who know what someone might write?

// *** Should extract this code into a utility shared with the JPEG handler.

void TIFF_MetaHandler::ProcessTNail()
{
	this->processedTNail = true;	// Make sure we only come through here once.
	this->containsTNail = false;	// Set it to true after all of the info is gathered.

	this->containsTNail = this->tiffMgr.GetTNailInfo ( &this->tnailInfo );
	if ( this->containsTNail ) this->tnailInfo.fileFormat = this->parent->format;

}	// TIFF_MetaHandler::ProcessTNail

// =================================================================================================
// TIFF_MetaHandler::ProcessXMP
// ============================
//
// Process the raw XMP and legacy metadata that was previously cached. The legacy metadata in TIFF
// is messy because there are 2 copies of the IPTC and because of a Photoshop 6 bug/quirk in the way
// Exif metadata is saved.

void TIFF_MetaHandler::ProcessXMP()
{
	
	this->processedXMP = true;	// Make sure we only come through here once.

	// Set up everything for the legacy import, but don't do it yet. This lets us do a forced legacy
	// import if the XMP packet gets parsing errors.

	// Parse the IPTC and PSIR, determine the last-legacy priority. For TIFF files the relevant
	// legacy priorities (ignoring Mac pnot and ANPA resources) are:
	//	kLegacyJTP_TIFF_IPTC - highest
	//	kLegacyJTP_TIFF_TIFF_Tags
	//	kLegacyJTP_PSIR_OldCaption
	//	kLegacyJTP_PSIR_IPTC - yes, a TIFF file can have the IPTC in 2 places
	//	kLegacyJTP_None - lowest
	
	// ! Photoshop 6 wrote annoyingly wacky TIFF files. It buried a lot of the Exif metadata inside
	// ! image resource 1058, itself inside of tag 34377 in the 0th IFD. Take care of this before
	// ! doing any of the legacy metadata presence or priority analysis. Delete image resource 1058
	// ! to get rid of the buried Exif, but don't mark the XMPFiles object as changed. This change
	// ! should not trigger an update, but should be included as part of a normal update.
	
	bool found;
	RecJTP_LegacyPriority lastLegacy = kLegacyJTP_None;

	bool readOnly = ((this->parent->openFlags & kXMPFiles_OpenForUpdate) == 0);

	if ( readOnly ) {
		this->psirMgr = new PSIR_MemoryReader();
		this->iptcMgr = new IPTC_Reader();
	} else {
		this->psirMgr = new PSIR_FileWriter();
		this->iptcMgr = new IPTC_Writer();
	}

	TIFF_Manager & tiff = this->tiffMgr;	// Give the compiler help in recognizing non-aliases.
	PSIR_Manager & psir = *this->psirMgr;
	IPTC_Manager & iptc = *this->iptcMgr;
	
	TIFF_Manager::TagInfo psirInfo;
	bool havePSIR = tiff.GetTag ( kTIFF_PrimaryIFD, kTIFF_PSIR, &psirInfo );
	
	TIFF_Manager::TagInfo iptcInfo;
	bool haveIPTC = tiff.GetTag ( kTIFF_PrimaryIFD, kTIFF_IPTC, &iptcInfo );	// The TIFF IPTC tag.
		
	if ( havePSIR ) {	// ! Do the Photoshop 6 integration before other legacy analysis.
		psir.ParseMemoryResources ( psirInfo.dataPtr, psirInfo.dataLen );
		PSIR_Manager::ImgRsrcInfo buriedExif;
		found = psir.GetImgRsrc ( kPSIR_Exif, &buriedExif );
		if ( found ) {
			tiff.IntegrateFromPShop6 ( buriedExif.dataPtr, buriedExif.dataLen );
			if ( ! readOnly ) psir.DeleteImgRsrc ( kPSIR_Exif );
		}
	}
		
	if ( haveIPTC ) {	// At this point "haveIPTC" means from TIFF tag 33723.
		iptc.ParseMemoryDataSets ( iptcInfo.dataPtr, iptcInfo.dataLen );
		lastLegacy = kLegacyJTP_TIFF_IPTC;
	}
	
	if ( lastLegacy < kLegacyJTP_TIFF_TIFF_Tags ) {
		found = tiff.GetTag ( kTIFF_PrimaryIFD, kTIFF_ImageDescription, 0 );
		if ( ! found ) found = tiff.GetTag ( kTIFF_PrimaryIFD, kTIFF_Artist, 0 );
		if ( ! found ) found = tiff.GetTag ( kTIFF_PrimaryIFD, kTIFF_Copyright, 0 );
		if ( found ) lastLegacy = kLegacyJTP_TIFF_TIFF_Tags;
	}
	
	if ( havePSIR ) {

		if ( lastLegacy < kLegacyJTP_PSIR_OldCaption ) {
			found = psir.GetImgRsrc ( kPSIR_OldCaption, 0 );
			if ( ! found ) found = psir.GetImgRsrc ( kPSIR_OldCaptionPStr, 0 );
			if ( found ) lastLegacy = kLegacyJTP_PSIR_OldCaption;
		}

		if ( ! haveIPTC ) {
			PSIR_Manager::ImgRsrcInfo iptcInfo;
			haveIPTC = psir.GetImgRsrc ( kPSIR_IPTC, &iptcInfo );
			if ( haveIPTC ) {
				iptc.ParseMemoryDataSets ( iptcInfo.dataPtr, iptcInfo.dataLen );
				if ( lastLegacy < kLegacyJTP_PSIR_IPTC ) lastLegacy = kLegacyJTP_PSIR_IPTC;
			}
		}

	}
		
	XMP_OptionBits options = k2XMP_FileHadExif;	// TIFF files are presumed to have Exif legacy.
	if ( this->containsXMP ) options |= k2XMP_FileHadXMP;
	if ( haveIPTC || (lastLegacy == kLegacyJTP_PSIR_OldCaption) ) options |= k2XMP_FileHadIPTC;

	// Process the XMP packet. If it fails to parse, do a forced legacy import but still throw an
	// exception. This tells the caller that an error happened, but gives them recovered legacy
	// should they want to proceed with that.

	if ( ! this->xmpPacket.empty() ) {
		XMP_Assert ( this->containsXMP );
		// Common code takes care of packetInfo.charForm, .padSize, and .writeable.
		XMP_StringPtr packetStr = this->xmpPacket.c_str();
		XMP_StringLen packetLen = this->xmpPacket.size();
		try {
			this->xmpObj.ParseFromBuffer ( packetStr, packetLen );
		} catch ( ... ) {
			XMP_ClearOption ( options, k2XMP_FileHadXMP );
			ImportJTPtoXMP ( kXMP_TIFFFile, lastLegacy, &tiff, psir, &iptc, &this->xmpObj, options );
			throw;	// ! Rethrow the exception, don't absorb it.
		}
	}

	// Process the legacy metadata.

	ImportJTPtoXMP ( kXMP_TIFFFile, lastLegacy, &tiff, psir, &iptc, &this->xmpObj, options );
	this->containsXMP = true;	// Assume we now have something in the XMP.

}	// TIFF_MetaHandler::ProcessXMP

// =================================================================================================
// TIFF_MetaHandler::UpdateFile
// ============================
//
// There is very little to do directly in UpdateFile. ExportXMPtoJTP takes care of setting all of
// the necessary TIFF tags, including things like the 2nd copy of the IPTC in the Photoshop image
// resources in tag 34377. TIFF_FileWriter::UpdateFileStream does all of the update-by-append I/O.

// *** Need to pass the abort proc and arg to TIFF_FileWriter::UpdateFileStream.

void TIFF_MetaHandler::UpdateFile ( bool doSafeUpdate )
{
	XMP_Assert ( ! doSafeUpdate );	// This should only be called for "unsafe" updates.

	LFA_FileRef   destRef    = this->parent->fileRef;
	XMP_AbortProc abortProc  = this->parent->abortProc;
	void *        abortArg   = this->parent->abortArg;

	// Decide whether to do an in-place update. This can only happen if all of the following are true:
	//	- There is an XMP packet in the file.
	//	- The are no changes to the legacy tags. (The IPTC and PSIR are in the TIFF tags.)
	//	- The new XMP can fit in the old space.
	
	ExportXMPtoJTP ( kXMP_TIFFFile, &this->xmpObj, &this->tiffMgr, this->psirMgr, this->iptcMgr );
	
	XMP_Int64 oldPacketOffset = this->packetInfo.offset;
	XMP_Int32 oldPacketLength = this->packetInfo.length;
	
	if ( oldPacketOffset == kXMPFiles_UnknownOffset ) oldPacketOffset = 0;	// ! Simplify checks.
	if ( oldPacketLength == kXMPFiles_UnknownLength ) oldPacketLength = 0;
	
	bool doInPlace = (oldPacketOffset != 0) && (oldPacketLength != 0);	// ! Has old packet and new packet fits.
	if ( doInPlace && (this->tiffMgr.IsLegacyChanged()) ) doInPlace = false;

	if ( doInPlace ) {

		#if GatherPerformanceData
			sAPIPerf->back().extraInfo += ", TIFF in-place update";
		#endif
	
		LFA_FileRef liveFile = this->parent->fileRef;
	
		XMP_Assert ( this->xmpPacket.size() == (size_t)oldPacketLength );	// ! Done by common PutXMP logic.
		
		LFA_Seek ( liveFile, oldPacketOffset, SEEK_SET );
		LFA_Write ( liveFile, this->xmpPacket.c_str(), this->xmpPacket.size() );
	
	} else {

		#if GatherPerformanceData
			sAPIPerf->back().extraInfo += ", TIFF append update";
		#endif
	
		// Reserialize the XMP to get standard padding, PutXMP has probably done an in-place serialize.
		this->xmpObj.SerializeToBuffer ( &this->xmpPacket, kXMP_UseCompactFormat );
		this->packetInfo.offset = kXMPFiles_UnknownOffset;
		this->packetInfo.length = this->xmpPacket.size();
		this->packetInfo.padSize = GetPacketPadSize ( this->xmpPacket.c_str(), this->xmpPacket.size() );
	
		this->tiffMgr.SetTag ( kTIFF_PrimaryIFD, kTIFF_XMP, kTIFF_UndefinedType, this->xmpPacket.size(), this->xmpPacket.c_str() );
		
		this->tiffMgr.UpdateFileStream ( destRef );
	
	}

	this->needsUpdate = false;

}	// TIFF_MetaHandler::UpdateFile

// =================================================================================================
// TIFF_MetaHandler::WriteFile
// ===========================
//
// The structure of TIFF makes it hard to do a sequential source-to-dest copy with interleaved
// updates. So, copy the existing source to the destination and call UpdateFile.

void TIFF_MetaHandler::WriteFile ( LFA_FileRef sourceRef, const std::string & sourcePath )
{
	LFA_FileRef   destRef    = this->parent->fileRef;
	XMP_AbortProc abortProc  = this->parent->abortProc;
	void *        abortArg   = this->parent->abortArg;
	
	XMP_Int64 fileLen = LFA_Measure ( sourceRef );
	if ( fileLen > 0xFFFFFFFFLL ) {	// Check before making a copy of the file.
		XMP_Throw ( "TIFF fles can't exceed 4GB", kXMPErr_BadTIFF );
	}
	
	LFA_Seek ( sourceRef, 0, SEEK_SET );
	LFA_Truncate ( destRef, 0 );
	LFA_Copy ( sourceRef, destRef, fileLen, abortProc, abortArg );

	this->UpdateFile ( false );

}	// TIFF_MetaHandler::WriteFile

// =================================================================================================
