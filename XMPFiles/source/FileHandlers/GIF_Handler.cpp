// =================================================================================================
// ADOBE SYSTEMS INCORPORATED
// Copyright 2002-2007 Adobe Systems Incorporated
// All Rights Reserved
//
// NOTICE: Adobe permits you to use, modify, and distribute this file in accordance with the terms
// of the Adobe license agreement accompanying it.
//
// Derived from PNG_Handler.cpp by Ian Jacobi
// =================================================================================================

#include "GIF_Handler.hpp"

#include "source/XIO.hpp"
#include "XMPFiles/source/FormatSupport/GIF_Support.hpp"

using namespace std;

// =================================================================================================
/// \file GIF_Handler.hpp
/// \brief File format handler for GIF.
///
/// This handler ...
///
// =================================================================================================

// =================================================================================================
// GIF_MetaHandlerCTor
// ====================

XMPFileHandler * GIF_MetaHandlerCTor ( XMPFiles * parent )
{
	return new GIF_MetaHandler ( parent );

}	// GIF_MetaHandlerCTor

// =================================================================================================
// GIF_CheckFormat
// ===============

bool GIF_CheckFormat ( XMP_FileFormat format,
					   XMP_StringPtr  filePath,
                       XMP_IO*    fileRef,
                       XMPFiles *     parent )
{
	IgnoreParam(format); IgnoreParam(fileRef); IgnoreParam(parent);
	XMP_Assert ( format == kXMP_GIFFile );

	IOBuffer ioBuf;

	fileRef->Rewind();
	if ( ! CheckFileSpace ( fileRef, &ioBuf, GIF_SIGNATURE_LEN ) ) return false;	// We need at least 3, so the buffer is not filled.

	if ( ! CheckBytes ( ioBuf.ptr, GIF_SIGNATURE_DATA, GIF_SIGNATURE_LEN ) ) return false;

	return true;

}	// GIF_CheckFormat

// =================================================================================================
// GIF_MetaHandler::GIF_MetaHandler
// ==================================

GIF_MetaHandler::GIF_MetaHandler ( XMPFiles * _parent )
{
	this->parent = _parent;
	this->handlerFlags = kGIF_HandlerFlags;
	// It MUST be UTF-8.
	this->stdCharForm  = kXMP_Char8Bit;

}

// =================================================================================================
// GIF_MetaHandler::~GIF_MetaHandler
// ===================================

GIF_MetaHandler::~GIF_MetaHandler()
{
}

// =================================================================================================
// GIF_MetaHandler::CacheFileData
// ===============================

void GIF_MetaHandler::CacheFileData()
{

	this->containsXMP = false;

	XMP_IO* fileRef ( this->parent->ioRef );
	if ( fileRef == 0) return;

	// We try to navigate through the blocks to find the XMP block.
	GIF_Support::BlockState blockState;
	long numBlocks = GIF_Support::OpenGIF ( fileRef, blockState );
	if ( numBlocks == 0 ) return;

	if (blockState.xmpLen != 0)
	{
		// XMP present

		this->xmpPacket.reserve(blockState.xmpLen);
		this->xmpPacket.assign(blockState.xmpLen, ' ');

		if (GIF_Support::ReadBuffer ( fileRef, blockState.xmpPos, blockState.xmpLen, const_cast<char *>(this->xmpPacket.data()) ))
		{
			this->packetInfo.offset = blockState.xmpPos;
			this->packetInfo.length = blockState.xmpLen;
			this->containsXMP = true;
		}
	}
	else
	{
		// no XMP
	}

}	// GIF_MetaHandler::CacheFileData

// =================================================================================================
// GIF_MetaHandler::ProcessTNail
// ==============================

void GIF_MetaHandler::ProcessTNail()
{

	XMP_Throw ( "GIF_MetaHandler::ProcessTNail isn't implemented yet", kXMPErr_Unimplemented );

}	// GIF_MetaHandler::ProcessTNail

// =================================================================================================
// GIF_MetaHandler::ProcessXMP
// ============================
//
// Process the raw XMP and legacy metadata that was previously cached.

