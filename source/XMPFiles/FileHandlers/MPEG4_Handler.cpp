// =================================================================================================
// ADOBE SYSTEMS INCORPORATED
// Copyright 2002-2007 Adobe Systems Incorporated
// All Rights Reserved
//
// NOTICE: Adobe permits you to use, modify, and distribute this file in accordance with the terms
// of the Adobe license agreement accompanying it.
// =================================================================================================

#include "MPEG4_Handler.hpp"

#include "UnicodeConversions.hpp"
#include "MD5.h"

#if XMP_WinBuild
	#pragma warning ( disable : 4996 )	// '...' was declared deprecated
#endif

using namespace std;

// =================================================================================================
/// \file MPEG4_Handler.cpp
/// \brief File format handler for MPEG-4, a flavor of the ISO Base Media File Format.
///
/// This handler ...
///
// =================================================================================================

// File and box type codes as big endian 32-bit integers, allows faster comparisons.

static XMP_Uns32 kBE_ftyp = MakeUns32BE ( 0x66747970UL );	// File header Box, no version/flags.

static XMP_Uns32 kBE_mp41 = MakeUns32BE ( 0x6D703431UL );	// Compatibility codes
static XMP_Uns32 kBE_mp42 = MakeUns32BE ( 0x6D703432UL );
static XMP_Uns32 kBE_f4v  = MakeUns32BE ( 0x66347620UL );

static XMP_Uns32 kBE_moov = MakeUns32BE ( 0x6D6F6F76UL );	// Container Box, no version/flags.
static XMP_Uns32 kBE_mvhd = MakeUns32BE ( 0x6D766864UL );	// Data FullBox, has version/flags.
static XMP_Uns32 kBE_udta = MakeUns32BE ( 0x75647461UL );	// Container Box, no version/flags.
static XMP_Uns32 kBE_cprt = MakeUns32BE ( 0x63707274UL );	// Data FullBox, has version/flags.
static XMP_Uns32 kBE_uuid = MakeUns32BE ( 0x75756964UL );	// Data Box, no version/flags.
static XMP_Uns32 kBE_free = MakeUns32BE ( 0x66726565UL );	// Free space Box, no version/flags.
static XMP_Uns32 kBE_mdat = MakeUns32BE ( 0x6D646174UL );	// Media data Box, no version/flags.

static XMP_Uns32 kBE_xmpUUID [4] = { MakeUns32BE ( 0xBE7ACFCBUL ),
									 MakeUns32BE ( 0x97A942E8UL ),
									 MakeUns32BE ( 0x9C719994UL ),
									 MakeUns32BE ( 0x91E3AFACUL ) };

// ! The buffer and constants are both already big endian.
#define Get4CharCode(buffPtr) (*((XMP_Uns32*)(buffPtr)))

// =================================================================================================

// Pairs of 3 letter ISO 639-2 codes mapped to 2 letter ISO 639-1 codes from:
//   http://www.loc.gov/standards/iso639-2/php/code_list.php
// Would have to write an "==" operator to use std::map, must compare values not pointers.
// ! Not fully sorted, do not use a binary search.
static XMP_StringPtr kKnownLangs[] =
	{ "aar", "aa", "abk", "ab", "afr", "af", "aka", "ak", "alb", "sq", "sqi", "sq", "amh", "am",
	  "ara", "ar", "arg", "an", "arm", "hy", "hye", "hy", "asm", "as", "ava", "av", "ave", "ae",
	  "aym", "ay", "aze", "az", "bak", "ba", "bam", "bm", "baq", "eu", "eus", "eu", "bel", "be",
	  "ben", "bn", "bih", "bh", "bis", "bi", "bod", "bo", "tib", "bo", "bos", "bs", "bre", "br",
	  "bul", "bg", "bur", "my", "mya", "my", "cat", "ca", "ces", "cs", "cze", "cs", "cha", "ch",
	  "che", "ce", "chi", "zh", "zho", "zh", "chu", "cu", "chv", "cv", "cor", "kw", "cos", "co",
	  "cre", "cr", "cym", "cy", "wel", "cy", "cze", "cs", "ces", "cs", "dan", "da", "deu", "de",
	  "ger", "de", "div", "dv", "dut", "nl", "nld", "nl", "dzo", "dz", "ell", "el", "gre", "el",
	  "eng", "en", "epo", "eo", "est", "et", "eus", "eu", "baq", "eu", "ewe", "ee", "fao", "fo",
	  "fas", "fa", "per", "fa", "fij", "fj", "fin", "fi", "fra", "fr", "fre", "fr", "fre", "fr",
	  "fra", "fr", "fry", "fy", "ful", "ff", "geo", "ka", "kat", "ka", "ger", "de", "deu", "de",
	  "gla", "gd", "gle", "ga", "glg", "gl", "glv", "gv", "gre", "el", "ell", "el", "grn", "gn",
	  "guj", "gu", "hat", "ht", "hau", "ha", "heb", "he", "her", "hz", "hin", "hi", "hmo", "ho",
	  "hrv", "hr", "scr", "hr", "hun", "hu", "hye", "hy", "arm", "hy", "ibo", "ig", "ice", "is",
	  "isl", "is", "ido", "io", "iii", "ii", "iku", "iu", "ile", "ie", "ina", "ia", "ind", "id",
	  "ipk", "ik", "isl", "is", "ice", "is", "ita", "it", "jav", "jv", "jpn", "ja", "kal", "kl",
	  "kan", "kn", "kas", "ks", "kat", "ka", "geo", "ka", "kau", "kr", "kaz", "kk", "khm", "km",
	  "kik", "ki", "kin", "rw", "kir", "ky", "kom", "kv", "kon", "kg", "kor", "ko", "kua", "kj",
	  "kur", "ku", "lao", "lo", "lat", "la", "lav", "lv", "lim", "li", "lin", "ln", "lit", "lt",
	  "ltz", "lb", "lub", "lu", "lug", "lg", "mac", "mk", "mkd", "mk", "mah", "mh", "mal", "ml",
	  "mao", "mi", "mri", "mi", "mar", "mr", "may", "ms", "msa", "ms", "mkd", "mk", "mac", "mk",
	  "mlg", "mg", "mlt", "mt", "mol", "mo", "mon", "mn", "mri", "mi", "mao", "mi", "msa", "ms",
	  "may", "ms", "mya", "my", "bur", "my", "nau", "na", "nav", "nv", "nbl", "nr", "nde", "nd",
	  "ndo", "ng", "nep", "ne", "nld", "nl", "dut", "nl", "nno", "nn", "nob", "nb", "nor", "no",
	  "nya", "ny", "oci", "oc", "oji", "oj", "ori", "or", "orm", "om", "oss", "os", "pan", "pa",
	  "per", "fa", "fas", "fa", "pli", "pi", "pol", "pl", "por", "pt", "pus", "ps", "que", "qu",
	  "roh", "rm", "ron", "ro", "rum", "ro", "rum", "ro", "ron", "ro", "run", "rn", "rus", "ru",
	  "sag", "sg", "san", "sa", "scc", "sr", "srp", "sr", "scr", "hr", "hrv", "hr", "sin", "si",
	  "slk", "sk", "slo", "sk", "slo", "sk", "slk", "sk", "slv", "sl", "sme", "se", "smo", "sm",
	  "sna", "sn", "snd", "sd", "som", "so", "sot", "st", "spa", "es", "sqi", "sq", "alb", "sq",
	  "srd", "sc", "srp", "sr", "scc", "sr", "ssw", "ss", "sun", "su", "swa", "sw", "swe", "sv",
	  "tah", "ty", "tam", "ta", "tat", "tt", "tel", "te", "tgk", "tg", "tgl", "tl", "tha", "th",
	  "tib", "bo", "bod", "bo", "tir", "ti", "ton", "to", "tsn", "tn", "tso", "ts", "tuk", "tk",
	  "tur", "tr", "twi", "tw", "uig", "ug", "ukr", "uk", "urd", "ur", "uzb", "uz", "ven", "ve",
	  "vie", "vi", "vol", "vo", "wel", "cy", "cym", "cy", "wln", "wa", "wol", "wo", "xho", "xh",
	  "yid", "yi", "yor", "yo", "zha", "za", "zho", "zh", "chi", "zh", "zul", "zu", 
	  0, 0 };

