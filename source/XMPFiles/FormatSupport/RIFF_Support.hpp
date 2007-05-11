#ifndef __RIFF_Support_hpp__
#define __RIFF_Support_hpp__ 1

// =================================================================================================
// ADOBE SYSTEMS INCORPORATED
// Copyright 2002-2007 Adobe Systems Incorporated
// All Rights Reserved
//
// NOTICE: Adobe permits you to use, modify, and distribute this file in accordance with the terms
// of the Adobe license agreement accompanying it.
// =================================================================================================

#include "XMP_Environment.h"	// ! This must be the first include.

#include <vector>

#include "XMPFiles_Impl.hpp"

#define MakeFourCC(a,b,c,d) ((long)a | ((long)b << 8) | ((long)c << 16) | ((long)d << 24))

#if XMP_WinBuild
	#include <vfw.h>
#else
	#ifndef FOURCC_RIFF
		#define FOURCC_RIFF	MakeFourCC ('R', 'I', 'F', 'F')
	#endif
	#ifndef FOURCC_LIST
		#define FOURCC_LIST	MakeFourCC ('L', 'I', 'S', 'T')
	#endif
	#ifndef listtypeAVIMOVIE
		#define listtypeAVIMOVIE	MakeFourCC ('m', 'o', 'v', 'i')
	#endif
#endif

namespace RIFF_Support 
{
	// Some types, if not already defined
	#ifndef UInt64
		typedef unsigned long long UInt64;
	#endif
	#ifndef UInt32
		typedef unsigned long UInt32;
	#endif

	/**
	** Obtain the meta-data for the tagID provided.
	** Returns true for success
	*/
	bool GetMetaData ( LFA_FileRef inFileRef, long tagID, char * outBuffer, unsigned long * outBufferSize );

	/**
	** Write the meta-data for the tagID provided.
	** Returns true for success
	*/
	bool SetMetaData ( LFA_FileRef inFileRef, long riffType, long tagID, const char * inBuffer, unsigned long inBufferSize );



	/**
	** A class to hold the information
	** about a particular chunk.
	*/
	class RiffTag {
	public:
	
		RiffTag() : pos(0), tagID(0), len(0), parent(0), parentID(0), subtypeID(0) {}
		virtual ~RiffTag() {}

		UInt64	pos;		/* file offset of chunk data */
		long	tagID;		/* ckid of chunk */
		UInt32	len;		/* length of chunk data */
		long	parent;		/* chunk# of parent */
		long	parentID;	/* FOURCC of parent */
		long	subtypeID;	/* Subtype of the tag (aka LIST ID) */

	};

	typedef std::vector<RiffTag> RiffVector;
	typedef RiffVector::iterator RiffIterator;

	/**
	** A class to hold a table of the parsed
	** chunks from a file.  Its validity
	** expires when new chunks are added.
	*/
	class RiffState {
	public:

		RiffState() : riffpos(0), rifflen(0), next(0) {}
		virtual ~RiffState() {}

		UInt64 riffpos;		/* file offset of current RIFF */
		long rifflen;		/* length of RIFF incl. header */
		long next;			/* next one to search */
		RiffVector tags;	/* vector of chunks */

	};

	struct ltag {
		long	id;
		UInt32	len;
		long	subid;
	};

	/**
	** Read from the RIFF file, and build a table of the chunks
	** in the RIFFState class provided.
	** Returns the number of chunks found.
	*/
	long OpenRIFF ( LFA_FileRef inFileRef, RiffState & inOutRiffState );

	/**
	** Get a chunk from an existing RIFFState, obtained from
	** a call to OpenRIFF.
	** If NULL is passed for the outBuffer, the outBufferSize parameter 
	** will contain the field size if true if returned.
	**
	** Returns true if the chunk is found.
	*/
	bool GetRIFFChunk ( LFA_FileRef inFileRef, RiffState & inOutRiffState, long tagID, long parentID,
						long subtypeID, char * outBuffer, unsigned long * outBufferSize );


	/**
	** The routine finds an existing list and tags it as Padding
	**
	** Returns true if success
	*/
	bool MarkChunkAsPadding ( LFA_FileRef inFileRef, RiffState & inOutRiffState, long riffType, long tagID, long subtypeID );


	/**
	** The routine finds an existing location to put the chunk into if
	** available, otherwise it creates a new chunk and writes to it.
	**
	** Returns true if success
	*/
	bool PutChunk ( LFA_FileRef inFileRef, RiffState & inOutRiffState, long riffType, long tagID, const char * inBuffer, UInt32 inBufferSize );

	/**
	** Locates the position of a chunk.
	** All parameters except the RiffState are optional.
	**
	** Return if found.
	*/
	bool FindChunk ( RiffState & inOutRiffState, long tagID, long parentID, long subtypeID, long * starttag, UInt32 * len, UInt64 * pos );

	/**
	** Low level routine to write a chunk.
	**
	** Returns true if write succeeded.
	*/
	bool WriteChunk ( LFA_FileRef inFileRef, long tagID, const char * data, UInt32 len );

	/**
	** Rewrites data into an existing chunk, not writing the header like WriteChunk
	**
	** Returns true if found and write succeeded.
	*/
	bool RewriteChunk ( LFA_FileRef inFileRef, RiffState & inOutRiffState, long tagID, long parentID, const char * inData );

	/**
	** Attempts to find a location to write a chunk, and if not found, prepares a chunk
	** at the end of the file.
	**
	** Returns true if successful.
	*/
	bool MakeChunk ( LFA_FileRef inFileRef, RiffState & inOutRiffState, long riffType, UInt32 len );

} // namespace RIFF_Support

#endif	// __RIFF_Support_hpp__
