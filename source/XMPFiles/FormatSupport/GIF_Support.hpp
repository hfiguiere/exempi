#ifndef __GIF_Support_hpp__
#define __GIF_Support_hpp__ 1

// =================================================================================================
// ADOBE SYSTEMS INCORPORATED
// Copyright 2002-2007 Adobe Systems Incorporated
// All Rights Reserved
//
// NOTICE: Adobe permits you to use, modify, and distribute this file in accordance with the terms
// of the Adobe license agreement accompanying it
//
// Derived from PNG_Support.hpp by Ian Jacobi
// =================================================================================================

#include "XMP_Environment.h"	// ! This must be the first include.

#include "XMPFiles_Impl.hpp"

#define GIF_SIGNATURE_LEN		3
#define GIF_SIGNATURE_DATA		"\x47\x49\x46"

#define APPLICATION_HEADER_LEN			14
#define APPLICATION_HEADER_DATA		"\x21\xFF\x0B\x58\x4D\x50\x20\x44\x61\x74\x61\x58\x4D\x50"

#define MAGIC_TRAILER_LEN		258
#define MAGIC_TRAILER_DATA		"\x01\xFF\xFE\xFD\xFC\xFB\xFA\xF9\xF8\xF7\xF6\xF5\xF4\xF3\xF2\xF1\xF0\xEF\xEE\xED\xEC\xEB\xEA\xE9\xE8\xE7\xE6\xE5\xE4\xE3\xE2\xE1\xE0\xDF\xDE\xDD\xDC\xDB\xDA\xD9\xD8\xD7\xD6\xD5\xD4\xD3\xD2\xD1\xD0\xCF\xCE\xCD\xCC\xCB\xCA\xC9\xC8\xC7\xC6\xC5\xC4\xC3\xC2\xC1\xC0\xBF\xBE\xBD\xBC\xBB\xBA\xB9\xB8\xB7\xB6\xB5\xB4\xB3\xB2\xB1\xB0\xAF\xAE\xAD\xAC\xAB\xAA\xA9\xA8\xA7\xA6\xA5\xA4\xA3\xA2\xA1\xA0\x9F\x9E\x9D\x9C\x9B\x9A\x99\x98\x97\x96\x95\x94\x93\x92\x91\x90\x8F\x8E\x8D\x8C\x8B\x8A\x89\x88\x87\x86\x85\x84\x83\x82\x81\x80\x7F\x7E\x7D\x7C\x7B\x7A\x79\x78\x77\x76\x75\x74\x73\x72\x71\x70\x6F\x6E\x6D\x6C\x6B\x6A\x69\x68\x67\x66\x65\x64\x63\x62\x61\x60\x5F\x5E\x5D\x5C\x5B\x5A\x59\x58\x57\x56\x55\x54\x53\x52\x51\x50\x4F\x4E\x4D\x4C\x4B\x4A\x49\x48\x47\x46\x45\x44\x43\x42\x41\x40\x3F\x3E\x3D\x3C\x3B\x3A\x39\x38\x37\x36\x35\x34\x33\x32\x31\x30\x2F\x2E\x2D\x2C\x2B\x2A\x29\x28\x27\x26\x25\x24\x23\x22\x21\x20\x1F\x1E\x1D\x1C\x1B\x1A\x19\x18\x17\x16\x15\x14\x13\x12\x11\x10\x0F\x0E\x0D\x0C\x0B\x0A\x09\x08\x07\x06\x05\x04\x03\x02\x01\x00\x00"

namespace GIF_Support 
{
	class BlockData
	{
		public:
			BlockData() : pos(0), len(0), type(0), xmp(false) {}
			virtual ~BlockData() {}

			XMP_Uns64	pos;		// file offset of block
			XMP_Uns32	len;		// length of block data, including extension introducer and label
			char		type;		// name/type of block
			bool		xmp;		// application extension block with XMP ?
	};

	typedef std::vector<BlockData> BlockVector;
	typedef BlockVector::iterator BlockIterator;

	class BlockState
	{
		public:
			BlockState() : xmpPos(0), xmpLen(0) {}
			virtual ~BlockState() {}

			XMP_Uns64	xmpPos;
			XMP_Uns32	xmpLen;
			BlockData	xmpBlock;
			BlockVector blocks;	/* vector of blocks */
	};

	long OpenGIF ( LFA_FileRef fileRef, BlockState& inOutBlockState );

	long ReadHeader ( LFA_FileRef fileRef );
	bool ReadBlock ( LFA_FileRef fileRef, BlockState& inOutBlockState, unsigned char* blockType, XMP_Uns32* blockLength, XMP_Uns64& inOutPosition );
	bool WriteXMPBlock ( LFA_FileRef fileRef, XMP_Uns32 len, const char* inBuffer );
	bool CopyBlock ( LFA_FileRef sourceRef, LFA_FileRef destRef, BlockData& block );

	unsigned long CheckApplicationBlockHeader ( LFA_FileRef fileRef, BlockState& inOutBlockState, BlockData& inOutBlockData, XMP_Uns64& inOutPosition );

	bool ReadBuffer ( LFA_FileRef fileRef, XMP_Uns64& pos, XMP_Uns32 len, char* outBuffer );
	bool WriteBuffer ( LFA_FileRef fileRef, XMP_Uns64& pos, XMP_Uns32 len, const char* inBuffer );

} // namespace GIF_Support

#endif	// __GIF_Support_hpp__