static XMP_StringPtr Lookup2LetterLang ( XMP_StringPtr lang3 )
{

	for ( size_t i = 0; kKnownLangs[i] != 0; i += 2 ) {
		if ( XMP_LitMatch ( lang3, kKnownLangs[i] ) ) return kKnownLangs[i+1];
	}
	
	return lang3;	// Not found, return the input.
	
}	// Lookup2LetterLang

// =================================================================================================
// MPEG4_CheckFormat
// =================
//
// An MPEG-4 file is an instance of an ISO Base Media file, ISO 14496-12 and -14. An MPEG-4 file
// must begin with an 'ftyp' box containing 'mp41', 'mp42', or 'f4v ' in the compatible brands.
//
// The 'ftyp' box structure is:
//
//   0  4  uns32  box size, 0 means "to EoF"
//   4  4  uns32  box type, 'ftyp'
//   8  8  uns64  box size, if uns32 size is 1
//   -  4  uns32  major brand
//   -  4  uns32  minor version
//   -  *  uns32  sequence of compatible brands, to the end of the box
//
// All integers are big endian.

bool MPEG4_CheckFormat ( XMP_FileFormat format,
						 XMP_StringPtr  filePath,
						 LFA_FileRef    fileRef,
						 XMPFiles*      parent )
{
	XMP_Uns8  buffer [4096];
	XMP_Uns32 ioCount, brandCount, brandOffset, id;
	XMP_Uns64 fileSize, boxSize;
	
	// Do some basic header checks, figure out how many compatible brands are listed.
	
	fileSize = LFA_Measure ( fileRef );
	
	LFA_Seek ( fileRef, 0, SEEK_SET );
	ioCount = LFA_Read ( fileRef, buffer, 16 );	// Read to the compatible brands, assuming a 32-bit size.
	if ( ioCount < 16 ) return false;

	id = Get4CharCode ( &buffer[4] );	// Is the first box an 'ftyp' box?
	if ( id != kBE_ftyp ) return false;
	
	boxSize = GetUns32BE ( &buffer[0] );	// Get the box length.
	brandOffset = 16;

	if ( boxSize == 0 ) {
		boxSize = fileSize;
	} else if ( boxSize == 1 ) {
		boxSize = GetUns64BE ( &buffer[8] );
		LFA_Seek ( fileRef, 24, SEEK_SET );	// Seek to the compatible brands.
		brandOffset = 24;
	}
	
	if ( (boxSize < brandOffset) || (boxSize > fileSize) ||
		 ((boxSize & 0x3) != 0)  || (boxSize > 4024) ) return false;	// Sanity limit of 1000 brands.

	// Look for the 'mp41', 'mp42', of 'f4v ' compatible brands.
	
	brandCount = (XMP_Uns32)( (boxSize - brandOffset) / 4 );
	while ( brandCount > 0 ) {
	
		XMP_Uns32 clumpSize = brandCount * 4;
		if ( clumpSize > sizeof(buffer) ) clumpSize = sizeof(buffer);
		ioCount = LFA_Read ( fileRef, buffer, clumpSize );
		if ( ioCount < clumpSize ) return false;
		
		for ( XMP_Uns32 i = 0; i < clumpSize; i += 4 ) {
			id = Get4CharCode ( &buffer[i] );
			if ( (id == kBE_mp41) || (id == kBE_mp42) || (id == kBE_f4v) ) return true;
		}
		
		brandCount -= clumpSize/4;

	}
	
	return false;

}	// MPEG4_CheckFormat

