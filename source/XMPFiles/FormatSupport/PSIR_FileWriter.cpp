// =================================================================================================
// ADOBE SYSTEMS INCORPORATED
// Copyright 2006-2007 Adobe Systems Incorporated
// All Rights Reserved
//
// NOTICE: Adobe permits you to use, modify, and distribute this file in accordance with the terms
// of the Adobe license agreement accompanying it.
// =================================================================================================

#include "PSIR_Support.hpp"
#include "EndianUtils.hpp"

// =================================================================================================
/// \file PSIR_FileWriter.cpp
/// \brief Implementation of the file-based or read-write form of PSIR_Manager.
// =================================================================================================

// =================================================================================================
// IsMetadataImgRsrc
// =================
//
// The only image resources of possible interest as metadata have type '8BIM' and IDs:
//    1008, 1020, 1028, 1034, 1035, 1036, 1058, 1060, 1061

static inline bool IsMetadataImgRsrc ( XMP_Uns16 id )
{

	if ( (id < 1008) || (id > 1061) ) {
		return false;
	} else if ( id >= 1058 ) {
		if ( id == 1059 ) return false;
	} else if ( id > 1036 ) {
		return false;
	} else if ( id > 1028 ) {
		if ( id < 1034 ) return false;
	} else if ( id < 1028 ) {
		if ( (id != 1008) && (id != 1020) ) return false;
	}

	return true;

}	// IsMetadataImgRsrc

// =================================================================================================
// PSIR_FileWriter::DeleteExistingInfo
// ===================================
//
// Delete all existing info about image resources.

void PSIR_FileWriter::DeleteExistingInfo()
{
	XMP_Assert ( ! (this->memParsed && this->fileParsed) );

	if ( this->memParsed ) {
		if ( this->ownedContent ) free ( this->memContent );
	} else {
		InternalRsrcMap::iterator irPos = this->imgRsrcs.begin();
		InternalRsrcMap::iterator irEnd = this->imgRsrcs.end();
		for ( ; irPos != irEnd; ++irPos ) irPos->second.changed = true;	// Fool the InternalRsrcInfo destructor.
	}

	this->imgRsrcs.clear();

	this->memContent = 0;
	this->memLength  = 0;

	this->changed  = false;
	this->memParsed = false;
	this->fileParsed = false;
	this->ownedContent = false;

}	// PSIR_FileWriter::DeleteExistingInfo

// =================================================================================================
// PSIR_FileWriter::~PSIR_FileWriter
// =================================
//
// The InternalRsrcInfo destructor will deallocate the data for changed image resources. It does not
// know whether they are memory-parsed or file-parsed though, so it won't deallocate captured but
// unchanged file-parsed resources. Mark those as changed to make the destructor deallocate them.

PSIR_FileWriter::~PSIR_FileWriter()
{
	XMP_Assert ( ! (this->memParsed && this->fileParsed) );

	if ( this->ownedContent ) {
		XMP_Assert ( this->memContent != 0 );
		free ( this->memContent );
	}
	
	if ( this->fileParsed ) {
		InternalRsrcMap::iterator irPos = this->imgRsrcs.begin();
		InternalRsrcMap::iterator irEnd = this->imgRsrcs.end();
		for ( ; irPos != irEnd; ++irPos ) {
			if ( irPos->second.dataPtr != 0 ) irPos->second.changed = true;
		}
	}
	

}	// PSIR_FileWriter::~PSIR_FileWriter

// =================================================================================================
// PSIR_FileWriter::GetImgRsrc
// ===========================

bool PSIR_FileWriter::GetImgRsrc ( XMP_Uns16 id, ImgRsrcInfo* info ) const
{
	InternalRsrcMap::const_iterator rsrcPos = this->imgRsrcs.find ( id );
	if ( rsrcPos == this->imgRsrcs.end() ) return false;
	
	const InternalRsrcInfo & rsrcInfo = rsrcPos->second;
	
	if ( info != 0 ) {
		info->id = rsrcInfo.id;
		info->dataLen = rsrcInfo.dataLen;
		info->dataPtr = rsrcInfo.dataPtr;
		info->origOffset = rsrcInfo.origOffset;
	}

	return true;
	
}	// PSIR_FileWriter::GetImgRsrc

