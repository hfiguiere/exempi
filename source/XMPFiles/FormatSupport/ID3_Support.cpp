// =================================================================================================
// ADOBE SYSTEMS INCORPORATED
// Copyright 2002-2007 Adobe Systems Incorporated
// All Rights Reserved
//
// NOTICE: Adobe permits you to use, modify, and distribute this file in accordance with the terms
// of the Adobe license agreement accompanying it.
// =================================================================================================

#include "XMP_Environment.h"	// ! This must be the first include.
#include "XMP_Const.h"

#include "ID3_Support.hpp"

#include "UnicodeConversions.hpp"
#include "Reconcile_Impl.hpp"

#if XMP_WinBuild
	#pragma warning ( disable : 4996 )	// '...' was declared deprecated
#endif

// For more information about the id3v2 specification
// Please refer to http://www.id3.org/develop.html

namespace ID3_Support {

	char Genres[128][32]={
		"Blues",			// 0
 		"Classic Rock",		// 1
 		"Country",			// 2
 		"Dance",
 		"Disco",
 		"Funk",
 		"Grunge",
 		"Hip-Hop",
 		"Jazz",				// 8
 		"Metal",
		"New Age",			// 10
 		"Oldies",
 		"Other",			// 12
 		"Pop",
 		"R&B",
 		"Rap",
 		"Reggae",			// 16
 		"Rock",				// 17
 		"Techno",
 		"Industrial",
 		"Alternative",
 		"Ska",
 		"Death Metal",
 		"Pranks",
 		"Soundtrack",		// 24
 		"Euro-Techno",
 		"Ambient",
 		"Trip-Hop",
 		"Vocal",
 		"Jazz+Funk",
 		"Fusion",
 		"Trance",
 		"Classical",		// 32
 		"Instrumental",
 		"Acid",
 		"House",
 		"Game",
 		"Sound Clip",
 		"Gospel",
 		"Noise",
 		"AlternRock",
 		"Bass",
 		"Soul",				//42
 		"Punk",
 		"Space",
 		"Meditative",
 		"Instrumental Pop",
 		"Instrumental Rock",
 		"Ethnic",
 		"Gothic",
 		"Darkwave",
 		"Techno-Industrial",
 		"Electronic",
 		"Pop-Folk",
 		"Eurodance",
 		"Dream",
 		"Southern Rock",
 		"Comedy",
 		"Cult",
 		"Gangsta",
 		"Top 40",
 		"Christian Rap",
 		"Pop/Funk",
 		"Jungle",
 		"Native American",
 		"Cabaret",
 		"New Wave",			// 66
 		"Psychadelic",
 		"Rave",
 		"Showtunes",
 		"Trailer",
 		"Lo-Fi",
 		"Tribal",
 		"Acid Punk",
 		"Acid Jazz",
 		"Polka",
 		"Retro",
 		"Musical",
 		"Rock & Roll",
 		"Hard Rock",
 		"Folk",				// 80
 		"Folk-Rock",
 		"National Folk",
 		"Swing",
 		"Fast Fusion",
 		"Bebob",
 		"Latin",
 		"Revival",
 		"Celtic",
 		"Bluegrass",		// 89
 		"Avantgarde",
 		"Gothic Rock",
 		"Progressive Rock",
 		"Psychedelic Rock",
 		"Symphonic Rock",
 		"Slow Rock",
 		"Big Band",
 		"Chorus",
 		"Easy Listening",
 		"Acoustic",
		"Humour",			// 100
		"Speech",
		"Chanson",
		"Opera",
		"Chamber Music",
		"Sonata",
		"Symphony",
		"Booty Bass",
		"Primus",
		"Porn Groove",
		"Satire",
		"Slow Jam",
		"Club",
		"Tango",
		"Samba",
		"Folklore",
		"Ballad",
		"Power Ballad",
		"Rhythmic Soul",
		"Freestyle",
		"Duet",
		"Punk Rock",
		"Drum Solo",
		"A capella",
		"Euro-House",
		"Dance Hall",
		"Unknown"			// 126
	};

	// Some types
	#ifndef XMP_Int64
		typedef unsigned long long XMP_Int64;
	#endif

	static bool FindXMPFrame(LFA_FileRef inFileRef, XMP_Int64 &posXMP, XMP_Int64 &posPAD, unsigned long &dwExtendedTag, unsigned long &dwLen);
	static unsigned long SkipExtendedHeader(LFA_FileRef inFileRef, XMP_Uns8 bVersion, XMP_Uns8 flag);
	static bool GetFrameInfo(LFA_FileRef inFileRef, XMP_Uns8 bVersion, char *strFrameID, XMP_Uns8 &cflag1, XMP_Uns8 &cflag2, unsigned long &dwSize);
	static bool ReadSize(LFA_FileRef inFileRef, XMP_Uns8 bVersion, unsigned long &dwSize);
	static unsigned long CalculateSize(XMP_Uns8 bVersion, unsigned long dwSizeIn);
	static bool LoadTagHeaderAndUnknownFrames(LFA_FileRef inFileRef, char *strBuffer, bool fRecon, unsigned long &posPad);
	
	#define GetFilePosition(file)	LFA_Seek ( file, 0, SEEK_CUR )

	const unsigned long k_dwTagHeaderSize = 10;
	const unsigned long k_dwFrameHeaderSize = 10;
	const unsigned long k_dwXMPLabelSize = 4; // 4 (size of "XMP\0")

//     ID3v2 flags                %abcd0000
//	   Where:
//		    a - Unsynchronisation
//			b - Extended header
//			c - Experimental indicator
//			d - Footer present
	const unsigned char flagUnsync = 0x80; // (MSb)
	const unsigned char flagExt	= 0x40;
	const unsigned char flagExp = 0x20;
	const unsigned char flagFooter = 0x10;


#ifndef Trace_ID3_Support
	#define Trace_ID3_Support 0
#endif

// =================================================================================================

#if XMP_WinBuild

