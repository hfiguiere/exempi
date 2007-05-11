// =================================================================================================
// ADOBE SYSTEMS INCORPORATED
// Copyright 2002-2007 Adobe Systems Incorporated
// All Rights Reserved
//
// NOTICE: Adobe permits you to use, modify, and distribute this file in accordance with the terms
// of the Adobe license agreement accompanying it.
// =================================================================================================

#include "RIFF_Support.hpp"

namespace RIFF_Support {

	#define	ckidPremierePadding	MakeFourCC ('J','U','N','Q')
	#define	formtypeAVIX		MakeFourCC ('A', 'V', 'I', 'X')


	#ifndef	AVIMAXCHUNKSIZE
		#define	AVIMAXCHUNKSIZE	((UInt32) 0x80000000)		/* 2 GB */
	#endif


	typedef struct
	{
		long	id;
		UInt32	len;
	} atag;

	// Local function declarations
	static bool ReadTag ( LFA_FileRef inFileRef, long * outTag, UInt32 * outLength, long * subtype, UInt64 & inOutPosition );
	static void AddTag ( RiffState & inOutRiffState, long tag, UInt32 len, UInt64 & inOutPosition, long parentID, long parentnum, long subtypeID );
	static long SubRead ( LFA_FileRef inFileRef, RiffState & inOutRiffState, long parentid, UInt32 parentlen, UInt64 & inOutPosition );
	static bool ReadChunk ( LFA_FileRef inFileRef, UInt64 & pos, UInt32 len, char * outBuffer );

	#define GetFilePosition(file)	LFA_Seek ( file, 0, SEEK_CUR )
	
	// =============================================================================================

	bool GetMetaData ( LFA_FileRef inFileRef, long tagID, char * outBuffer, unsigned long * outBufferSize )
	{
		RiffState riffState;
	
		long numTags = OpenRIFF ( inFileRef, riffState );
		if ( numTags == 0 ) return false;
	
		return GetRIFFChunk ( inFileRef, riffState, tagID, 0, 0, outBuffer, outBufferSize );
	
	}

	// =============================================================================================

	bool SetMetaData ( LFA_FileRef inFileRef, long riffType, long tagID, const char * inBuffer, unsigned long inBufferSize )
	{
		RiffState riffState;
	
		long numTags = OpenRIFF ( inFileRef, riffState );
		if ( numTags == 0 ) return false;
	
		return PutChunk ( inFileRef, riffState, riffType, tagID, inBuffer, inBufferSize );
	
	}

	// =============================================================================================

	bool MarkChunkAsPadding ( LFA_FileRef inFileRef, RiffState & inOutRiffState, long riffType, long tagID, long subtypeID )
	{
		UInt32 len;
		UInt64 pos;
		atag tag;
	
		try {
	
			bool found = FindChunk ( inOutRiffState, tagID, riffType, subtypeID, NULL, &len, &pos );
			if ( ! found ) return false;
	
			if ( subtypeID != 0 ) {
				pos -= 12;
			} else {
				pos -= 8;
			}

			tag.id = MakeUns32LE ( ckidPremierePadding );
			LFA_Seek ( inFileRef, pos, SEEK_SET );
			LFA_Write ( inFileRef, &tag, 4 );
	
			pos += 8;
			AddTag ( inOutRiffState, ckidPremierePadding, len, pos, 0, 0, 0 );
	
		} catch(...) {
	
			return false;	// If a write fails, it throws, so we return false.
	
		}
	
		return true;
	}

	// =============================================================================================