// =================================================================================================
// PSIR_FileWriter::SetImgRsrc
// ===========================

void PSIR_FileWriter::SetImgRsrc ( XMP_Uns16 id, const void* clientPtr, XMP_Uns32 length )
{
	InternalRsrcMap::iterator rsrcPos = this->imgRsrcs.find ( id );
	
	if ( (rsrcPos != this->imgRsrcs.end()) &&
		 (length == rsrcPos->second.dataLen) &&
		 (memcmp ( rsrcPos->second.dataPtr, clientPtr, length ) == 0) ) {
		return;	// ! The value is unchanged, exit.
	}
	
	void* dataPtr = malloc ( length );
	if ( dataPtr == 0 ) XMP_Throw ( "Out of memory", kXMPErr_NoMemory );
	memcpy ( dataPtr, clientPtr, length );	// AUDIT: Safe, malloc'ed length bytes above.

	if ( rsrcPos == this->imgRsrcs.end() ) {

		// This resource is not yet in the map, create the map entry.
		InternalRsrcInfo newRsrc ( id, length, dataPtr, (XMP_Uns32)(-1) );
		newRsrc.changed = true;
		this->imgRsrcs[id] = newRsrc;

	} else {
	
		// This resource is in the map, update the existing map entry.
		InternalRsrcInfo* rsrcInfo = &rsrcPos->second;
		if ( rsrcInfo->changed && (rsrcInfo->dataPtr != 0) ) free ( rsrcInfo->dataPtr );
		rsrcInfo->dataPtr = dataPtr;
		rsrcInfo->dataLen = length;
		rsrcInfo->changed = true;
	
	}

	this->changed = true;

}	// PSIR_FileWriter::SetImgRsrc

// =================================================================================================
// PSIR_FileWriter::DeleteImgRsrc
// ==============================

void PSIR_FileWriter::DeleteImgRsrc ( XMP_Uns16 id )
{
	InternalRsrcMap::iterator rsrcPos = this->imgRsrcs.find ( id );
	if ( rsrcPos == this->imgRsrcs.end() ) return;	// Nothing to delete.
	
	this->imgRsrcs.erase ( id );
	this->changed = true;

}	// PSIR_FileWriter::DeleteImgRsrc

// =================================================================================================
// PSIR_FileWriter::IsLegacyChanged
// ================================

bool PSIR_FileWriter::IsLegacyChanged()
{

	if ( ! this->changed ) return false;
	
	InternalRsrcMap::iterator irPos = this->imgRsrcs.begin();
	InternalRsrcMap::iterator irEnd = this->imgRsrcs.end();

	for ( ; irPos != irEnd; ++irPos ) {
		const InternalRsrcInfo & rsrcInfo = irPos->second;
		if ( rsrcInfo.changed && (rsrcInfo.id != kPSIR_XMP) ) return true;
	}

	return false;	// Can get here if the XMP is the only thing changed.
	
}	// PSIR_FileWriter::IsLegacyChanged

// =================================================================================================
// PSIR_FileWriter::ParseMemoryResources
// =====================================