// =================================================================================================
// MPEG4_MetaHandlerCTor
// =====================

XMPFileHandler * MPEG4_MetaHandlerCTor ( XMPFiles * parent )
{

	return new MPEG4_MetaHandler ( parent );

}	// MPEG4_MetaHandlerCTor

// =================================================================================================
// MPEG4_MetaHandler::MPEG4_MetaHandler
// ====================================

MPEG4_MetaHandler::MPEG4_MetaHandler ( XMPFiles * _parent ) : xmpBoxPos(0)
{

	this->parent = _parent;	// Inherited, can't set in the prefix.
	this->handlerFlags = kMPEG4_HandlerFlags;
	this->stdCharForm  = kXMP_Char8Bit;

}	// MPEG4_MetaHandler::MPEG4_MetaHandler

// =================================================================================================
// MPEG4_MetaHandler::~MPEG4_MetaHandler
// =====================================

MPEG4_MetaHandler::~MPEG4_MetaHandler()
{

	// Nothing to do yet.

}	// MPEG4_MetaHandler::~MPEG4_MetaHandler

// =================================================================================================
// GetBoxInfo
// ==========
//
// Seek to the start of a box and extract the type and size, split the size into header and content
// portions. Leave the file positioned at the first byte of content.

// ! We're returning the content size, not the raw (full) MPEG-4 box size!

static void GetBoxInfo ( LFA_FileRef fileRef, XMP_Uns64 fileSize, XMP_Uns64 boxPos,
						 XMP_Uns32 * boxType, XMP_Uns64 * headerSize, XMP_Uns64 * contentSize )
{
	XMP_Uns8  buffer [8];
	XMP_Uns32 u32Size;

	LFA_Seek ( fileRef, boxPos, SEEK_SET );
	(void) LFA_Read ( fileRef, buffer, 8, kLFA_RequireAll );

	u32Size  = GetUns32BE ( &buffer[0] );
	*boxType = Get4CharCode ( &buffer[4] );

	if ( u32Size > 1 ) {
		*headerSize  = 8;	// Normal explicit size case.
		*contentSize = u32Size - 8;
	} else if ( u32Size == 0 ) {
		*headerSize  = 8;	// The box goes to EoF.
		*contentSize = fileSize - (boxPos + 8);
	} else {
		XMP_Uns64 u64Size;	// Have a 64-bit size.
		(void) LFA_Read ( fileRef, &u64Size, 8, kLFA_RequireAll );
		u64Size = MakeUns64BE ( u64Size );
		*headerSize  = 16;
		*contentSize = u64Size - 16;
	}

}	// GetBoxInfo

// =================================================================================================
// MPEG4_MetaHandler::CacheFileData
// ================================
//
// The XMP in MPEG-4 is in a top level XMP 'uuid' box. Legacy metadata might be found in 2 places,
// the 'moov'/'mvhd' box, and the 'moov'/'udta'/'cprt' boxes. There is at most 1 each of the 'moov',
// 'mvhd' and 'udta' boxes. There are any number of 'cprt' boxes, including none. The 'udta' box can
// be large, so don't cache all of it.