	bool PutChunk ( LFA_FileRef inFileRef, RiffState & inOutRiffState, long riffType, long tagID, const char * inBuffer, UInt32 inBufferSize )
	{
		UInt32 len;
		UInt64 pos;
		atag tag;
	
		// Make sure we're writting an even number of bytes. Required by the RIFF specification.
		XMP_Assert ( (inBufferSize & 1) == 0 );
	
		try {

			bool found = FindChunk ( inOutRiffState, tagID, 0, 0, NULL, &len, &pos );
			if ( found ) {

				if ( len == inBufferSize ) {
					LFA_Seek ( inFileRef, pos, SEEK_SET );
					LFA_Write ( inFileRef, inBuffer, inBufferSize );
					return true;
				}
	
				pos -= 8;
				tag.id = MakeUns32LE ( ckidPremierePadding );
				LFA_Seek ( inFileRef, pos, SEEK_SET );
				LFA_Write ( inFileRef, &tag, 4 );
	
				if ( len > inBufferSize ) {
					pos += 8;
					AddTag ( inOutRiffState, ckidPremierePadding, len, pos, 0, 0, 0 );
				}

			}

		} catch ( ... ) {

			// If a write fails, it throws, so we return false
			return false;

		}
	
		bool ok = MakeChunk ( inFileRef, inOutRiffState, riffType, (inBufferSize + 8) );
		if ( ! ok ) return false;
	
		return WriteChunk ( inFileRef, tagID, inBuffer, inBufferSize );
	
	}

	// =============================================================================================

	bool RewriteChunk ( LFA_FileRef inFileRef, RiffState & inOutRiffState, long tagID, long parentID, const char * inData )
	{
		UInt32 len;
		UInt64 pos;
	
		try {
			if ( FindChunk ( inOutRiffState, tagID, parentID, 0, NULL, &len, &pos ) ) {
				LFA_Seek ( inFileRef, pos, SEEK_SET );
				LFA_Write ( inFileRef, inData, len );
			}
		} catch ( ... ) {
			return false;
		}
	
		return true;
	
	}

	// =============================================================================================

	bool MakeChunk ( LFA_FileRef inFileRef, RiffState & inOutRiffState, long riffType, UInt32 len )
	{
		long starttag, taglen;
		UInt32 rifflen, avail;
		UInt64 pos;
	
		/* look for top level Premiere padding chunk */
		starttag = 0;
		while ( FindChunk ( inOutRiffState, ckidPremierePadding, riffType, 0, &starttag, reinterpret_cast<unsigned long*>(&taglen), &pos ) ) {
	
			pos -= 8;
			taglen += 8;
			long extra = taglen - len;
			if ( extra < 0 ) continue;
	
			RiffIterator iter = inOutRiffState.tags.begin();
			iter += (starttag - 1);
	
			if ( extra == 0 ) {

				iter->len = 0;

			} else {

				atag pad;
				UInt64 padpos;
	
				/*  need 8 bytes extra to be able to split it */
				extra -= 8;
				if ( extra < 0 ) continue;
	
				try{
					padpos = pos + len;
					LFA_Seek ( inFileRef, padpos, SEEK_SET );
					pad.id = MakeUns32LE ( ckidPremierePadding );
					pad.len = MakeUns32LE ( extra );
					LFA_Write ( inFileRef, &pad, sizeof(pad) );
				} catch ( ... ) {
					return false;
				}

				iter->pos = padpos + 8;
				iter->len = extra;

			}
	
			/* seek back to start of original padding chunk */
			LFA_Seek ( inFileRef, pos, SEEK_SET );
	
			return true;

		}
	
		/* can't take padding chunk, so append new chunk to end of file */
	
		rifflen = inOutRiffState.rifflen + 8;
		avail = AVIMAXCHUNKSIZE - rifflen;
	
		LFA_Seek ( inFileRef, 0, SEEK_END );
		pos = GetFilePosition ( inFileRef );
	
		if ( avail < len ) {

			/* if needed, create new AVIX chunk */
			ltag avix;
	
			avix.id = MakeUns32LE ( FOURCC_RIFF );
			avix.len = MakeUns32LE ( 4 + len );
			avix.subid = MakeUns32LE ( formtypeAVIX );
			LFA_Write(inFileRef, &avix, sizeof(avix));
	
			pos += 12;
			AddTag ( inOutRiffState, avix.id, len, pos, 0, 0, 0 );

		} else {

			/* otherwise, rewrite length of last RIFF chunk in file */
			pos = inOutRiffState.riffpos + 4;
			rifflen = inOutRiffState.rifflen + len;
			XMP_Uns32 fileLen = MakeUns32LE ( rifflen );
			LFA_Seek ( inFileRef, pos, SEEK_SET );
			LFA_Write ( inFileRef, &fileLen, 4 );
			inOutRiffState.rifflen = rifflen;
	
			/* prepare to write data */
			LFA_Seek ( inFileRef, 0, SEEK_END );

		}
	
		return true;

	}