void PSIR_FileWriter::ParseMemoryResources ( const void* data, XMP_Uns32 length, bool copyData /* = true */ )
{
	this->DeleteExistingInfo();
	this->memParsed = true;
	if ( length == 0 ) return;

	// Allocate space for the full in-memory data and copy it.
	
	if ( ! copyData ) {
		this->memContent = (XMP_Uns8*) data;
		XMP_Assert ( ! this->ownedContent );
	} else {
		if ( length > 100*1024*1024 ) XMP_Throw ( "Outrageous length for memory-based PSIR", kXMPErr_BadPSIR );
		this->memContent = (XMP_Uns8*) malloc ( length );
		if ( this->memContent == 0 ) XMP_Throw ( "Out of memory", kXMPErr_NoMemory );
		memcpy ( this->memContent, data, length );	// AUDIT: Safe, malloc'ed length bytes above.
		this->ownedContent = true;
	}
	this->memLength = length;
	
	// Capture the info for all of the resources.
	
	XMP_Uns8* psirPtr   = this->memContent;
	XMP_Uns8* psirEnd   = psirPtr + length;
	XMP_Uns8* psirLimit = psirEnd - kMinImgRsrcSize;
	
	while ( psirPtr <= psirLimit ) {
	
		XMP_Uns8* origin = psirPtr;	// The beginning of this resource.
		XMP_Uns32 type = GetUns32BE(psirPtr);
		XMP_Uns16 id = GetUns16BE(psirPtr+4);
		psirPtr += 6;	// Advance to the resource name.

		XMP_Uns8* namePtr = psirPtr;
		XMP_Uns16 nameLen = namePtr[0];			// ! The length for the Pascal string, w/ room for "+2".
		psirPtr += ((nameLen + 2) & 0xFFFE);	// ! Round up to an even offset. Yes, +2!
		
		if ( psirPtr > psirEnd-4 ) break;	// Bad image resource. Throw instead?

		XMP_Uns32 dataLen = GetUns32BE(psirPtr);
		psirPtr += 4;	// Advance to the resource data.

		XMP_Uns32 dataOffset = psirPtr - this->memContent;
		XMP_Uns8* nextRsrc   = psirPtr + ((dataLen + 1) & 0xFFFFFFFEUL);	// ! Round up to an even offset.
		
		if ( (dataLen > length) || (psirPtr > psirEnd-dataLen) ) break;	// Bad image resource. Throw instead?
		
		if ( type == k8BIM ) {
			InternalRsrcInfo newRsrc ( id, dataLen, psirPtr, dataOffset );
			this->imgRsrcs[id] = newRsrc;
			if ( nameLen != 0 ) this->imgRsrcs[id].rsrcName = namePtr;
		} else {
			XMP_Uns32 rsrcOffset = origin - this->memContent;
			XMP_Uns32 rsrcLength = nextRsrc - origin;	// Includes trailing pad.
			XMP_Assert ( (rsrcLength & 1) == 0 );
			this->otherRsrcs.push_back ( OtherRsrcInfo ( rsrcOffset, rsrcLength ) );
		}
		
		psirPtr = nextRsrc;
	
	}

}	// PSIR_FileWriter::ParseMemoryResources

// =================================================================================================
// PSIR_FileWriter::ParseFileResources
// ===================================