void MPEG4_MetaHandler::CacheFileData()
{
	XMP_Assert ( (! this->containsXMP) && (! this->containsTNail) );
	
	LFA_FileRef fileRef = this->parent->fileRef;

	XMP_AbortProc abortProc  = this->parent->abortProc;
	void *        abortArg   = this->parent->abortArg;
	const bool    checkAbort = (abortProc != 0);
	
	XMP_Uns64 fileSize = LFA_Measure ( fileRef );
	XMP_Uns64 outerPos, outerSize, hSize, cSize;
	XMP_Uns32 boxType;

	// The outer loop looks for the top level 'moov' and 'uuid'/XMP boxes.
	
	bool moovFound = false;
	if ( this->parent->openFlags & kXMPFiles_OpenOnlyXMP ) moovFound = true;	// Ignore legacy.
	
	for ( outerPos = 0; (outerPos < fileSize) && ((! this->containsXMP) || (! moovFound)); outerPos += outerSize ) {

		if ( checkAbort && abortProc(abortArg) ) {
			XMP_Throw ( "MPEG4_MetaHandler::CacheFileData - User abort", kXMPErr_UserAbort );
		}
	
		GetBoxInfo ( fileRef, fileSize, outerPos, &boxType, &hSize, &cSize );
		outerSize = hSize + cSize;
		
		if ( (! this->containsXMP) && (boxType == kBE_uuid) ) {

			XMP_Uns8 uuid [16];
			LFA_Read ( fileRef, uuid, 16, kLFA_RequireAll );
			
			if ( memcmp ( uuid, kBE_xmpUUID, 16 ) == 0 ) {

				// Found the XMP, record the offset and size, read the packet.

				this->containsXMP = true;
				this->xmpBoxPos   = outerPos;
				this->packetInfo.offset = outerPos + hSize + 16;
				this->packetInfo.length = (XMP_Int32) (cSize - 16);

				this->xmpPacket.reserve ( this->packetInfo.length );
				this->xmpPacket.assign ( this->packetInfo.length, ' ' );
				LFA_Read ( fileRef, (void*)this->xmpPacket.data(), this->packetInfo.length, kLFA_RequireAll );

			}

		} else if ( (! moovFound) && (boxType == kBE_moov) ) {

			// The middle loop loop looks for the 'moov'/'mvhd' and 'moov'/'udta' boxes.

			moovFound = true;
			
			XMP_Uns64 middleStart = outerPos + hSize;
			XMP_Uns64 middleEnd   = outerPos + outerSize;
			XMP_Uns64 middlePos, middleSize;

			bool mvhdFound = false, udtaFound = false;

			for ( middlePos = middleStart; (middlePos < middleEnd) && ((! mvhdFound) || (! udtaFound)); middlePos += middleSize ) {

				if ( checkAbort && abortProc(abortArg) ) {
					XMP_Throw ( "MPEG4_MetaHandler::CacheFileData - User abort", kXMPErr_UserAbort );
				}

				GetBoxInfo ( fileRef, fileSize, middlePos, &boxType, &hSize, &cSize );
				middleSize = hSize + cSize;

				if ( (! mvhdFound) && (boxType == kBE_mvhd) ) {

					// Save the entire 'moov'/'mvhd' box content, it isn't very big.
					mvhdFound = true;
					this->mvhdBox.reserve ( (size_t)cSize );
					this->mvhdBox.assign ( (size_t)cSize, ' ' );
					LFA_Read ( fileRef, (void*)this->mvhdBox.data(), (XMP_Int32)cSize, kLFA_RequireAll );

				} else if ( (! udtaFound) && (boxType == kBE_udta) ) {

					// The inner loop looks for the 'moov'/'udta'/'cprt' boxes.
		
					udtaFound = true;
					XMP_Uns64 innerStart = middlePos + hSize;
					XMP_Uns64 innerEnd   = middlePos + middleSize;
					XMP_Uns64 innerPos, innerSize;

					for ( innerPos = innerStart; innerPos < innerEnd; innerPos += innerSize ) {
		
						if ( checkAbort && abortProc(abortArg) ) {
							XMP_Throw ( "MPEG4_MetaHandler::CacheFileData - User abort", kXMPErr_UserAbort );
						}
		
						GetBoxInfo ( fileRef, fileSize, innerPos, &boxType, &hSize, &cSize );
						innerSize = hSize + cSize;
						if ( boxType != kBE_cprt ) continue;

						// ! Actually capturing structured data - the 'cprt' box content.
						this->cprtBoxes.push_back ( std::string() );
						std::string & newCprt = this->cprtBoxes.back();
						newCprt.reserve ( (size_t)cSize );
						newCprt.assign ( (size_t)cSize, ' ' );
						LFA_Read ( fileRef, (void*)newCprt.data(), (XMP_Int32)cSize, kLFA_RequireAll );

					}	// inner loop

				}	// 'moov'/'udta' box

			}	// middle loop

		}	// 'moov' box
	
	}	// outer loop
	
}	// MPEG4_MetaHandler::CacheFileData

// =================================================================================================
// MPEG4_MetaHandler::MakeLegacyDigest
// ===================================

// *** Will need updating if we process the 'ilst' metadata.

#define kHexDigits "0123456789ABCDEF"

void MPEG4_MetaHandler::MakeLegacyDigest ( std::string * digestStr )
{
	MD5_CTX    context;
	unsigned char digestBin [16];
	MD5Init ( &context );
	
	MD5Update ( &context, (XMP_Uns8*)this->mvhdBox.c_str(), (unsigned int) this->mvhdBox.size() );
	
	for ( size_t i = 0, limit = this->cprtBoxes.size(); i < limit; ++i ) {
		const std::string & currCprt = this->cprtBoxes[i];
		MD5Update ( &context, (XMP_Uns8*)currCprt.c_str(), (unsigned int) currCprt.size() );
	}

	MD5Final ( digestBin, &context );

	char buffer [40];
	for ( int in = 0, out = 0; in < 16; in += 1, out += 2 ) {
		XMP_Uns8 byte = digestBin[in];
		buffer[out]   = kHexDigits [ byte >> 4 ];
		buffer[out+1] = kHexDigits [ byte & 0xF ];
	}
	buffer[32] = 0;
	digestStr->erase();
	digestStr->append ( buffer, 32 );

}	// MPEG4_MetaHandler::MakeLegacyDigest

// =================================================================================================

struct MVHD_v1 {					//  v1   v0	- offsets within the content portion of the 'mvhd' box
	XMP_Uns8 version;				//   0    0
	XMP_Uns8 flags [3];				//   1    1
	XMP_Uns64 creationTime;			//   4    4 - Uns32 in v0
	XMP_Uns64 modificationTime;		//  12    8 - Uns32 in v0
	XMP_Uns32 timescale;			//  20   12
	XMP_Uns64 duration;				//  24   16 - Uns32 in v0
	XMP_Int32 rate;					//  32   20
	XMP_Int16 volume;				//  36   24
	XMP_Uns16 pad_1;				//  38   26
	XMP_Uns32 pad_2, pad_3;			//  40   28
	XMP_Int32 matrix [9];			//  48   36
	XMP_Uns32 preDef [6];			//  84   72
	XMP_Uns32 nextTrackID;			// 108   96
};									// 112  100

static void ExtractMVHD_v0 ( XMP_Uns8 * buffer, MVHD_v1 * mvhd )	// Always convert to the v1 form.
{
	mvhd->version          = buffer[0];
	mvhd->flags[0]         = buffer[1];
	mvhd->flags[1]         = buffer[2];
	mvhd->flags[2]         = buffer[3];
	mvhd->creationTime     = GetUns32BE ( &buffer[ 4] );
	mvhd->modificationTime = GetUns32BE ( &buffer[ 8] );
	mvhd->timescale        = GetUns32BE ( &buffer[12] );
	mvhd->duration         = GetUns32BE ( &buffer[16] );
	mvhd->rate             = GetUns32BE ( &buffer[20] );
	mvhd->volume           = GetUns16BE ( &buffer[24] );
	mvhd->pad_1            = GetUns16BE ( &buffer[26] );
	mvhd->pad_2            = GetUns32BE ( &buffer[28] );
	mvhd->pad_3            = GetUns32BE ( &buffer[32] );
	for ( int i = 0, j = 36; i < 9; ++i, j += 4 ) mvhd->matrix[i] = GetUns32BE ( &buffer[j] );
	for ( int i = 0, j = 72; i < 6; ++i, j += 4 ) mvhd->preDef[i] = GetUns32BE ( &buffer[j] );
	mvhd->nextTrackID      = GetUns32BE ( &buffer[96] );
}