	// =============================================================================================

	bool WriteChunk ( LFA_FileRef inFileRef, long tagID, const char * data, UInt32 len )
	{
		atag ck;
		ck.id = MakeUns32LE ( tagID );
		ck.len = MakeUns32LE ( len );
	
		try {
			LFA_Write ( inFileRef, &ck, 8 );
			LFA_Write ( inFileRef, data, len );
		} catch ( ... ) {
			return false;
		}
	
		return true;
	}

	// =============================================================================================

	long OpenRIFF ( LFA_FileRef inFileRef, RiffState & inOutRiffState )
	{
		UInt64 pos = 0;
		long tag, subtype;
		UInt32 len;
	
		LFA_Seek ( inFileRef, 0, SEEK_SET );
	
		// read first tag (always RIFFtype)
		while ( ReadTag ( inFileRef, &tag, &len, &subtype, pos) ) {
			if ( tag != FOURCC_RIFF ) break;
			AddTag ( inOutRiffState, tag, len, pos, 0, 0, subtype );
			if ( subtype != 0 ) SubRead ( inFileRef, inOutRiffState, subtype, len, pos );
		}
	
		return inOutRiffState.tags.size();

	}

	// =============================================================================================

	static bool  ReadTag ( LFA_FileRef inFileRef, long * outTag, UInt32 * outLength, long * subtype, UInt64 & inOutPosition )
	{
		UInt32	realLength;
	
		try {

			long bytesRead;
			bytesRead = LFA_Read ( inFileRef, outTag, 4 );
			if ( bytesRead == 0 ) return false;
			*outTag = GetUns32LE ( outTag );
			
			bytesRead = LFA_Read ( inFileRef, outLength, 4 );
			if ( bytesRead == 0 ) return false;
			*outLength = GetUns32LE ( outLength );
	
			realLength = *outLength;
			realLength += (realLength & 1);		// round up to words
	
			*subtype = 0;
	
			if ( (*outTag != FOURCC_LIST) && (*outTag != FOURCC_RIFF) ) {

				inOutPosition = GetFilePosition ( inFileRef );
				UInt64 tempPos = inOutPosition + realLength;
				LFA_Seek ( inFileRef, tempPos, SEEK_SET );

			} else  {

				bytesRead = LFA_Read ( inFileRef, subtype, 4 );
				if ( bytesRead == 0 ) return false;
				*subtype = GetUns32LE ( subtype );

				*outLength -= 4;
				realLength -= 4;
	
				// Special case:
				// Since the 'movi' chunk can contain billions of subchunks, skip over the 'movi' subchunk.
				//
				// The 'movi' subtype is added to the list as the TAG.
				// The subtype is returned empty so nobody will try to parse the subchunks.

				if ( *subtype == listtypeAVIMOVIE ) {
					inOutPosition = GetFilePosition ( inFileRef );
					UInt64 tempPos = inOutPosition + realLength;
					LFA_Seek ( inFileRef, tempPos, SEEK_SET );
					*outLength += 4;
					*outTag = *subtype;
					*subtype = 0;
				}

				inOutPosition = GetFilePosition ( inFileRef );

			}

		} catch ( ... ) {

			return false;

		}
	
		return true;

	}

	// =============================================================================================