void PSIR_FileWriter::ParseFileResources ( LFA_FileRef fileRef, XMP_Uns32 length )
{
	bool ok;
	
	this->DeleteExistingInfo();
	this->fileParsed = true;
	if ( length == 0 ) return;
	
	// Parse the image resource block.

	IOBuffer ioBuf;
	ioBuf.filePos = LFA_Seek ( fileRef, 0, SEEK_CUR );
	
	XMP_Int64 psirOrigin = ioBuf.filePos;	// Need this to determine the resource data offsets.
	
	XMP_Int64 fileEnd = ioBuf.filePos + length;
	
	while ( (ioBuf.filePos + (ioBuf.ptr - ioBuf.data)) < fileEnd ) {
	
		ok = CheckFileSpace ( fileRef, &ioBuf, 12 );	// The minimal image resource takes 12 bytes.
		if ( ! ok ) break;	// Bad image resource. Throw instead?

		XMP_Int64 thisRsrcPos = ioBuf.filePos + (ioBuf.ptr - ioBuf.data);
	
		XMP_Uns32 type = GetUns32BE(ioBuf.ptr);
		XMP_Uns16 id   = GetUns16BE(ioBuf.ptr+4);
		ioBuf.ptr += 6;	// Advance to the resource name.

		XMP_Uns16 nameLen = ioBuf.ptr[0];	// ! The length for the Pascal string, w/ room for "+2".
		nameLen = (nameLen + 2) & 0xFFFE;	// ! Round up to an even total. Yes, +2!
		ok = CheckFileSpace ( fileRef, &ioBuf, nameLen+4 );	// Get the name and data length.
		if ( ! ok ) break;	// Bad image resource. Throw instead?

		ioBuf.ptr += nameLen;	// Move to the data length.
		XMP_Uns32 dataLen   = GetUns32BE(ioBuf.ptr);
		XMP_Uns32 dataTotal = ((dataLen + 1) & 0xFFFFFFFEUL);	// Round up to an even total.
		ioBuf.ptr += 4;	// Advance to the resource data.

		XMP_Int64 thisDataPos = ioBuf.filePos + (ioBuf.ptr - ioBuf.data);
		XMP_Int64 nextRsrcPos = thisDataPos + dataTotal;

		if ( type != k8BIM ) {
			XMP_Uns32 fullRsrcLen = (XMP_Uns32) (nextRsrcPos - thisRsrcPos);
			this->otherRsrcs.push_back ( OtherRsrcInfo ( (XMP_Uns32)thisRsrcPos, fullRsrcLen ) );
			MoveToOffset ( fileRef, nextRsrcPos, &ioBuf );
			continue;
		}

		InternalRsrcInfo newRsrc ( id, dataLen, 0, (XMP_Uns32)thisDataPos );
		this->imgRsrcs[id] = newRsrc;
		
		if ( ! IsMetadataImgRsrc ( id ) ) {
			MoveToOffset ( fileRef, nextRsrcPos, &ioBuf );
			continue;
		}

		newRsrc.dataPtr = malloc ( dataLen );
		if ( newRsrc.dataPtr == 0 ) XMP_Throw ( "Out of memory", kXMPErr_NoMemory );
		
		try {
			
			if ( dataTotal <= kIOBufferSize ) {
				// The image resource data fits within the I/O buffer.
				ok = CheckFileSpace ( fileRef, &ioBuf, dataTotal );
				if ( ! ok ) break;	// Bad image resource. Throw instead?
				memcpy ( (void*)newRsrc.dataPtr, ioBuf.ptr, dataLen );	// AUDIT: Safe, malloc'ed dataLen bytes above.
				ioBuf.ptr += dataTotal;	// ! Add the rounded length.
			} else {
				// The image resource data is bigger than the I/O buffer.
				LFA_Seek ( fileRef, (ioBuf.filePos + (ioBuf.ptr - ioBuf.data)), SEEK_SET );
				LFA_Read ( fileRef, (void*)newRsrc.dataPtr, dataLen );
				FillBuffer ( fileRef, nextRsrcPos, &ioBuf );
			}
			
			this->imgRsrcs[id].dataPtr = newRsrc.dataPtr;

		} catch ( ... ) {

			free ( (void*)newRsrc.dataPtr );
			throw;

		}
		
	}
	
	#if 0
	{
		printf ( "\nPSIR_FileWriter::ParseFileResources, count = %d\n", this->imgRsrcs.size() );
		InternalRsrcMap::iterator irPos = this->imgRsrcs.begin();
		InternalRsrcMap::iterator irEnd = this->imgRsrcs.end();
		for ( ; irPos != irEnd; ++irPos ) {
			InternalRsrcInfo& thisRsrc = irPos->second;
			printf ( "  #%d, dataLen %d, origOffset %d (0x%X)%s\n",
					 thisRsrc.id, thisRsrc.dataLen, thisRsrc.origOffset, thisRsrc.origOffset,
					 (thisRsrc.changed ? ", changed" : "") );
		}
	}
	#endif
	
}	// PSIR_FileWriter::ParseFileResources

// =================================================================================================
// PSIR_FileWriter::UpdateMemoryResources
// ======================================