void GIF_MetaHandler::ProcessXMP()
{
	this->processedXMP = true;	// Make sure we only come through here once.

	// Process the XMP packet.

	if ( ! this->xmpPacket.empty() ) {
	
		XMP_Assert ( this->containsXMP );
		XMP_StringPtr packetStr = this->xmpPacket.c_str();
		XMP_StringLen packetLen = this->xmpPacket.size();

		this->xmpObj.ParseFromBuffer ( packetStr, packetLen );

		this->containsXMP = true;

	}

}	// GIF_MetaHandler::ProcessXMP

// =================================================================================================
// GIF_MetaHandler::UpdateFile
// ============================

void GIF_MetaHandler::UpdateFile ( bool doSafeUpdate )
{
	bool updated = false;
	
	if ( ! this->needsUpdate ) return;
	if ( doSafeUpdate ) XMP_Throw ( "GIF_MetaHandler::UpdateFile: Safe update not supported", kXMPErr_Unavailable );
	
	XMP_StringPtr packetStr = xmpPacket.c_str();
	XMP_StringLen packetLen = xmpPacket.size();
	if ( packetLen == 0 ) return;

	XMP_IO* fileRef(this->parent->ioRef);
	if ( fileRef == 0 ) return;

	GIF_Support::BlockState blockState;
	long numBlocks = GIF_Support::OpenGIF ( fileRef, blockState );
	if ( numBlocks == 0 ) return;

	// write/update block(s)
	if (blockState.xmpLen == 0)
	{
		// no current chunk -> inject
		updated = SafeWriteFile();
	}
	else if (blockState.xmpLen >= packetLen )
	{
		// current chunk size is sufficient -> write and update CRC (in place update)
		updated = GIF_Support::WriteBuffer(fileRef, blockState.xmpPos, packetLen, packetStr );
		// GIF doesn't have a CRC like PNG.
	}
	else if (blockState.xmpLen < packetLen)
	{
		// XMP is too large for current chunk -> expand 
		updated = SafeWriteFile();
	}

	if ( ! updated )return;	// If there's an error writing the chunk, bail.

	this->needsUpdate = false;

}	// GIF_MetaHandler::UpdateFile

// =================================================================================================
// GIF_MetaHandler::WriteFile
// ===========================

void GIF_MetaHandler::WriteTempFile ( XMP_IO* tempRef )
{
	XMP_IO* originalRef = this->parent->ioRef;

	GIF_Support::BlockState blockState;
	long numBlocks = GIF_Support::OpenGIF ( originalRef, blockState );
	if ( numBlocks == 0 ) return;

	tempRef->Truncate( 0 );
	// LFA_Write(destRef, GIF_SIGNATURE_DATA, GIF_SIGNATURE_LEN);

	GIF_Support::BlockIterator curPos = blockState.blocks.begin();
	GIF_Support::BlockIterator endPos = blockState.blocks.end();

	long blockCount;

	for (blockCount = 0; (curPos != endPos); ++curPos, ++blockCount)
	{
		GIF_Support::BlockData block = *curPos;

		// discard existing XMP block
		if (block.xmp)
			continue;

		// copy any other block
		GIF_Support::CopyBlock(originalRef, tempRef, block);

		// place XMP block immediately before trailer
		if (blockCount == numBlocks - 2)
		{
			XMP_StringPtr packetStr = xmpPacket.c_str();
			XMP_StringLen packetLen = xmpPacket.size();

			GIF_Support::WriteXMPBlock(tempRef, packetLen, packetStr );
		}
	}

}	// GIF_MetaHandler::WriteFile

// =================================================================================================
// GIF_MetaHandler::SafeWriteFile
// ===========================

bool GIF_MetaHandler::SafeWriteFile ()
{
	bool ret = false;

	XMP_IO* originalFile  = this->parent->ioRef;
	XMP_IO* tempFile = originalFile->DeriveTemp();
	if ( tempFile == 0 )
		XMP_Throw ( "Failure creating GIF temp file", kXMPErr_InternalFailure );

	this->WriteTempFile( tempFile );
	originalFile->AbsorbTemp();

	return true;
} // GIF_MetaHandler::SafeWriteFile

// =================================================================================================