static void ExtractMVHD_v1 ( XMP_Uns8 * buffer, MVHD_v1 * mvhd )
{
	mvhd->version          = buffer[0];
	mvhd->flags[0]         = buffer[1];
	mvhd->flags[1]         = buffer[2];
	mvhd->flags[2]         = buffer[3];
	mvhd->creationTime     = GetUns64BE ( &buffer[ 4] );
	mvhd->modificationTime = GetUns64BE ( &buffer[12] );
	mvhd->timescale        = GetUns32BE ( &buffer[20] );
	mvhd->duration         = GetUns64BE ( &buffer[24] );
	mvhd->rate             = GetUns32BE ( &buffer[32] );
	mvhd->volume           = GetUns16BE ( &buffer[36] );
	mvhd->pad_1            = GetUns16BE ( &buffer[38] );
	mvhd->pad_2            = GetUns32BE ( &buffer[40] );
	mvhd->pad_3            = GetUns32BE ( &buffer[44] );
	for ( int i = 0, j = 48; i < 9; ++i, j += 4 ) mvhd->matrix[i] = GetUns32BE ( &buffer[j] );
	for ( int i = 0, j = 84; i < 6; ++i, j += 4 ) mvhd->preDef[i] = GetUns32BE ( &buffer[j] );
	mvhd->nextTrackID      = GetUns32BE ( &buffer[108] );
}

// =================================================================================================
// MPEG4_MetaHandler::ProcessXMP
// =============================

#define kAlmostMaxSeconds 0x7FFFFF00

void MPEG4_MetaHandler::ProcessXMP()
{
	if ( this->processedXMP ) return;
	this->processedXMP = true;	// Make sure only called once.
	
	if ( this->containsXMP ) {
		FillPacketInfo ( this->xmpPacket, &this->packetInfo );
		this->xmpObj.ParseFromBuffer ( this->xmpPacket.c_str(), (XMP_StringLen)this->xmpPacket.size() );
	}
	
	if ( this->mvhdBox.empty() && this->cprtBoxes.empty() ) return;	// No legacy, we're done.
	
	std::string oldDigest;
	bool oldDigestFound = this->xmpObj.GetStructField ( kXMP_NS_XMP, "NativeDigests", kXMP_NS_XMP, "MPEG-4", &oldDigest, 0 );
	
	if ( oldDigestFound ) {
		std::string newDigest;
		this->MakeLegacyDigest ( &newDigest );
		if ( oldDigest == newDigest ) return;	// No legacy changes.
	}
	
	// If we get here we need to import the legacy metadata. Either there is no old digest in the
	// XMP, or the digests differ. In the former case keep any existing XMP, in the latter case take
	// new legacy values. So, oldDigestFound means digestsDiffer, else we would have returned.
	
	// *** The "official" MPEG-4 metadata might not be very interesting. It looks like the 'ilst'
	// *** metadata (invented by Apple?) is more interesting. There appears to be no official
	// *** documentation.

	if ( ! this->mvhdBox.empty() ) {

		MVHD_v1 mvhd;
		XMP_DateTime xmpDate;

		if ( this->mvhdBox.size() == 100 ) {	// *** Should check the version - extract to a static function.
			ExtractMVHD_v0 ( (XMP_Uns8*)(this->mvhdBox.data()), &mvhd );
		} else if ( this->mvhdBox.size() == 112 ) {
			ExtractMVHD_v1 ( (XMP_Uns8*)(this->mvhdBox.data()), &mvhd );
		}

		if ( oldDigestFound || (! this->xmpObj.DoesPropertyExist ( kXMP_NS_XMP, "CreateDate" )) ) {
			if ( (mvhd.creationTime >> 32) < 0xFF ) {		// Sanity check for bogus date info.

				memset ( &xmpDate, 0, sizeof(xmpDate) );	// AUDIT: Using sizeof(xmpDate) is safe.
				xmpDate.year  = 1904;	// Start at midnight, January 1 1904, UTC
				xmpDate.month = 1;		// ! Note that the XMP binary fields are signed to allow
				xmpDate.day   = 1;		// ! offsets and normalization in both directions. 

				while ( mvhd.creationTime > kAlmostMaxSeconds ) {
					xmpDate.second += kAlmostMaxSeconds;
					SXMPUtils::ConvertToUTCTime ( &xmpDate );	// ! For the normalization side effect.
					mvhd.creationTime -= kAlmostMaxSeconds;
				}
				xmpDate.second += (XMP_Uns32)mvhd.creationTime;
				SXMPUtils::ConvertToUTCTime ( &xmpDate );	// ! For the normalization side effect.
				
				this->xmpObj.SetProperty_Date ( kXMP_NS_XMP, "CreateDate", xmpDate );
				this->containsXMP = true;

			}
		}

		if ( oldDigestFound || (! this->xmpObj.DoesPropertyExist ( kXMP_NS_XMP, "ModifyDate" )) ) {
			if ( (mvhd.modificationTime >> 32) < 0xFF ) {	// Sanity check for bogus date info.

				memset ( &xmpDate, 0, sizeof(xmpDate) );	// AUDIT: Using sizeof(xmpDate) is safe.
				xmpDate.year  = 1904;	// Start at midnight, January 1 1904, UTC
				xmpDate.month = 1;
				xmpDate.day   = 1;

				while ( mvhd.modificationTime > kAlmostMaxSeconds ) {
					xmpDate.second += kAlmostMaxSeconds;
					SXMPUtils::ConvertToUTCTime ( &xmpDate );	// ! For the normalization side effect.
					mvhd.modificationTime -= kAlmostMaxSeconds;
				}
				xmpDate.second += (XMP_Uns32)mvhd.modificationTime;
				SXMPUtils::ConvertToUTCTime ( &xmpDate );	// ! For the normalization side effect.
				
				this->xmpObj.SetProperty_Date ( kXMP_NS_XMP, "ModifyDate", xmpDate );
				this->containsXMP = true;

			}
		}

		if ( oldDigestFound || (! this->xmpObj.DoesPropertyExist ( kXMP_NS_DM, "duration" )) ) {
			if ( mvhd.timescale != 0 ) {	// Avoid 1/0 for the scale field.
				char buffer [32];	// A 64-bit number is at most 20 digits.
				this->xmpObj.DeleteProperty ( kXMP_NS_DM, "duration" );	// Delete the whole struct.
				snprintf ( buffer, sizeof(buffer), "%llu", mvhd.duration );	// AUDIT: The buffer is big enough.
				this->xmpObj.SetStructField ( kXMP_NS_DM, "duration", kXMP_NS_DM, "value", &buffer[0] );
				snprintf ( buffer, sizeof(buffer), "1/%u", mvhd.timescale );	// AUDIT: The buffer is big enough.
				this->xmpObj.SetStructField ( kXMP_NS_DM, "duration", kXMP_NS_DM, "scale", &buffer[0] );
				this->containsXMP = true;
			}
		}

	}

	if ( oldDigestFound || (! this->xmpObj.DoesPropertyExist ( kXMP_NS_DC, "rights" )) ) {

		std::string tempStr;
		char lang3 [4];	// The unpacked ISO-639-2/T language code.
		lang3[3] = 0;

		for ( size_t i = 0, limit = this->cprtBoxes.size(); i < limit; ++i ) {
			if ( this->cprtBoxes[i].size() < 7 ) continue;

			const XMP_Uns8 * currCprt = (XMP_Uns8*) (this->cprtBoxes[i].c_str());	// ! Actually structured data!
			size_t rawLen = this->cprtBoxes[i].size() - 6;
			if ( currCprt[0] != 0 ) continue;	// Only proceed for version 0, ignore the flags.

			XMP_Uns16 packedLang = GetUns16BE ( &currCprt[4] );
			lang3[0] = (packedLang >> 10) | 0x60;
			lang3[1] = ((packedLang >> 5) & 0x1F) | 0x60;
			lang3[2] = (packedLang & 0x1F) | 0x60;
			
			XMP_StringPtr xmpLang  = Lookup2LetterLang ( lang3 );
			XMP_StringPtr xmpValue = (XMP_StringPtr) &currCprt[6];
			
			if ( (rawLen >= 8) && (GetUns16BE ( xmpValue ) == 0xFEFF) ) {
				FromUTF16 ( (UTF16Unit*)xmpValue, rawLen/2, &tempStr, true /* big endian */ );
				xmpValue = tempStr.c_str();
			}
			
			this->xmpObj.SetLocalizedText ( kXMP_NS_DC, "rights", xmpLang, "", xmpValue );
			this->containsXMP = true;

		}

	}

}	// MPEG4_MetaHandler::ProcessXMP