XMP_Uns32 PSIR_FileWriter::UpdateMemoryResources ( void** dataPtr )
{
	if ( this->fileParsed ) XMP_Throw ( "Not memory based", kXMPErr_EnforceFailure );
	
	// Compute the size and allocate the new image resource block.
	
	XMP_Uns32 newLength = 0;

	InternalRsrcMap::iterator irPos = this->imgRsrcs.begin();
	InternalRsrcMap::iterator irEnd = this->imgRsrcs.end();

	for ( ; irPos != irEnd; ++irPos ) {	// Add in the lengths for the 8BIM resources.
		const InternalRsrcInfo & rsrcInfo = irPos->second;
		newLength += 10;
		newLength += ((rsrcInfo.dataLen + 1) & 0xFFFFFFFEUL);
		if ( rsrcInfo.rsrcName == 0 ) {
			newLength += 2;
		} else {
			XMP_Uns32 nameLen = rsrcInfo.rsrcName[0];
			newLength += ((nameLen + 2) & 0xFFFFFFFEUL);	// ! Yes, +2.
		}
	}
	
	for ( size_t i = 0; i < this->otherRsrcs.size(); ++i ) {	// Add in the non-8BIM resources.
		newLength += this->otherRsrcs[i].rsrcLength;
	}
	
	XMP_Uns8* newContent = (XMP_Uns8*) malloc ( newLength );
	if ( newContent == 0 ) XMP_Throw ( "Out of memory", kXMPErr_NoMemory );
	
	// Fill in the new image resource block.
	
	XMP_Uns8* rsrcPtr = newContent;

	for ( irPos = this->imgRsrcs.begin(); irPos != irEnd; ++irPos ) {	// Do the 8BIM resources.

		const InternalRsrcInfo & rsrcInfo = irPos->second;
		
		PutUns32BE ( k8BIM, rsrcPtr );
		rsrcPtr += 4;
		PutUns16BE ( rsrcInfo.id, rsrcPtr );
		rsrcPtr += 2;
		
		if ( rsrcInfo.rsrcName == 0 ) {
			PutUns16BE ( 0, rsrcPtr );
			rsrcPtr += 2;
		} else {
			XMP_Uns32 nameLen = rsrcInfo.rsrcName[0];
			if ( (nameLen+1) > (newLength - (rsrcPtr - newContent)) ) {
				XMP_Throw ( "Buffer overrun", kXMPErr_InternalFailure );
			}
			memcpy ( rsrcPtr, rsrcInfo.rsrcName, nameLen+1 );	// AUDIT: Protected by the above check.
			rsrcPtr += nameLen+1;
			if ( (nameLen & 1) == 0 ) {
				*rsrcPtr = 0;
				++rsrcPtr;
			}
		}
		
		PutUns32BE ( rsrcInfo.dataLen, rsrcPtr );
		rsrcPtr += 4;
		if ( rsrcInfo.dataLen > (newLength - (rsrcPtr - newContent)) ) {
			XMP_Throw ( "Buffer overrun", kXMPErr_InternalFailure );
		}
		memcpy ( rsrcPtr, rsrcInfo.dataPtr, rsrcInfo.dataLen );	// AUDIT: Protected by the above check.
		rsrcPtr += rsrcInfo.dataLen;
		if ( (rsrcInfo.dataLen & 1) != 0 ) {	// Pad to an even length if necessary.
			*rsrcPtr = 0;
			++rsrcPtr;
		}

	}

	for ( size_t i = 0; i < this->otherRsrcs.size(); ++i ) {	// Do the non-8BIM resources.
		XMP_Uns8* srcPtr = this->memContent + this->otherRsrcs[i].rsrcOffset;
		XMP_Uns32 srcLen = this->otherRsrcs[i].rsrcLength;
		if ( srcLen > (newLength - (rsrcPtr - newContent)) ) {
			XMP_Throw ( "Buffer overrun", kXMPErr_InternalFailure );
		}
		memcpy ( rsrcPtr, srcPtr, srcLen );	// AUDIT: Protected by the above check.
		rsrcPtr += srcLen;	// No need to pad, included in the original resource length.
	}
	
	XMP_Assert ( rsrcPtr == (newContent + newLength) );
	
	// Parse the rebuilt image resource block. This is the easiest way to reconstruct the map.
	
	this->ParseMemoryResources ( newContent, newLength, false );
	this->ownedContent = true;	// ! We really do own it.
	
	if ( dataPtr != 0 ) *dataPtr = newContent;
	return newLength;
	
}	// PSIR_FileWriter::UpdateMemoryResources

// =================================================================================================
// PSIR_FileWriter::UpdateFileResources
// ====================================