	static void  AddTag ( RiffState & inOutRiffState, long tag, UInt32 len, UInt64 & inOutPosition, long parentID, long parentnum, long subtypeID )
	{
		RiffTag	newTag;
	
		newTag.pos = inOutPosition;
		newTag.tagID = tag;
		newTag.len = len;
		newTag.parent = parentnum;
		newTag.parentID = parentID;
		newTag.subtypeID = subtypeID;
	
		inOutRiffState.tags.push_back ( newTag );
	
		if ( tag == FOURCC_RIFF ) {
			inOutRiffState.riffpos = inOutPosition - 12;
			inOutRiffState.rifflen = len + 4;
		}

	}

	// =============================================================================================

	static long SubRead ( LFA_FileRef inFileRef, RiffState & inOutRiffState, long parentid, UInt32 parentlen, UInt64 & inOutPosition )
	{
		long tag;
		long subtype = 0;
		long parentnum;
		UInt32 len, total, childlen;
		UInt64 oldpos;
	
		total = 0;
		parentnum = inOutRiffState.tags.size() - 1;
	
		while ( parentlen > 0 ) {

			oldpos = inOutPosition;
			ReadTag ( inFileRef, &tag, &len, &subtype, inOutPosition );
			AddTag ( inOutRiffState, tag, len, inOutPosition, parentid, parentnum, subtype );
			len += (len & 1);

			if ( subtype == 0 ) {
				childlen = 8 + len;
			} else {
				childlen = 12 + SubRead ( inFileRef, inOutRiffState, subtype, len, inOutPosition );
			}

			if ( parentlen < childlen ) parentlen = childlen;
			parentlen -= childlen;
			total += childlen;

		}
	
		return total;
	
	}

	// =============================================================================================

	bool GetRIFFChunk ( LFA_FileRef inFileRef, RiffState & inOutRiffState, long tagID,
						long parentID, long subtypeID, char * outBuffer, unsigned long * outBufferSize )
	{
		UInt32 len;
		UInt64 pos;
		
		bool found = FindChunk ( inOutRiffState, tagID, parentID, subtypeID, 0, &len, &pos );
		if ( ! found ) return false;
	
		if ( outBuffer == 0 ) {
			*outBufferSize = (unsigned long)len;
			return true;	// Found, but not wanted.
		}
	
		if ( len > *outBufferSize ) len = *outBufferSize;
	
		found = ReadChunk ( inFileRef, pos, len, outBuffer );
		return found;
	
	}

	// =============================================================================================

	bool FindChunk ( RiffState & inOutRiffState, long tagID, long parentID, long subtypeID,
					 long * startTagIndex, UInt32 * len, UInt64 * pos)
	{
		std::vector<RiffTag>::iterator iter = inOutRiffState.tags.begin();
		std::vector<RiffTag>::iterator endIter = inOutRiffState.tags.end();
		
		// If we're using the next index, skip the iterator.
		if ( startTagIndex != 0 ) iter += *startTagIndex;
	
		for ( ; iter != endIter ; ++iter ) {

			if ( startTagIndex != 0 ) *startTagIndex += 1;
	
			if ( (parentID!= 0) && (iter->parentID != parentID) ) continue;
			if ( (tagID != 0) && (iter->tagID != tagID) ) continue;
			if ( (subtypeID != 0) && (iter->subtypeID != subtypeID) ) continue;
	
			if ( len != 0 ) *len = iter->len;
			if ( pos != 0 ) *pos = iter->pos;
			
			return true;

		}
	
		return false;
	}

	// =============================================================================================

	static bool ReadChunk ( LFA_FileRef inFileRef, UInt64 & pos, UInt32 len, char * outBuffer )
	{
	
		if ( (inFileRef == 0) || (outBuffer == 0) ) return false;
	
		LFA_Seek (inFileRef, pos, SEEK_SET );
		UInt32 bytesRead = LFA_Read ( inFileRef, outBuffer, len );
		if ( bytesRead != len ) return false;
	
		return true;
	
	}

} // namespace RIFF_Support