// =================================================================================================
// MPEG4_MetaHandler::PickNewLocation
// ==================================
//
// Pick a new location for the XMP. This is the first available 'free' space before any 'mdat' box,
// otherwise the end of the file. Any existing XMP 'uuid' box has already been marked as 'free'.

void MPEG4_MetaHandler::PickNewLocation()
{
	LFA_FileRef fileRef  = this->parent->fileRef;
	XMP_Uns64   fileSize = LFA_Measure ( fileRef );

	XMP_Uns32 xmpBoxSize = 4+4+16 + (XMP_Uns32)this->xmpPacket.size();

	XMP_Uns64 currPos, prevPos;		// Info about the most recent 2 boxes.
	XMP_Uns32 currType, prevType;
	XMP_Uns64 currSize, prevSize, hSize, cSize;

	XMP_Uns32 be32Size;
	XMP_Uns64 be64Size;
	
	XMP_AbortProc abortProc  = this->parent->abortProc;
	void *        abortArg   = this->parent->abortArg;
	const bool    checkAbort = (abortProc != 0);
	
	bool pastMDAT = false;
	
	currType = 0;
	prevPos = prevSize = currSize = 0;
	for ( currPos = 0; currPos < fileSize; currPos += currSize ) {

		if ( checkAbort && abortProc(abortArg) ) {
			XMP_Throw ( "MPEG4_MetaHandler::UpdateFile - User abort", kXMPErr_UserAbort );
		}
	
		prevPos += prevSize;	// ! We'll go through at least once, the first box is 'ftyp'.
		prevType = currType;	// ! Care is needed to preserve the prevBox and currBox info when
		prevSize = currSize;	// ! the loop exits because it hits EoF.
		XMP_Assert ( (prevPos + prevSize) == currPos );
		
		GetBoxInfo ( fileRef, fileSize, currPos, &currType, &hSize, &cSize );
		currSize = hSize + cSize;
		
		if ( pastMDAT ) continue;	// Keep scanning to the end of the file.
		if ( currType == kBE_mdat ) pastMDAT = true;
		if ( currType != kBE_free ) continue;
		if ( currSize >= xmpBoxSize ) break;
		
		if ( prevType == kBE_free ) {
			// If we get here the prevBox and currBox are both 'free' and neither big enough alone.
			// Pretend to combine them for this check and for possible following checks. We don't
			// really compbine them, we just remember the start and total length.
			currPos = prevPos;
			currSize += prevSize;
			prevSize = 0;	// ! For the start of the next loop pass.
			if ( currSize >= xmpBoxSize ) break;
		}
		
	}
	
	if ( currPos < fileSize ) {

		// Put the XMP at the start of the 'free' space. Increase the size of the XMP if the excess
		// is less than 8 bytes, otherwise write a new 'free' box header.
		
		this->xmpBoxPos = currPos;
		XMP_Assert ( (currType == kBE_free) && (currSize >= xmpBoxSize) );

		XMP_Uns64 excessSpace = currSize - xmpBoxSize;
		if ( excessSpace < 8 ) {
			this->xmpPacket.append ( (size_t)excessSpace, ' ' );
		} else {
			LFA_Seek ( fileRef, (currPos + xmpBoxSize), SEEK_SET );
			if ( excessSpace <= 0xFFFFFFFFULL ) {
				be32Size = MakeUns32BE ( (XMP_Uns32)excessSpace );
				LFA_Write ( fileRef, &be32Size, 4 );
				LFA_Write ( fileRef, &kBE_free, 4 );
			} else {
				be32Size = MakeUns32BE ( 1 );
				be64Size = MakeUns64BE ( excessSpace );
				LFA_Write ( fileRef, &be32Size, 4 );
				LFA_Write ( fileRef, &kBE_free, 4 );
				LFA_Write ( fileRef, &be64Size, 8 );
			}
		}

	} else {

		// Appending the XMP, make sure the current final box has an explicit size.
		
		this->xmpBoxPos = fileSize;
		XMP_Assert ( currPos == fileSize );

		currPos -= currSize;	// Move back to the final box's origin.
		LFA_Seek ( fileRef, currPos, SEEK_SET );
		LFA_Read ( fileRef, &be32Size, 4, kLFA_RequireAll );
		be32Size = MakeUns32BE ( be32Size );

		if ( be32Size == 0 ) {
		
			// The current final box is "to-EoF", we need to write the actual size. If the size fits
			// in 32 bits then we just set it in the leading Uns32 field instead of the "to-EoF"
			// value. Otherwise we have to insert a 64-bit size. If the previous box is 'free' then
			// we take 8 bytes from the end of it, shift the size/type parts of the final box up 8
			// bytes, making space for the 64-bit size. Give up if the previous box is not 'free'.

			if ( currSize <= 0xFFFFFFFFULL ) {

				// The size fits in 32 bits, reuse the leading Uns32 size.
				be32Size = MakeUns32BE ( (XMP_Uns32)currSize );
				LFA_Seek ( fileRef, currPos, SEEK_SET );
				LFA_Write ( fileRef, &be32Size, 4 );

			} else if ( prevType != kBE_free ) {

				// We need to insert a 64-bit size, but the previous box is not 'free'.
				XMP_Throw ( "MPEG4_MetaHandler::PickNewLocation - Can't set box size", kXMPErr_ExternalFailure );

			} else if ( prevSize == 8 ) {
			
				// Absorb the whole free box.
				LFA_Seek ( fileRef, prevPos, SEEK_SET );
				be32Size = MakeUns32BE ( 1 );
				LFA_Write ( fileRef, &be32Size, 4 );
				LFA_Write ( fileRef, &currType, 4 );
				be64Size = MakeUns64BE ( currSize );
				LFA_Write ( fileRef, &be64Size, 8 );

			} else {

				// Trim 8 bytes off the end of the free box.
				
				prevSize -= 8;

				LFA_Seek ( fileRef, prevPos, SEEK_SET );
				LFA_Read ( fileRef, &be32Size, 4, kLFA_RequireAll );
				if ( be32Size != MakeUns32BE ( 1 ) ) {
					be32Size = MakeUns32BE ( (XMP_Uns32)prevSize );
					LFA_Seek ( fileRef, prevPos, SEEK_SET );
					LFA_Write ( fileRef, &be32Size, 4 );
				} else {
					be64Size = MakeUns64BE ( prevSize );
					LFA_Seek ( fileRef, (prevPos + 8), SEEK_SET );
					LFA_Write ( fileRef, &be64Size, 8 );
				}

				LFA_Seek ( fileRef, (currPos - 8), SEEK_SET );
				be32Size = MakeUns32BE ( 1 );
				LFA_Write ( fileRef, &be32Size, 4 );
				LFA_Write ( fileRef, &currType, 4 );
				be64Size = MakeUns64BE ( currSize );
				LFA_Write ( fileRef, &be64Size, 8 );

			}

		}

	}
	
}	// MPEG4_MetaHandler::PickNewLocation