XMP_Uns32 PSIR_FileWriter::UpdateFileResources ( LFA_FileRef sourceRef, LFA_FileRef destRef,
												 IOBuffer * ioBuf, XMP_AbortProc abortProc, void * abortArg )
{
	IgnoreParam(ioBuf);
	
	const bool checkAbort = (abortProc != 0);
	
	struct RsrcHeader {
		XMP_Uns32 type;
		XMP_Uns16 id;
		XMP_Uns16 name;
		XMP_Uns32 dataLen;
	};
	XMP_Assert ( sizeof(RsrcHeader) == 12 );
	
	if ( this->memParsed ) XMP_Throw ( "Not file based", kXMPErr_EnforceFailure );
	
	XMP_Int64 destLenOffset = LFA_Seek ( destRef, 0, SEEK_CUR );
	XMP_Uns32 destLength = 0;
	
	LFA_Write ( destRef, &destLength, 4 );	// Write a placeholder for the new PSIR section length.

	#if 0
	{
		printf ( "\nPSIR_FileWriter::UpdateFileResources, count = %d\n", this->imgRsrcs.size() );
		InternalRsrcMap::iterator irPos = this->imgRsrcs.begin();
		InternalRsrcMap::iterator irEnd = this->imgRsrcs.end();
		for ( ; irPos != irEnd; ++irPos ) {
			InternalRsrcInfo& thisRsrc = irPos->second;
			printf ( "  #%d, dataLen %d, origOffset %d (0x%X)%s\n",
					 thisRsrc.id, thisRsrc.dataLen, thisRsrc.origOffset, thisRsrc.origOffset,
					 (thisRsrc.changed ? ", changed" : "") );
		}
	}
	#endif
	
	// First write all of the '8BIM' resources from the map. Use the internal data if present, else
	// copy the data from the file. We don't preserve names for these resources, but then Photoshop
	// itself tosses out all resource names and has no plans to ever support them.

	RsrcHeader outRsrc;
	outRsrc.type  = MakeUns32BE ( k8BIM );
	outRsrc.name = 0;

	InternalRsrcMap::iterator rsrcPos = this->imgRsrcs.begin();
	InternalRsrcMap::iterator rsrcEnd = this->imgRsrcs.end();

	// printf ( "\nPSIR_FileWriter::UpdateFileResources - 8BIM resources\n" );
	for ( ; rsrcPos != rsrcEnd; ++rsrcPos ) {

		InternalRsrcInfo& currRsrc = rsrcPos->second;
		
		outRsrc.id = MakeUns16BE ( currRsrc.id );
		outRsrc.dataLen = MakeUns32BE ( currRsrc.dataLen );
		LFA_Write ( destRef, &outRsrc, sizeof(RsrcHeader) );
		// printf ( "  #%d, offset %d (0x%X), dataLen %d\n", currRsrc.id, destLength, destLength, currRsrc.dataLen );

		if ( currRsrc.dataPtr != 0 ) {
			LFA_Write ( destRef, currRsrc.dataPtr, currRsrc.dataLen );
		} else {
			LFA_Seek ( sourceRef, currRsrc.origOffset, SEEK_SET );
			LFA_Copy ( sourceRef, destRef, currRsrc.dataLen );
		}
		
		destLength += sizeof(RsrcHeader) + currRsrc.dataLen;
		
		if ( (currRsrc.dataLen & 1) != 0 ) {
			LFA_Write ( destRef, &outRsrc.name, 1 );	// ! The name contains zero.
			++destLength;
		}

	}
	
	// Now write all of the non-8BIM resources. Copy the entire resource chunk from the source file.
	
	// printf ( "\nPSIR_FileWriter::UpdateFileResources - other resources\n" );
	for ( size_t i = 0; i < this->otherRsrcs.size(); ++i ) {
		// printf ( "  offset %d (0x%X), length %d",
		//		 this->otherRsrcs[i].rsrcOffset, this->otherRsrcs[i].rsrcOffset, this->otherRsrcs[i].rsrcLength );
		LFA_Seek ( sourceRef, this->otherRsrcs[i].rsrcOffset, SEEK_SET );
		LFA_Copy ( sourceRef, destRef, this->otherRsrcs[i].rsrcLength );
		destLength += this->otherRsrcs[i].rsrcLength;	// Alignment padding is already included.
	}

	// Write the final PSIR section length, seek back to the end of the file, return the length.

	// printf ( "\nPSIR_FileWriter::UpdateFileResources - final length %d (0x%X)\n", destLength, destLength );
	LFA_Seek ( destRef, destLenOffset, SEEK_SET );
	XMP_Uns32 outLen = MakeUns32BE ( destLength );
	LFA_Write ( destRef, &outLen, 4 );
	LFA_Seek ( destRef, 0, SEEK_END );
	
	// *** Not rebuilding the internal map - turns out we never want it, why pay for the I/O.
	// *** Should probably add an option for all of these cases, memory and file based.

	return destLength;

}	// PSIR_FileWriter::UpdateFileResources