	#define stricmp _stricmp

#else

	static int stricmp ( const char * left, const char * right )	// Case insensitive ASCII compare.
	{
		char chL = *left;	// ! Allow for 0 passes in the loop (one string is empty).
		char chR = *right;	// ! Return -1 for stricmp ( "a", "Z" ).

		for ( ; (*left != 0) && (*right != 0); ++left, ++right ) {
			chL = *left;
			chR = *right;
			if ( chL == chR ) continue;
			if ( ('A' <= chL) && (chL <= 'Z') ) chL |= 0x20;
			if ( ('A' <= chR) && (chR <= 'Z') ) chR |= 0x20;
			if ( chL != chR ) break;
		}
		
		if ( chL == chR ) return 0;
		if ( chL < chR ) return -1;
		return 1;
		
	}

#endif

// =================================================================================================

// *** Load Scenario:
// - Check for id3v2 tag
//
// - Parse through the frame for "PRIV" + UTF8 Encoding + "XMP"
//
// - If found, load it.
bool GetMetaData ( LFA_FileRef inFileRef, char* buffer, unsigned long* pBufferSize, ::XMP_Int64* fileOffset )
{

	if ( pBufferSize == 0 ) return false;

	unsigned long dwSizeIn = *pBufferSize;
	*pBufferSize = 0;
	XMP_Int64 posXMP = 0ULL, posPAD = 0ULL;
	unsigned long dwLen = 0, dwExtendedTag = 0;
	if ( ! FindXMPFrame ( inFileRef, posXMP, posPAD, dwExtendedTag, dwLen ) ) return false;

	// Found the XMP frame! Get the rest of frame into the buffer
	unsigned long dwXMPBufferLen = dwLen - k_dwXMPLabelSize;
	*pBufferSize = dwXMPBufferLen; 
	
	if ( fileOffset != 0 ) *fileOffset = posXMP + k_dwXMPLabelSize;
	
	if ( buffer != 0 ) {
		// Seek 4 bytes ahead to get the XMP data.
		LFA_Seek ( inFileRef, posXMP+k_dwXMPLabelSize, SEEK_SET );
		if ( dwXMPBufferLen > dwSizeIn ) dwXMPBufferLen = dwSizeIn;
		LFA_Read ( inFileRef, buffer, dwXMPBufferLen );	// Get the XMP frame
	}

	return true;

}

// =================================================================================================

bool FindFrame ( LFA_FileRef inFileRef, char* strFrame, XMP_Int64 & posFrame, unsigned long & dwLen )
{
	// Taking into account that the first Tag is the ID3 tag
	bool fReturn = false;
	LFA_Seek ( inFileRef, 0ULL, SEEK_SET );

	#if Trace_ID3_Support
		fprintf ( stderr, "ID3_Support::FindFrame : Looking for %s\n", strFrame );
	#endif

	// Read the tag name
	char szID[4] = {"xxx"};
	long bytesRead = LFA_Read ( inFileRef, szID, 3 );
	if ( bytesRead == 0 ) return fReturn;

	// Check for "ID3"
	if ( strcmp ( szID, "ID3" ) != 0 ) return fReturn;

	// Read the version, flag and size
	XMP_Uns8 v1 = 0, v2 = 0, flags = 0;
	unsigned long dwTagSize = 0;

	if ( ! GetTagInfo ( inFileRef, v1, v2, flags, dwTagSize ) ) return fReturn;
	if ( dwTagSize == 0 ) return fReturn;
	if ( v1 > 4 ) return fReturn; 	// We don't support anything newer than id3v2 4.0

	// If there's an extended header, ignore it
	XMP_Int32 dwExtendedTag = SkipExtendedHeader(inFileRef, v1, flags);
	dwTagSize -= dwExtendedTag;

	// Enumerate through the frames
	XMP_Int64 posCur = 0ULL;
	posCur = GetFilePosition ( inFileRef );
	XMP_Int64 posEnd = posCur + dwTagSize;

	while ( posCur < posEnd ) {

		char szFrameID[5] = {"xxxx"};
		unsigned long dwFrameSize = 0;
		XMP_Uns8 cflag1 = 0, cflag2 = 0;

		// Get the next frame
		if ( ! GetFrameInfo ( inFileRef, v1, szFrameID, cflag1, cflag2, dwFrameSize ) ) break;

		// Are we in a padding frame?
		if ( dwFrameSize == 0 ) break;

		// Is it the Frame we're looking for?
		if ( strcmp ( szFrameID, strFrame ) == 0 ) {
			posFrame = GetFilePosition ( inFileRef );
			dwLen = dwFrameSize;
			fReturn = true;
			break;
		} else {
			// Jump to the next frame
			LFA_Seek ( inFileRef, dwFrameSize, SEEK_CUR );
		}

		posCur = GetFilePosition ( inFileRef );
	}

	#if Trace_ID3_Support
		if ( fReturn ) {
			fprintf ( stderr, "  Found %s, offset %d, length %d\n", strFrame, (long)posFrame, dwLen );
		}
	#endif

	return fReturn;
}

// =================================================================================================

bool GetFrameData ( LFA_FileRef inFileRef, char* strFrame, char* buffer, unsigned long &dwBufferSize )
{
	char strData[TAG_MAX_SIZE+4];	// Plus 4 for two worst case UTF-16 nul terminators.
	size_t sdPos = 0;	// Offset within strData to the value.
	memset ( &strData[0], 0, sizeof(strData) );

	if ( (buffer == 0) || (dwBufferSize > TAG_MAX_SIZE) ) return false;

	const unsigned long dwSizeIn = dwBufferSize;
	XMP_Int64 posFrame = 0ULL;
	unsigned long dwLen = 0;
	XMP_Uns8 bEncoding = 0;

	// Find the frame
	if ( ! FindFrame ( inFileRef, strFrame, posFrame, dwLen ) ) return false;
	#if Trace_ID3_Support
		fprintf ( stderr, "  Getting frame data\n" );
	#endif
	
	if ( dwLen <= 0 ) {

		dwBufferSize = 1;
		buffer[0] = 0;

	} else {
		
		// Get the value for a typical text frame, having an encoding byte followed by the value.
		// COMM frames are special, see below. First get encoding, and the frame data into strData.

		dwBufferSize = dwLen - 1;	// Don't count the encoding byte.
		
		// Seek to the frame
		LFA_Seek  (  inFileRef, posFrame, SEEK_SET  );

		// Read the Encoding
		LFA_Read ( inFileRef, &bEncoding, 1 );
		if ( bEncoding > 3 ) return false;

		// Get the frame
		if ( dwBufferSize > dwSizeIn ) dwBufferSize = dwSizeIn;

		if ( dwBufferSize >= TAG_MAX_SIZE ) return false;	// No room for data.
		LFA_Read ( inFileRef, &strData[0], dwBufferSize );

		if ( strcmp ( strFrame, "COMM" ) == 0 ) {
		
			// A COMM frame has a 3 byte language tag, then an encoded and nul terminated description
			// string, then the encoded value string. Set dwOffset to the offset to the value.

			unsigned long dwOffset = 3;	// Skip the 3 byte language code.

			if ( (bEncoding == 0) || (bEncoding == 3) ) {
				dwOffset += (strlen ( &strData[3] ) + 1);	// Skip the descriptor and nul.
			} else {
				UTF16Unit* u16Ptr = (UTF16Unit*) (&strData[3]);
				for ( ; *u16Ptr != 0; ++u16Ptr ) dwOffset += 2;	// Skip the descriptor.
				dwOffset += 2;	// Skip the nul also.
			}

			if ( dwOffset >= dwBufferSize ) return false;
			dwBufferSize -= dwOffset;

			sdPos = dwOffset;

			#if Trace_ID3_Support
				fprintf ( stderr, "    COMM frame, dwOffset %d\n", dwOffset );
			#endif

		}

		// Encoding translation
		switch ( bEncoding ) {

			case 1:	// UTF-16 with a BOM. (Might be missing for empty string.)
			case 2:	// Big endian UTF-16 with no BOM.
				{
					bool bigEndian = true;	// Assume big endian if no BOM.
					UTF16Unit* u16Ptr = (UTF16Unit*) &strData[sdPos];

					if ( GetUns16BE ( u16Ptr ) == 0xFEFF ) {
						++u16Ptr;	// Don't translate the BOM.
					} else if ( GetUns16BE ( u16Ptr ) == 0xFFFE ) {
						bigEndian = false;
						++u16Ptr;	// Don't translate the BOM.
					}
					
					size_t u16Len = 0;	// Count the UTF-16 units, not bytes.
					for ( UTF16Unit* temp = u16Ptr; *temp != 0; ++temp ) ++u16Len;

					std::string utf8Str;
					FromUTF16 ( u16Ptr, u16Len, &utf8Str, bigEndian );
					if ( utf8Str.size() >= (sizeof(strData) - sdPos) ) return false;
					strcpy ( &strData[sdPos], utf8Str.c_str() );	// AUDIT: Protected by the above check.
				}
				break;

			case 0: // ISO Latin-1 (8859-1).
				{
					std::string utf8Str;
					char* localPtr  = &strData[sdPos];
					size_t localLen = dwBufferSize;

					ReconcileUtils::Latin1ToUTF8 ( localPtr, localLen, &utf8Str );
					if ( utf8Str.size() >= (sizeof(strData) - sdPos) ) return false;
					strcpy ( &strData[sdPos], utf8Str.c_str() );	// AUDIT: Protected by the above check.
				}
				break;

			case 3: // UTF-8
			default:
				// Handled appropriately
				break;

		}

		char * strTemp = &strData[sdPos];
		
		if ( strcmp ( strFrame, "TCON" ) == 0 ) {

			char str[TAG_MAX_SIZE];
			str[0] = 0;
			if ( strlen ( &strData[sdPos] ) >= sizeof(str) ) return false;
			strcpy ( str, &strData[sdPos] );	// AUDIT: Protected by the above check.

			#if Trace_ID3_Support
				fprintf ( stderr, "    TCON frame, first char '%c'\n", str[0] );
			#endif

			// Genre: let's get the "string" value
			if ( str[0] == '(' ) {
				int iGenre = atoi(str+1);
				if ( (iGenre > 0) && (iGenre < 127) ) {
					strTemp = Genres[iGenre];
				} else {
					strTemp = Genres[12];
				}
			} else {
				// Text, let's "try" to find it anyway
				int i = 0;
				for ( i=0; i < 127; ++i ) {
					if ( stricmp ( str, Genres[i] ) == 0 ) {
						strTemp = Genres[i]; // Found, let's use the one in the list
						break;
					}
				}
				if ( i == 127 ) strTemp = Genres[12]; // Not found
			}

		}

		#if Trace_ID3_Support
			fprintf ( stderr, "    Have data, length %d, \"%s\"\n", strlen(strTemp), strTemp );
		#endif

		if ( strlen(strTemp) >= dwSizeIn ) return false;
		strcpy ( buffer, strTemp );	// AUDIT: Protected by the above check.

	}

	return true;

}

// =================================================================================================

bool AddXMPTagToID3Buffer ( char * strCur, unsigned long * pdwCurOffset, unsigned long dwMaxSize, XMP_Uns8 bVersion,
							char *strFrameName, const char * strXMPTagTemp, unsigned long dwXMPLengthTemp )
{
	char strGenre[64];
	const char * strXMPTag = strXMPTagTemp;
	XMP_Int32 dwCurOffset = *pdwCurOffset;
	XMP_Uns8 bEncoding = 0;
	long dwXMPLength = dwXMPLengthTemp;

	if ( dwXMPLength == 0 ) return false;

	if ( strcmp ( strFrameName, "TCON" ) == 0 ) {

		// Genre: we need to get the number back...
		int iFound = 12;

		for ( int i=0; i < 127; ++i ) {
			if ( stricmp ( strXMPTag, Genres[i] ) == 0 ) {
				iFound = i; // Found
				break;
			}
		}

		snprintf ( strGenre, sizeof(strGenre), "(%d)", iFound );	// AUDIT: Using sizeof(strGenre) is safe.
		strXMPTag = strGenre;
		dwXMPLength = strlen(strXMPTag);

	}

	// Stick with the ID3v2.3 encoding choices, they are a proper subset of ID3v2.4.
	//	0 - ISO Latin-1
	//	1 - UTF-16 with BOM
	// For 3rd party reliability we always write UTF-16 as little endian. For example, Windows
	// Media Player fails to honor the BOM, it assumes little endian.

	std::string tempLatin1, tempUTF8;
	ReconcileUtils::UTF8ToLatin1 ( strXMPTag, dwXMPLength, &tempLatin1 );
	ReconcileUtils::Latin1ToUTF8 ( tempLatin1.data(), tempLatin1.size(), &tempUTF8 );
	if ( ((size_t)dwXMPLength != tempUTF8.size()) || (memcmp ( strXMPTag, tempUTF8.data(), dwXMPLength ) != 0) ) {
		bEncoding = 1;	// Will convert to UTF-16 later.
	} else {
		strXMPTag   = tempLatin1.c_str();	// Use the Latin-1 encoding for output.
		dwXMPLength = tempLatin1.size();
	}

	std::string strUTF16;
	if ( bEncoding == 1 ) {
		ToUTF16 ( (UTF8Unit*)strXMPTag, dwXMPLength, &strUTF16, false /* little endian */ );
		dwXMPLength = strUTF16.size() + 2;	// ! Include the (to be inserted) BOM in the count.
	}

	//		Frame Structure
    //		Frame ID      $xx xx xx xx  (four characters)
    //		Size      4 * %0xxxxxxx     <<--- IMPORTANT NOTE: This is true only in v4.0 (v3.0 uses a UInt32)
    //		Flags         $xx xx
	//		Encoding	  $xx (Not included in the frame header)
	//		Special case: "COMM" which we have to include "XXX\0" in front of it (also not included in the frame header)

	unsigned long dwFrameSize = dwXMPLength + 1; // 1 == Encoding;

	bool fCOMM = (strcmp ( strFrameName, "COMM" ) == 0);
	if ( fCOMM ) {
		dwFrameSize += 3;	// The "XXX" language part.
		dwFrameSize += ((bEncoding == 0) ? 1 : 4 );	// The empty descriptor string.
	}
	
	if ( (dwCurOffset + k_dwFrameHeaderSize + dwFrameSize) > dwMaxSize ) return false;

	unsigned long dwCalculated = CalculateSize ( bVersion, dwFrameSize );

	// FrameID
	if ( (dwMaxSize - dwCurOffset) < 4 ) return false;
	memcpy ( strCur+dwCurOffset, strFrameName, 4 );	// AUDIT: Protected by the above check.
	dwCurOffset += 4;

	// Frame Size - written as big endian
	strCur[dwCurOffset] = (char)(dwCalculated >> 24);
	++dwCurOffset;
	strCur[dwCurOffset] = (char)((dwCalculated >> 16) & 0xFF);
	++dwCurOffset;
	strCur[dwCurOffset] = (char)((dwCalculated >> 8) & 0xFF);
	++dwCurOffset;
	strCur[dwCurOffset] = (char)(dwCalculated & 0xFF);
	++dwCurOffset;

	// Flags
	strCur[dwCurOffset] = 0;
	++dwCurOffset;
	strCur[dwCurOffset] = 0;
	++dwCurOffset;

	// Encoding
	strCur[dwCurOffset] = bEncoding;
	++dwCurOffset;

	// COMM extras: XXX language and empty encoded descriptor string.
	if ( fCOMM ) {
		if ( (dwMaxSize - dwCurOffset) < 3 ) return false;
		memcpy ( strCur+dwCurOffset, "XXX", 3 );	// AUDIT: Protected by the above check.
		dwCurOffset += 3;
		if ( bEncoding == 0 ) {
			strCur[dwCurOffset] = 0;
			++dwCurOffset;
		} else {
			strCur[dwCurOffset] = 0xFF;
			++dwCurOffset;
			strCur[dwCurOffset] = 0xFE;
			++dwCurOffset;
			strCur[dwCurOffset] = 0;
			++dwCurOffset;
			strCur[dwCurOffset] = 0;
			++dwCurOffset;
		}
	}

	if ( bEncoding == 1 ) {
		// Add the BOM "FFFE"
		strCur[dwCurOffset] = 0xFF;
		++dwCurOffset;
		strCur[dwCurOffset] = 0xFE;
		++dwCurOffset;
		dwXMPLength -= 2;	// The BOM was included above.
		// Copy the Unicode data
		if ( (long)(dwMaxSize - dwCurOffset) < dwXMPLength ) return false;
		memcpy ( strCur+dwCurOffset, strUTF16.data(), dwXMPLength );	// AUDIT: Protected by the above check.
		dwCurOffset += dwXMPLength;
	} else {
		// Copy the data
		if ( (long)(dwMaxSize - dwCurOffset) < dwXMPLength ) return false;
		memcpy ( strCur+dwCurOffset, strXMPTag, dwXMPLength );	// AUDIT: Protected by the above check.
		dwCurOffset += dwXMPLength;
	}

	*pdwCurOffset = dwCurOffset;

	return true;

}

// =================================================================================================

static void OffsetAudioData ( LFA_FileRef inFileRef, XMP_Int64 audioOffset, XMP_Int64 oldAudioBase )
{
	enum { kBuffSize = 64*1024 };
	XMP_Uns8 buffer [kBuffSize];

	const XMP_Int64 posEOF = LFA_Measure ( inFileRef );
	XMP_Int64 posCurrentCopy;	// ! Must be a signed type!

	posCurrentCopy = posEOF;
	while ( posCurrentCopy >= (oldAudioBase + kBuffSize) ) {
		posCurrentCopy -= kBuffSize;	// *** Xcode 2.3 seemed to generate bad code using a for loop.
		LFA_Seek ( inFileRef, posCurrentCopy, SEEK_SET );
		LFA_Read ( inFileRef, buffer, kBuffSize );
		LFA_Seek ( inFileRef, (posCurrentCopy + audioOffset), SEEK_SET );
		LFA_Write ( inFileRef, buffer, kBuffSize );
	}
	
	if ( posCurrentCopy != oldAudioBase ) {
		XMP_Uns32 remainder = (XMP_Uns32) (posCurrentCopy - oldAudioBase);
		XMP_Assert ( remainder < kBuffSize );
		LFA_Seek ( inFileRef, oldAudioBase, SEEK_SET );
		LFA_Read ( inFileRef, buffer, remainder );
		LFA_Seek ( inFileRef, (oldAudioBase + audioOffset), SEEK_SET );
		LFA_Write ( inFileRef, buffer, remainder );
	}

}

// =================================================================================================

bool SetMetaData ( LFA_FileRef inFileRef, char* strXMPPacket, unsigned long dwXMPPacketSize,
                   char* strLegacyFrames, unsigned long dwFullLegacySize, bool fRecon )
{
	// The ID3 section layout:
	//	ID3 header, 10 bytes
	//	Unrecognized ID3 frames
	//	Legacy ID3 metadata frames (artist, album, genre, etc.)
	//	XMP frame, content is "XMP\0" plus the packet
	//	padding

	// ID3 Buffer vars
	const unsigned long kiMaxBuffer = 100*1000;
	char szID3Buffer [kiMaxBuffer];	// Must be enough for the ID3 header, unknown ID3 frames, and legacy ID3 metadata.
	unsigned long id3BufferLen = 0;	// The amount of stuff currently in the buffer.

	unsigned long dwOldID3ContentSize = 0;	// The size of the existing ID3 content (not counting the header).
	unsigned long dwNewID3ContentSize = 0;	// The size of the updated ID3 content (not counting the header).
	
	unsigned long newPadSize = 0;

	XMP_Uns8 bMajorVersion = 3;

	bool fFoundID3 = FindID3Tag ( inFileRef, dwOldID3ContentSize, bMajorVersion );
	if ( (bMajorVersion > 4) || (bMajorVersion < 3) ) return false;	// Not supported

	// Now that we know the version of the ID3 tag, let's format the size of the XMP frame.

	#define k_XMPPrefixSize	(k_dwFrameHeaderSize + k_dwXMPLabelSize)
	char szXMPPrefix [k_XMPPrefixSize] = { 'P', 'R', 'I', 'V', 0, 0, 0, 0, 0, 0, 'X', 'M', 'P', 0 };
	unsigned long dwXMPContentSize = k_dwXMPLabelSize + dwXMPPacketSize;
	unsigned long dwFullXMPFrameSize = k_dwFrameHeaderSize + dwXMPContentSize;

	unsigned long dwFormattedTemp = CalculateSize ( bMajorVersion, dwXMPContentSize );

	szXMPPrefix[4] = (char)(dwFormattedTemp >> 24);
	szXMPPrefix[5] = (char)((dwFormattedTemp >> 16) & 0xFF);
	szXMPPrefix[6] = (char)((dwFormattedTemp >> 8) & 0xFF);
	szXMPPrefix[7] = (char)(dwFormattedTemp & 0xFF);

	// Set up the ID3 buffer with the ID3 header and any existing unrecognized ID3 frames.

	if ( ! fFoundID3 ) {

		// Case 1 - No id3v2 tag: Create the tag with the XMP frame.
		// Create the tag
		//     ID3v2/file identifier      "ID3"
		//     ID3v2 version              $03 00
		//     ID3v2 flags                %abcd0000
		//     ID3v2 size             4 * %0xxxxxxx

		XMP_Assert ( dwOldID3ContentSize == 0 );
		
		char szID3Header [k_dwTagHeaderSize] = { 'I', 'D', '3', 3, 0, 0, 0, 0, 0, 0 };

		// Copy the ID3 header
		if ( sizeof(szID3Buffer) < k_dwTagHeaderSize ) return false;
		memcpy ( szID3Buffer, szID3Header, k_dwTagHeaderSize );	// AUDIT: Protected by the above check.
		id3BufferLen = k_dwTagHeaderSize;

		newPadSize = 100;
		dwNewID3ContentSize = dwFullLegacySize + dwFullXMPFrameSize + newPadSize;

	} else {
 
		// Case 2 - id3v2 tag is present
		// 1. Copy all the unknown tags
		// 2. Make the rest padding (to be used right there).

		if ( (k_dwFrameHeaderSize + dwOldID3ContentSize) > kiMaxBuffer ) {
			// The ID3Buffer is not big enough to fit the id3v2 tag... let's bail...
			return false;
		}

		LoadTagHeaderAndUnknownFrames ( inFileRef, szID3Buffer, fRecon, id3BufferLen );
		
		unsigned long spareLen = (k_dwFrameHeaderSize + dwOldID3ContentSize) - id3BufferLen;
		
		if ( spareLen >= (dwFullLegacySize + dwFullXMPFrameSize) ) {
		
			// The exising ID3 header can hold the update.
			dwNewID3ContentSize = dwOldID3ContentSize;
			newPadSize = spareLen - (dwFullLegacySize + dwFullXMPFrameSize);
		
		} else {
		
			// The existing ID3 header is too small, it will have to grow.
			newPadSize = 100;
			dwNewID3ContentSize = (id3BufferLen - k_dwTagHeaderSize) +
								  dwFullLegacySize + dwFullXMPFrameSize + newPadSize;
		
		}

	}
	
	// Move the audio data if the ID3 frame is new or has to grow.
	
	XMP_Assert ( dwNewID3ContentSize >= dwOldID3ContentSize );

	if ( dwNewID3ContentSize > dwOldID3ContentSize ) {
		unsigned long audioOffset = dwNewID3ContentSize - dwOldID3ContentSize;
		unsigned long oldAudioBase = k_dwTagHeaderSize + dwOldID3ContentSize;
		if ( ! fFoundID3 ) {
			// We're injecting an entire ID3 section.
			audioOffset = k_dwTagHeaderSize + dwNewID3ContentSize;
			oldAudioBase = 0;
		}
		OffsetAudioData ( inFileRef, audioOffset, oldAudioBase );
	}

	// Set the new size for the ID3 content. This always uses the 4x7 format.
	
	dwFormattedTemp = CalculateSize ( 4, dwNewID3ContentSize );
	szID3Buffer[6] = (char)(dwFormattedTemp >> 24);
	szID3Buffer[7] = (char)((dwFormattedTemp >> 16) & 0xFF);
	szID3Buffer[8] = (char)((dwFormattedTemp >> 8) & 0xFF);
	szID3Buffer[9] = (char)(dwFormattedTemp & 0xFF);

	// Write the partial ID3 buffer (ID3 header plus unknown tags)
	LFA_Seek ( inFileRef, 0, SEEK_SET );
	LFA_Write ( inFileRef, szID3Buffer, id3BufferLen );

	// Append the new legacy metadata frames
	if ( dwFullLegacySize > 0 ) {
		LFA_Write ( inFileRef, strLegacyFrames, dwFullLegacySize );
	}

	// Append the XMP frame prefix
	LFA_Write ( inFileRef, szXMPPrefix, k_XMPPrefixSize );

	// Append the XMP packet
	LFA_Write ( inFileRef, strXMPPacket, dwXMPPacketSize );

	// Append the padding.
	if ( newPadSize > 0 ) {
		std::string szPad;
		szPad.reserve ( newPadSize );
		szPad.assign ( newPadSize, '\0' );
		LFA_Write ( inFileRef, const_cast<char *>(szPad.data()), newPadSize );
	}

	LFA_Flush ( inFileRef );

	return true;

}

// =================================================================================================

bool LoadTagHeaderAndUnknownFrames ( LFA_FileRef inFileRef, char * strBuffer, bool fRecon, unsigned long & posPad )
{

	LFA_Seek ( inFileRef, 3ULL, SEEK_SET );	// Point after the "ID3"

	// Get the tag info
	unsigned long dwOffset = 0;
	XMP_Uns8 v1 = 0, v2 = 0, flags = 0;
	unsigned long dwTagSize = 0;
	GetTagInfo ( inFileRef, v1, v2, flags, dwTagSize );

	unsigned long dwExtendedTag = SkipExtendedHeader ( inFileRef, v1, flags );

	LFA_Seek ( inFileRef, 0ULL, SEEK_SET );
	LFA_Read ( inFileRef, strBuffer, k_dwTagHeaderSize );
	dwOffset += k_dwTagHeaderSize;

	// Completely ignore the Extended Header
	if ( ((flags & flagExt) == flagExt) && (dwExtendedTag > 0) ) {
		strBuffer[5] = strBuffer[5] & 0xBF;	// If the flag has been set, let's reset it
		LFA_Seek ( inFileRef, dwExtendedTag, SEEK_CUR );	// And let's seek up to after the extended header
	}

	// Enumerate through the frames
	XMP_Int64 posCur = 0ULL;
	posCur = GetFilePosition ( inFileRef );
	XMP_Int64 posEnd = posCur + dwTagSize;

	while ( posCur < posEnd ) {

		char szFrameID[5] = {"xxxx"};
		unsigned long dwFrameSize = 0;
		XMP_Uns8 cflag1 = 0, cflag2 = 0;

		// Get the next frame
		if ( ! GetFrameInfo ( inFileRef, v1, szFrameID, cflag1, cflag2, dwFrameSize ) ) break;

		// Are we in a padding frame?
		if ( dwFrameSize == 0 ) break;	// We just hit a padding frame

		bool fIgnore = false;
		bool knownID = (strcmp ( szFrameID, "TIT2" ) == 0) ||
					   (strcmp ( szFrameID, "TYER" ) == 0) ||
					   (strcmp ( szFrameID, "TDRV" ) == 0) ||
					   (strcmp ( szFrameID, "TPE1" ) == 0) ||
					   (strcmp ( szFrameID, "TALB" ) == 0) ||
					   (strcmp ( szFrameID, "TCON" ) == 0) ||
					   (strcmp ( szFrameID, "COMM" ) == 0) ||
					   (strcmp ( szFrameID, "TRCK" ) == 0);

		// If a known frame, just ignore
		// Note: If recon is turned off, let's consider all known frames as unknown
		if ( knownID && fRecon ) {

			fIgnore = true;

		} else if ( strcmp ( szFrameID, "PRIV" ) == 0 ) {

			// Read the "PRIV" frame
		    //		<Header for "PRIV">
		    //		Short content descrip. <text string according to encoding> $00 (00)
		    //		The actual data        <full text string according to encoding>

			// Get the PRIV descriptor
			char szXMPTag[4] = {"xxx"};
			if ( LFA_Read ( inFileRef, &szXMPTag, k_dwXMPLabelSize ) != 0 ) {
				// Is it a XMP "PRIV"
				if ( (szXMPTag[3] == 0) && (strcmp ( szXMPTag, "XMP" ) == 0) ) fIgnore = true;
				LFA_Seek ( inFileRef, -(long)k_dwXMPLabelSize, SEEK_CUR );
			}

		}

		if ( fIgnore ) {
			LFA_Seek ( inFileRef, dwFrameSize, SEEK_CUR );
		} else {
			// Unknown frame, let's copy it
			LFA_Seek ( inFileRef, -(long)k_dwFrameHeaderSize, SEEK_CUR );
			LFA_Read ( inFileRef, strBuffer+dwOffset, dwFrameSize+k_dwFrameHeaderSize );
			dwOffset += dwFrameSize+k_dwFrameHeaderSize;
		}

		posCur = GetFilePosition ( inFileRef );

	}

	posPad = dwOffset;

	return true;

}

// =================================================================================================

bool FindID3Tag ( LFA_FileRef inFileRef, unsigned long & dwLen, XMP_Uns8 & bMajorVer )
{
	// id3v2 tag:
	//     ID3v2/file identifier      "ID3"
	//     ID3v2 version              $04 00
	//     ID3v2 flags                %abcd0000
	//     ID3v2 size             4 * %0xxxxxxx

	// Taking into account that the first Tag is the ID3 tag
	LFA_Seek ( inFileRef, 0ULL, SEEK_SET );

	// Read the tag name
	char szID[4] = {"xxx"};
	long bytesRead = LFA_Read ( inFileRef, szID, 3 );
	if ( bytesRead == 0 ) return false;

	// Check for "ID3"
	if ( strcmp ( szID, "ID3" ) != 0 ) return false;

	// Read the version, flag and size
	XMP_Uns8 v2 = 0, flags = 0;
	if ( ! GetTagInfo ( inFileRef, bMajorVer, v2, flags, dwLen ) ) return false;

	return true;

}

// =================================================================================================

bool GetTagInfo ( LFA_FileRef inFileRef, XMP_Uns8 & v1, XMP_Uns8 & v2, XMP_Uns8 & flags, unsigned long & dwTagSize )
{

	if ((LFA_Read(inFileRef, &v1, 1)) == 0) return false;
	if ((LFA_Read(inFileRef, &v2, 1)) == 0) return false;
	if ((LFA_Read(inFileRef, &flags, 1)) == 0) return false;
	if (!ReadSize(inFileRef, 4, dwTagSize)) return false; // Tag size is always using the size reading method.
	
	return true;

}

// =================================================================================================

static bool FindXMPFrame ( LFA_FileRef inFileRef, XMP_Int64 & posXMP, XMP_Int64 & posPAD, unsigned long & dwExtendedTag, unsigned long & dwLen )
{
	// Taking into account that the first Tag is the ID3 tag
	bool fReturn = false;
	dwExtendedTag = 0;
	posPAD = 0;

	LFA_Seek ( inFileRef, 0ULL, SEEK_SET );

	// Read the tag name
	char szID[4] = {"xxx"};
	long bytesRead = LFA_Read ( inFileRef, szID, 3 );
	if ( bytesRead == 0 ) return fReturn;

	// Check for "ID3"
	if ( strcmp ( szID, "ID3") != 0 ) return fReturn;

	// Read the version, flag and size
	XMP_Uns8 v1 = 0, v2 = 0, flags = 0;
	unsigned long dwTagSize = 0;
	if ( ! GetTagInfo ( inFileRef, v1, v2, flags, dwTagSize ) ) return fReturn;
	if ( dwTagSize == 0 ) return fReturn;
	if ( v1 > 4 ) return fReturn; 	// We don't support anything newer than id3v2 4.0

	// If there's an extended header, ignore it
	dwExtendedTag = SkipExtendedHeader(inFileRef, v1, flags);
	dwTagSize -= dwExtendedTag;

	// Enumerate through the frames
	XMP_Int64 posCur = 0ULL;
	posCur = GetFilePosition ( inFileRef );
	XMP_Int64 posEnd = posCur + dwTagSize;

	while ( posCur < posEnd ) {

		char szFrameID[5] = {"xxxx"};
		unsigned long dwFrameSize = 0;
		XMP_Uns8 cflag1 = 0, cflag2 = 0;

		// Get the next frame
		if ( ! GetFrameInfo ( inFileRef, v1, szFrameID, cflag1, cflag2, dwFrameSize ) ) {
			// Set the file pointer to the XMP or the start
			LFA_Seek ( inFileRef, fReturn ? posXMP : 0ULL, SEEK_SET );
			break;
		}

		// Are we in a padding frame?
		if ( dwFrameSize == 0 ) {

			// We just hit a padding frame
			LFA_Seek ( inFileRef, -(long)k_dwFrameHeaderSize, SEEK_CUR );
			posPAD = GetFilePosition ( inFileRef );

			// Set the file pointer to the XMP or the start
			LFA_Seek ( inFileRef, fReturn ? posXMP : 0ULL, SEEK_SET );
			break;

		}

		// Is it a "PRIV"?
		if ( strcmp(szFrameID, "PRIV") != 0 ) {

			// Jump to the next frame
			LFA_Seek ( inFileRef, dwFrameSize, SEEK_CUR );

		} else {

			// Read the "PRIV" frame
		    //		<Header for "PRIV">
		    //		Short content descrip. <text string according to encoding> $00 (00)
		    //		The actual data        <full text string according to encoding>

			unsigned long dwBytesRead = 0;

			// Get the PRIV descriptor
			char szXMPTag[4] = {"xxx"};
			if (LFA_Read(inFileRef, &szXMPTag, k_dwXMPLabelSize) == 0) return fReturn;
			dwBytesRead += k_dwXMPLabelSize;

			// Is it a XMP "PRIV"
			if ( (szXMPTag[3] == 0) && (strcmp ( szXMPTag, "XMP" ) == 0) ) {
				dwLen = dwFrameSize;
				LFA_Seek ( inFileRef, -(long)k_dwXMPLabelSize, SEEK_CUR );
				posXMP = GetFilePosition ( inFileRef );
				fReturn = true;
				dwBytesRead -= k_dwXMPLabelSize;
			}

			// Didn't find it, let skip the rest of the frame and continue
			LFA_Seek ( inFileRef, dwFrameSize - dwBytesRead, SEEK_CUR );

		}

		posCur = GetFilePosition ( inFileRef );

	}

	return fReturn;

}

// =================================================================================================

// Returns the size of the extended header
static unsigned long SkipExtendedHeader ( LFA_FileRef inFileRef, XMP_Uns8 bVersion, XMP_Uns8 flags )
{
	if ( flags & flagExt ) {

		unsigned long dwExtSize = 0; // <-- This will include the size (full extended header size)

		if ( ReadSize ( inFileRef, bVersion, dwExtSize ) ) {
			if ( bVersion < 4 ) dwExtSize += 4;	// v3 doesn't include the size, while v4 does.
			LFA_Seek ( inFileRef, (size_t)(dwExtSize - 4), SEEK_CUR );
		}

		return dwExtSize;

	}

	return 0;

}

// =================================================================================================

static bool GetFrameInfo ( LFA_FileRef inFileRef, XMP_Uns8 bVersion, char * strFrameID, XMP_Uns8 & cflag1, XMP_Uns8 & cflag2, unsigned long & dwSize)
{
    //		Frame ID      $xx xx xx xx  (four characters)
    //		Size      4 * %0xxxxxxx                          <<--- IMPORTANT NOTE: This is true only in v4.0 (v3.0 uses a XMP_Int32)
    //		Flags         $xx xx

	if ( strFrameID == 0 ) return false;

	if ( LFA_Read ( inFileRef, strFrameID, 4 ) == 0 ) return false;
	if ( ! ReadSize ( inFileRef, bVersion, dwSize ) ) return false;
	if ( LFA_Read ( inFileRef, &cflag1, 1 ) == 0 ) return false;
	if ( LFA_Read ( inFileRef, &cflag2, 1 ) == 0 ) return false;

	return true;

}

// =================================================================================================

static bool ReadSize ( LFA_FileRef inFileRef, XMP_Uns8 bVersion, unsigned long & dwSize )
{
	char s4 = 0, s3 = 0, s2 = 0, s1 = 0;

	if ( LFA_Read ( inFileRef, &s4, 1 ) == 0 ) return false;
	if ( LFA_Read ( inFileRef, &s3, 1 ) == 0 ) return false;
	if ( LFA_Read ( inFileRef, &s2, 1 ) == 0 ) return false;
	if ( LFA_Read ( inFileRef, &s1, 1 ) == 0 ) return false;

	if ( bVersion > 3 ) {
		dwSize = ((s4 & 0x7f) << 21) | ((s3 & 0x7f) << 14) | ((s2 & 0x7f) << 7) | (s1 & 0x7f);
	} else {
		dwSize = ((s4 << 24) | (s3 << 16) | (s2 << 8) | s1);
	}

	return true;

}

// =================================================================================================

static unsigned long CalculateSize ( XMP_Uns8 bVersion, unsigned long dwSizeIn )
{
	unsigned long dwReturn;

	if ( bVersion <= 3 ) {
		dwReturn = dwSizeIn;
	} else {
		dwReturn = dwSizeIn & 0x7f;	// Expand to 7 bits per byte.
		dwSizeIn = dwSizeIn >> 7;
		dwReturn |= ((dwSizeIn & 0x7f) << 8);
		dwSizeIn = dwSizeIn >> 7;
		dwReturn |= ((dwSizeIn & 0x7f) << 16);
		dwSizeIn = dwSizeIn >> 7;
		dwReturn |= ((dwSizeIn & 0x7f) << 24);
	}

	return dwReturn;

}

} // namespace ID3_Support
