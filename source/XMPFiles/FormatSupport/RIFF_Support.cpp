// =================================================================================================
// ADOBE SYSTEMS INCORPORATED
// Copyright 2002-2008 Adobe Systems Incorporated
// All Rights Reserved
//
// NOTICE: Adobe permits you to use, modify, and distribute this file in accordance with the terms
// of the Adobe license agreement accompanying it.
// =================================================================================================

#include <stdlib.h>

#include "RIFF_Support.hpp"

#if XMP_WinBuild
	#pragma warning ( disable : 4996 )	// '...' was declared deprecated
#endif

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
	static bool ReadTag ( LFA_FileRef inFileRef, long * outTag, UInt32 * outLength, long * subtype, UInt64 & inOutPosition, UInt64 maxOffset );
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
		long starttag;
		UInt32 taglen;
		UInt32 rifflen, avail;
		UInt64 pos;
	
		/* look for top level Premiere padding chunk */
		starttag = 0;
		while ( FindChunk ( inOutRiffState, ckidPremierePadding, riffType, 0, &starttag, &taglen, &pos ) ) {
	
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

		if ( (pos & 1) == 1 ) {
			// The file length is odd, need a pad byte.
			XMP_Uns8 pad = 0;
			LFA_Write ( inFileRef, &pad, 1 );
			++pos;
		}
	
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

		const XMP_Int64 fileLen = LFA_Measure ( inFileRef );
		if ( fileLen < 8 ) return 0;
		
		LFA_Seek ( inFileRef, 0, SEEK_SET );
	
		while ( ReadTag ( inFileRef, &tag, &len, &subtype, pos, fileLen ) ) {
			if ( tag != FOURCC_RIFF ) break;
			AddTag ( inOutRiffState, tag, len, pos, 0, 0, subtype );
			if ( subtype != 0 ) SubRead ( inFileRef, inOutRiffState, subtype, len, pos );
		}
	
		return (long) inOutRiffState.tags.size();

	}

	// =============================================================================================

	static bool  ReadTag ( LFA_FileRef inFileRef, long * outTag, UInt32 * outLength, long * subtype, UInt64 & inOutPosition, UInt64 maxOffset )
	{
		UInt32	realLength;
	
		long bytesRead;
		bytesRead = LFA_Read ( inFileRef, outTag, 4 );
		if ( bytesRead != 4 ) return false;
		*outTag = GetUns32LE ( outTag );
		
		bytesRead = LFA_Read ( inFileRef, outLength, 4 );
		if ( bytesRead != 4 ) return false;
		*outLength = GetUns32LE ( outLength );

		realLength = *outLength;
		realLength += (realLength & 1);		// Round up to an even value.

		inOutPosition = GetFilePosition ( inFileRef );	// The file offset of the data portion.
		UInt64 maxLength = maxOffset - inOutPosition;

		if ( (inOutPosition > maxOffset) || ((UInt64)(*outLength) > maxLength) ) {

			bool ignoreLastPad = true;	// Ignore cases where a final pad byte is missing.
			UInt64 fileLen = LFA_Measure ( inFileRef );
			if ( inOutPosition > (maxOffset + 1) ) ignoreLastPad = false;
			if ( (UInt64)(*outLength) > (maxLength + 1) ) ignoreLastPad = false;

			if ( ! ignoreLastPad ) {

				// Workaround for bad files in the field that have a bad size in the outermost RIFF
				// chunk. Do a "runtime repair" of cases where the length is too long (beyond EOF).
				// This handles read-only usage, update usage is repaired (or not) in the handler.

				bool oversizeRIFF = (inOutPosition == 8) &&	// Is this the initial 'RIFF' chunk?
									(fileLen >= 8);			// Is the file at least of the minimal size?
								 
				if ( ! oversizeRIFF ) {
					XMP_Throw ( "RIFF tag exceeds maximum length", kXMPErr_BadValue );
				} else {
					*outLength = (UInt32)(fileLen) - 8;
					realLength = *outLength;
					realLength += (realLength & 1);		// Round up to an even value.
				}

			}

		}
		
		*subtype = 0;

		if ( (*outTag != FOURCC_LIST) && (*outTag != FOURCC_RIFF) ) {

			UInt64 tempPos = inOutPosition + realLength;
			if ( tempPos <= maxOffset ) {
				LFA_Seek ( inFileRef, tempPos, SEEK_SET );
			} else if ( (tempPos == (maxOffset + 1)) && (maxOffset == (UInt64)LFA_Measure(inFileRef)) ) {
				LFA_Seek ( inFileRef, 0, SEEK_END );	// Hack to tolerate a missing final pad byte.
			} else {
				XMP_Throw ( "Bad RIFF offset", kXMPErr_BadValue );
			}

		} else  {

			bytesRead = LFA_Read ( inFileRef, subtype, 4 );
			if ( bytesRead != 4 ) return false;
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
				if ( tempPos <= maxOffset ) {
					LFA_Seek ( inFileRef, tempPos, SEEK_SET );
				} else if ( (tempPos == (maxOffset + 1)) && (maxOffset == (UInt64)LFA_Measure(inFileRef)) ) {
					LFA_Seek ( inFileRef, 0, SEEK_END );	// Hack to tolerate a missing final pad byte.
				} else {
					XMP_Throw ( "Bad RIFF offset", kXMPErr_BadValue );
				}
				*outLength += 4;
				*outTag = *subtype;
				*subtype = 0;
			}

			inOutPosition = GetFilePosition ( inFileRef );

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
		parentnum = (long) inOutRiffState.tags.size() - 1;
		
		UInt64 maxOffset = inOutPosition + parentlen;
	
		while ( parentlen > 0 ) {

			oldpos = inOutPosition;
			ReadTag ( inFileRef, &tag, &len, &subtype, inOutPosition, maxOffset );
			AddTag ( inOutRiffState, tag, len, inOutPosition, parentid, parentnum, subtype );
			len += (len & 1); //padding byte

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
						long parentID, long subtypeID, char * outBuffer, unsigned long * outBufferSize,
						UInt64* posPtr )
	{
		UInt32 len;
		UInt64 pos;
		
		bool found = FindChunk ( inOutRiffState, tagID, parentID, subtypeID, 0, &len, &pos );
		if ( ! found ) return false;
	
		if ( posPtr != 0 )
			*posPtr = pos;	// return position CBR

		if ( outBuffer == 0 ) {
			*outBufferSize = (unsigned long)len;
			return true;	// Found, but not wanted.
		}
	
		if ( len > *outBufferSize )
			len = *outBufferSize;
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

// =================================================================================================

// *** Could be moved to a separate header

#pragma pack(push,1)

//	[TODO] Can we switch to using just a full path here?
struct FSSpecLegacy
{
	short	vRefNum;
	long	parID;
	char	name[260]; // http://msdn.microsoft.com/library/default.asp?url=/library/en-us/fileio/fs/naming_a_file.asp  -- 260 is "old school", 32000 is "new school".
};

struct CR8R_CreatorAtom
{
	unsigned long		magicLu;

	long				atom_sizeL;			// Size of this structure.
	short				atom_vers_majorS;	// Major atom version.
	short				atom_vers_minorS;	// Minor atom version.

	// mac
	unsigned long		creator_codeLu;		// Application code on MacOS.
	unsigned long		creator_eventLu;	// Invocation appleEvent.

	// windows
	char				creator_extAC[16];	// Extension allowing registry search to app.
	char				creator_flagAC[16];	// Flag passed to app at invocation time.

	char				creator_nameAC[32];	// Name of the creator application.
};

typedef CR8R_CreatorAtom** CR8R_CreatorAtomHandle;

#define PR_PROJECT_LINK_MAGIC 0x600DF00D // GoodFood 

typedef enum
{
	Embed_ExportTypeMovie = 0,
	Embed_ExportTypeStill,
	Embed_ExportTypeAudio,
	Embed_ExportTypeCustom
}
Embed_ExportType;


struct Embed_ProjectLinkAtom
{
	unsigned long		magicLu;
	long				atom_sizeL;
	short				atom_vers_apiS;
	short				atom_vers_codeS;
	unsigned long		exportType;		// See enum. The type of export that generated the file
	FSSpecLegacy		fullPath;		// Full path of the project file
};

#pragma pack(pop)

// -------------------------------------------------------------------------------------------------

#define kCreatorTool		"CreatorTool"
#define AdobeCreatorAtomVersion_Major 1
#define AdobeCreatorAtomVersion_Minor 0
#define AdobeCreatorAtom_Magic 0xBEEFCAFE

#define myCreatorAtom	MakeFourCC ( 'C','r','8','r' )

static void CreatorAtom_Initialize ( CR8R_CreatorAtom& creatorAtom )
{
	memset ( &creatorAtom, 0, sizeof(CR8R_CreatorAtom) );
	creatorAtom.magicLu				= AdobeCreatorAtom_Magic;
	creatorAtom.atom_vers_majorS	= AdobeCreatorAtomVersion_Major;
	creatorAtom.atom_vers_minorS	= AdobeCreatorAtomVersion_Minor;
	creatorAtom.atom_sizeL			= sizeof(CR8R_CreatorAtom);
}

// -------------------------------------------------------------------------------------------------

static void CreatorAtom_MakeValid ( CR8R_CreatorAtom * creator_atomP )
{
	// If already valid, no conversion is needed.
	if ( creator_atomP->magicLu == AdobeCreatorAtom_Magic ) return;

	Flip4 ( &creator_atomP->magicLu );
	Flip2 ( &creator_atomP->atom_vers_majorS );
	Flip2 ( &creator_atomP->atom_vers_minorS );

	Flip4 ( &creator_atomP->atom_sizeL );
	Flip4 ( &creator_atomP->creator_codeLu );
	Flip4 ( &creator_atomP->creator_eventLu );

	XMP_Assert ( creator_atomP->magicLu == AdobeCreatorAtom_Magic );
}

// -------------------------------------------------------------------------------------------------

static void CreatorAtom_ToBE ( CR8R_CreatorAtom * creator_atomP )
{
	creator_atomP->atom_vers_majorS = MakeUns16BE ( creator_atomP->atom_vers_majorS );
	creator_atomP->atom_vers_minorS = MakeUns16BE ( creator_atomP->atom_vers_minorS );

	creator_atomP->magicLu			= MakeUns32BE ( creator_atomP->magicLu );
	creator_atomP->atom_sizeL		= MakeUns32BE ( creator_atomP->atom_sizeL );
	creator_atomP->creator_codeLu	= MakeUns32BE ( creator_atomP->creator_codeLu );
	creator_atomP->creator_eventLu	= MakeUns32BE ( creator_atomP->creator_eventLu );
}

// -------------------------------------------------------------------------------------------------

static void ProjectLinkAtom_MakeValid ( Embed_ProjectLinkAtom * link_atomP )
{
	// If already valid, no conversion is needed.
	if ( link_atomP->magicLu == PR_PROJECT_LINK_MAGIC ) return;

	// do the header
	Flip4 ( &link_atomP->magicLu );
	Flip4 ( &link_atomP->atom_sizeL );
	Flip2 ( &link_atomP->atom_vers_apiS );
	Flip2 ( &link_atomP->atom_vers_codeS );

	// do the FSSpec data
	Flip2 ( &link_atomP->fullPath.vRefNum );
	Flip4 ( &link_atomP->fullPath.parID );

	XMP_Assert ( link_atomP->magicLu == PR_PROJECT_LINK_MAGIC );
}

// -------------------------------------------------------------------------------------------------

static std::string CharsToString ( const char* buffer, int maxBuffer )
{
	// convert possibly non-zero terminated char buffer to std::string
	std::string result;

	char bufferz[256];
	XMP_Assert ( maxBuffer < 256 );
	if ( maxBuffer >= 256 ) return result;

	memcpy ( bufferz, buffer, maxBuffer );
	bufferz[maxBuffer] = 0;

	result = bufferz;
	return result;

}

// -------------------------------------------------------------------------------------------------

bool CreatorAtom::Import ( SXMPMeta& xmpObj, 
						   LFA_FileRef fileRef, 
						   RIFF_Support::RiffState& riffState )
{
	static const long myProjectLink	= MakeFourCC ( 'P','r','m','L' );
	
	unsigned long projectLinkSize;
	bool ok = RIFF_Support::GetRIFFChunk ( fileRef, riffState, myProjectLink, 0, 0, 0, &projectLinkSize );
	if ( ok ) {

		Embed_ProjectLinkAtom epla;

		std::string projectPathString;		
		RIFF_Support::GetRIFFChunk ( fileRef, riffState, myProjectLink, 0, 0, (char*) &epla, &projectLinkSize );
		if ( ok ) {
			ProjectLinkAtom_MakeValid ( &epla );
			projectPathString = epla.fullPath.name;
		}

		if ( ! projectPathString.empty() ) {

			if ( projectPathString[0] == '/' ) {
				xmpObj.SetStructField ( kXMP_NS_CreatorAtom, "macAtom",
										kXMP_NS_CreatorAtom, "posixProjectPath", projectPathString, 0 );
			} else if ( projectPathString.substr(0,4) == std::string("\\\\?\\") ) {
				xmpObj.SetStructField ( kXMP_NS_CreatorAtom, "windowsAtom",
										kXMP_NS_CreatorAtom, "uncProjectPath", projectPathString, 0 );
			}

			std::string projectTypeString;
			switch ( epla.exportType ) {
				case Embed_ExportTypeMovie	: projectTypeString = "movie"; break;
				case Embed_ExportTypeStill	: projectTypeString = "still"; break;
				case Embed_ExportTypeAudio  : projectTypeString = "audio"; break;
				case Embed_ExportTypeCustom : projectTypeString = "custom"; break;
			}

			if ( ! projectTypeString.empty() ) {
				xmpObj.SetStructField ( kXMP_NS_DM, "projectRef", kXMP_NS_DM, "type", projectTypeString.c_str() );
			}

		}

	}

	unsigned long creatorAtomSize = 0;
	ok = RIFF_Support::GetRIFFChunk ( fileRef, riffState, myCreatorAtom, 0, 0, 0, &creatorAtomSize );
	if ( ok ) {

		CR8R_CreatorAtom creatorAtom;
		ok = RIFF_Support::GetRIFFChunk ( fileRef, riffState, myCreatorAtom, 0, 0, (char*) &creatorAtom, &creatorAtomSize );

		if ( ok ) {

			CreatorAtom_MakeValid ( &creatorAtom );

			char buffer[256];
			std::string xmpString;

			sprintf ( buffer, "%d", creatorAtom.creator_codeLu );
			xmpString = buffer;
			xmpObj.SetStructField ( kXMP_NS_CreatorAtom, "macAtom", kXMP_NS_CreatorAtom, "applicationCode", xmpString, 0 );

			sprintf ( buffer, "%d", creatorAtom.creator_eventLu );
			xmpString = buffer;
			xmpObj.SetStructField ( kXMP_NS_CreatorAtom, "macAtom", kXMP_NS_CreatorAtom, "invocationAppleEvent", xmpString, 0 );

			xmpString = CharsToString ( creatorAtom.creator_extAC, sizeof(creatorAtom.creator_extAC) );
			xmpObj.SetStructField ( kXMP_NS_CreatorAtom, "windowsAtom", kXMP_NS_CreatorAtom, "extension", xmpString, 0 );

			xmpString = CharsToString ( creatorAtom.creator_flagAC, sizeof(creatorAtom.creator_flagAC) );
			xmpObj.SetStructField ( kXMP_NS_CreatorAtom, "windowsAtom", kXMP_NS_CreatorAtom, "invocationFlags", xmpString, 0 );

			xmpString = CharsToString ( creatorAtom.creator_nameAC, sizeof(creatorAtom.creator_nameAC) );
			xmpObj.SetProperty ( kXMP_NS_XMP, "CreatorTool", xmpString, 0 );

		}

	}

	return ok;

}

// -------------------------------------------------------------------------------------------------

// *** Not in C library:
#ifndef min
	#define min(a,b) ( (a < b) ? a : b )
#endif

#define EnsureFinalNul(buffer) buffer [ sizeof(buffer) - 1 ] = 0

bool CreatorAtom::Update ( SXMPMeta& xmpObj,
						   LFA_FileRef fileRef,
						   long riffType,
						   RIFF_Support::RiffState& riffState )
{

	// Creator Atom related values.
	bool found = false;
	std::string posixPathString, uncPathString;
	if ( xmpObj.GetStructField ( kXMP_NS_CreatorAtom, "macAtom", kXMP_NS_CreatorAtom, "posixProjectPath", &posixPathString, 0 ) ) found = true;
	if ( xmpObj.GetStructField ( kXMP_NS_CreatorAtom, "windowsAtom", kXMP_NS_CreatorAtom, "uncProjectPath", &uncPathString, 0 ) ) found = true;

	std::string applicationCodeString, invocationAppleEventString, extensionString, invocationFlagsString, creatorToolString;
	if ( xmpObj.GetStructField ( kXMP_NS_CreatorAtom, "macAtom", kXMP_NS_CreatorAtom, "applicationCode", &applicationCodeString, 0 ) ) found = true;
	if ( xmpObj.GetStructField ( kXMP_NS_CreatorAtom, "macAtom", kXMP_NS_CreatorAtom, "invocationAppleEvent", &invocationAppleEventString, 0 ) ) found = true;
	if ( xmpObj.GetStructField ( kXMP_NS_CreatorAtom, "windowsAtom", kXMP_NS_CreatorAtom, "extension", &extensionString, 0 ) ) found = true;
	if ( xmpObj.GetStructField ( kXMP_NS_CreatorAtom, "windowsAtom", kXMP_NS_CreatorAtom, "invocationFlags", &invocationFlagsString, 0 ) ) found = true;
	if ( xmpObj.GetProperty ( kXMP_NS_XMP, "CreatorTool", &creatorToolString, 0 ) ) found = true;

	// No Creator Atom information present.
	if ( ! found ) return true;

	// Read Legacy Creator Atom.
	unsigned long creatorAtomSize = 0;
	CR8R_CreatorAtom creatorAtomLegacy;
	CreatorAtom_Initialize ( creatorAtomLegacy );
	bool ok = RIFF_Support::GetRIFFChunk ( fileRef, riffState, myCreatorAtom, 0, 0, 0, &creatorAtomSize );
	if ( ok ) {
		XMP_Assert ( creatorAtomSize == sizeof(CR8R_CreatorAtom) );
		ok = RIFF_Support::GetRIFFChunk ( fileRef, riffState, myCreatorAtom, 0, 0, (char*) &creatorAtomLegacy, &creatorAtomSize );
		CreatorAtom_MakeValid ( &creatorAtomLegacy );
	}

	// Generate new Creator Atom from XMP.
	CR8R_CreatorAtom creatorAtomViaXMP;
	CreatorAtom_Initialize ( creatorAtomViaXMP );
	if ( ! applicationCodeString.empty() ) {
		creatorAtomViaXMP.creator_codeLu = strtoul ( applicationCodeString.c_str(), 0, 0 );
	}
	if ( ! invocationAppleEventString.empty() ) {
		creatorAtomViaXMP.creator_eventLu = strtoul ( invocationAppleEventString.c_str(), 0, 0 );
	}
	if ( ! extensionString.empty() ) {
		strncpy ( creatorAtomViaXMP.creator_extAC, extensionString.c_str(), sizeof(creatorAtomViaXMP.creator_extAC) );
		EnsureFinalNul ( creatorAtomViaXMP.creator_extAC );
	}
	if ( ! invocationFlagsString.empty() ) {
		strncpy ( creatorAtomViaXMP.creator_flagAC, invocationFlagsString.c_str(), sizeof(creatorAtomViaXMP.creator_flagAC) );
		EnsureFinalNul ( creatorAtomViaXMP.creator_flagAC );
	}
	if ( ! creatorToolString.empty() ) {
		strncpy ( creatorAtomViaXMP.creator_nameAC, creatorToolString.c_str(), sizeof(creatorAtomViaXMP.creator_nameAC) );
		EnsureFinalNul ( creatorAtomViaXMP.creator_nameAC );
	}

	// Write new Creator Atom, if necessary.
	if ( memcmp ( &creatorAtomViaXMP, &creatorAtomLegacy, sizeof(CR8R_CreatorAtom) ) != 0 ) {
		CreatorAtom_ToBE ( &creatorAtomViaXMP );
		ok = RIFF_Support::PutChunk ( fileRef, riffState, riffType, myCreatorAtom, (char*)&creatorAtomViaXMP, sizeof(CR8R_CreatorAtom) );
	}

	return ok;

}