// =================================================================================================
// MPEG4_MetaHandler::UpdateFile
// =============================

// *** There are no writebacks to legacy metadata yet. We need to resolve the questions about
// *** standard MPEG-4 metadata versus 'ilst' metadata.

// *** The current logic for the XMP is simple. Use the existing XMP if it is big enough, next look
// *** for 'free' space before any 'mdat' boxes, finally append to the end.

// ! MPEG-4 can be indexed with absolute offsets, only the 'moov' and XMP 'uuid' boxes can be moved!

void MPEG4_MetaHandler::UpdateFile ( bool doSafeUpdate )
{
	if ( ! this->needsUpdate ) return;
	this->needsUpdate = false;	// Make sure only called once.
	XMP_Assert ( ! doSafeUpdate );	// This should only be called for "unsafe" updates.

	XMP_AbortProc abortProc  = this->parent->abortProc;
	void *        abortArg   = this->parent->abortArg;
	const bool    checkAbort = (abortProc != 0);
	
	LFA_FileRef fileRef  = this->parent->fileRef;
	XMP_Uns64   fileSize = LFA_Measure ( fileRef );

	XMP_Uns32 be32Size;
	
	// Make sure the XMP has a current legacy digest.
	std::string newDigest;
	this->MakeLegacyDigest ( &newDigest );
	this->xmpObj.SetStructField ( kXMP_NS_XMP, "NativeDigests", kXMP_NS_XMP, "MPEG-4", newDigest.c_str(), kXMP_DeleteExisting );
	XMP_StringLen xmpLen = (XMP_StringLen)this->xmpPacket.size();
	try {
		this->xmpObj.SerializeToBuffer ( &this->xmpPacket, (kXMP_UseCompactFormat | kXMP_ExactPacketLength), xmpLen );
	} catch ( ... ) {
		this->xmpObj.SerializeToBuffer ( &this->xmpPacket, kXMP_UseCompactFormat );
	}

	#if XMP_DebugBuild	// Sanity check that the XMP is where we think it is.

		if ( this->xmpBoxPos != 0 ) {
			
			XMP_Uns32 boxType;
			XMP_Uns64 hSize, cSize;
			XMP_Uns8 uuid [16];
	
			GetBoxInfo ( fileRef, LFA_Measure ( fileRef ), this->xmpBoxPos, &boxType, &hSize, &cSize );
			LFA_Read ( fileRef, uuid, 16, kLFA_RequireAll );
	
			if ( ((this->xmpBoxPos + hSize + 16) != (XMP_Uns64)this->packetInfo.offset) || 
				 ((cSize - 16) != (XMP_Uns64)this->packetInfo.length) ||
				 (boxType != kBE_uuid) || (memcmp ( uuid, kBE_xmpUUID, 16 ) != 0) ) {
				XMP_Throw ( "Inaccurate MPEG-4 packet info", kXMPErr_InternalFailure );
			}
			
		}

	#endif
	
	if ( (this->xmpBoxPos != 0) && (this->xmpPacket.size() <= (size_t)this->packetInfo.length) ) {

		// Update existing XMP in-place.
		
		if ( this->xmpPacket.size() < (size_t)this->packetInfo.length ) {
			// They ought to match, cheap to be sure.
			size_t extraSpace = (size_t)this->packetInfo.length - this->xmpPacket.size();
			this->xmpPacket.append ( extraSpace, ' ' );
		}
		
		XMP_Assert ( this->xmpPacket.size() == (size_t)this->packetInfo.length );
		LFA_Seek ( fileRef, this->packetInfo.offset, SEEK_SET );
		LFA_Write ( fileRef, this->xmpPacket.data(), (XMP_Int32)this->xmpPacket.size() );

	} else if ( (this->xmpBoxPos != 0) &&
				((XMP_Uns64)(this->packetInfo.offset + this->packetInfo.length) == fileSize) ) {
	
		// The XMP is already at the end of the file, rewrite the whole 'uuid' box.

		XMP_Assert ( this->xmpPacket.size() > (size_t)this->packetInfo.length );

		LFA_Seek ( fileRef, this->xmpBoxPos, SEEK_SET );
		be32Size = MakeUns32BE ( 4+4+16 + (XMP_Uns32)this->xmpPacket.size() );	// ! XMP must be 4GB or less!
		LFA_Write ( fileRef, &be32Size, 4 );
		LFA_Write ( fileRef, &kBE_uuid, 4 );
		LFA_Write ( fileRef, &kBE_xmpUUID, 16 );
		LFA_Write ( fileRef, this->xmpPacket.data(), (XMP_Int32)this->xmpPacket.size() );
	
	} else {
	
		// We are injecting first time XMP, or making the existing XMP larger. Pick a new location
		// for the XMP. This is the first available space before any 'mdat' box, otherwise the end
		// of the file. Available space can be any contiguous combination of 'free' space with the
		// existing XMP. Mark any existing XMP as 'free' first, this simplifies the logic and we
		// can't do it later since we might reuse the space.
	
		if ( this->xmpBoxPos != 0 ) {
			LFA_Seek ( fileRef, (this->xmpBoxPos + 4), SEEK_SET );
			LFA_Write ( fileRef, "free", 4 );
		}
		
		this->PickNewLocation();	// ! Might increase the size of the XMP packet.

		LFA_Seek ( fileRef, this->xmpBoxPos, SEEK_SET );
		be32Size = MakeUns32BE ( 4+4+16 + (XMP_Uns32)this->xmpPacket.size() );	// ! XMP must be 4GB or less!
		LFA_Write ( fileRef, &be32Size, 4 );
		LFA_Write ( fileRef, &kBE_uuid, 4 );
		LFA_Write ( fileRef, &kBE_xmpUUID, 16 );
		LFA_Write ( fileRef, this->xmpPacket.data(), (XMP_Int32)this->xmpPacket.size() );
	
	}
	
}	// MPEG4_MetaHandler::UpdateFile

// =================================================================================================
// MPEG4_MetaHandler::WriteFile
// ============================
//
// Since the XMP and legacy is probably a miniscule part of the entire file, and since we can't
// change the offset of most of the boxes, just copy the entire source file to the dest file, then
// do an in-place update to the destination file.

void MPEG4_MetaHandler::WriteFile ( LFA_FileRef sourceRef, const std::string & sourcePath )
{
	XMP_Assert ( this->needsUpdate );
	
	LFA_FileRef destRef = this->parent->fileRef;
	
	LFA_Seek ( sourceRef, 0, SEEK_SET );
	LFA_Seek ( destRef, 0, SEEK_SET );
	LFA_Copy ( sourceRef, destRef, LFA_Measure ( sourceRef ),
			   this->parent->abortProc, this->parent->abortArg );
	
	this->UpdateFile ( false );

}	// MPEG4_MetaHandler::WriteFile

// =================================================================================================
