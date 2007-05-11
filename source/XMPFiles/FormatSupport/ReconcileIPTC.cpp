// =================================================================================================
// ADOBE SYSTEMS INCORPORATED
// Copyright 2006-2007 Adobe Systems Incorporated
// All Rights Reserved
//
// NOTICE: Adobe permits you to use, modify, and distribute this file in accordance with the terms
// of the Adobe license agreement accompanying it.
// =================================================================================================

#include "XMP_Environment.h"	// ! This must be the first include.

#include "Reconcile_Impl.hpp"

#if XMP_WinBuild
	#pragma warning ( disable : 4800 )	// forcing value to bool 'true' or 'false' (performance warning)
	#pragma warning ( disable : 4996 )	// '...' was declared deprecated
#endif

// =================================================================================================
/// \file ReconcileIPTC.cpp
/// \brief Utilities to reconcile between XMP and legacy IPTC and PSIR metadata.
///
// =================================================================================================

// =================================================================================================
// NormalizeToCR
// =============

static inline void NormalizeToCR ( std::string * value )
{
	char * strPtr = (char*) value->data();
	char * strEnd = strPtr + value->size();
	
	for ( ; strPtr < strEnd; ++strPtr ) {
		if ( *strPtr == kLF ) *strPtr = kCR;
	}
	
}	// NormalizeToCR

// =================================================================================================
// NormalizeToLF
// =============

static inline void NormalizeToLF ( std::string * value )
{
	char * strPtr = (char*) value->data();
	char * strEnd = strPtr + value->size();
	
	for ( ; strPtr < strEnd; ++strPtr ) {
		if ( *strPtr == kCR ) *strPtr = kLF;
	}
	
}	// NormalizeToLF

// =================================================================================================
// ComputeIPTCDigest
// =================
//
// Compute a 128 bit (16 byte) MD5 digest of the full IPTC block.

static inline void ComputeIPTCDigest ( IPTC_Manager * iptc, MD5_Digest * digest )
{
	MD5_CTX    context;
	void *     iptcData;
	XMP_Uns32  iptcLen;

	iptcLen = iptc->UpdateMemoryDataSets ( &iptcData );
	
	MD5Init ( &context );
	MD5Update ( &context, (XMP_Uns8*)iptcData, iptcLen );
	MD5Final ( *digest, &context );

}	// ComputeIPTCDigest;

// =================================================================================================
// ReconcileUtils::CheckIPTCDigest
// ===============================

int ReconcileUtils::CheckIPTCDigest ( IPTC_Manager * iptc, const PSIR_Manager & psir )
{
	MD5_Digest newDigest;
	PSIR_Manager::ImgRsrcInfo ir1061;
	
	ComputeIPTCDigest ( iptc, &newDigest );
	bool found = psir.GetImgRsrc ( kPSIR_IPTCDigest, &ir1061 );

	if ( ! found ) return kDigestMissing;
	if ( ir1061.dataLen != 16 ) return kDigestMissing;
	
	if ( memcmp ( newDigest, ir1061.dataPtr, 16 ) == 0 ) return kDigestMatches;
	return kDigestDiffers;
	
}	// ReconcileUtils::CheckIPTCDigest

// =================================================================================================
// ReconcileUtils::SetIPTCDigest
// ===============================

void ReconcileUtils::SetIPTCDigest ( IPTC_Manager * iptc, PSIR_Manager * psir )
{
	MD5_Digest newDigest;
	
	ComputeIPTCDigest ( iptc, &newDigest );
	psir->SetImgRsrc ( kPSIR_IPTCDigest, &newDigest, sizeof(newDigest) );
	
}	// ReconcileUtils::SetIPTCDigest

// =================================================================================================
// =================================================================================================

// =================================================================================================
// ImportIPTC_Simple
// =================

static void ImportIPTC_Simple ( const IPTC_Manager & iptc, SXMPMeta * xmp, int digestState,
								XMP_Uns8 id, const char * xmpNS, const char * xmpProp )
{
	if ( digestState == kDigestDiffers ) {
		xmp->DeleteProperty ( xmpNS, xmpProp );
	} else {
		XMP_Assert ( digestState == kDigestMissing );
		if ( xmp->DoesPropertyExist ( xmpNS, xmpProp ) ) return;
	}

	std::string utf8Str;
	size_t count = iptc.GetDataSet_UTF8 ( id, &utf8Str );

	if ( count != 0 ) {
		NormalizeToLF ( &utf8Str );
		xmp->SetProperty ( xmpNS, xmpProp, utf8Str.c_str() );
	}

}	// ImportIPTC_Simple

// =================================================================================================
// ImportIPTC_LangAlt
// ==================

static void ImportIPTC_LangAlt ( const IPTC_Manager & iptc, SXMPMeta * xmp, int digestState,
								 XMP_Uns8 id, const char * xmpNS, const char * xmpProp )
{
	if ( digestState == kDigestDiffers ) {
		std::string xdItemPath = xmpProp;	// Delete just the x-default item, not the whole array.
		xdItemPath += "[?xml:lang='x-default']";
		xmp->DeleteProperty ( xmpNS, xdItemPath.c_str() );
	} else {
		XMP_Assert ( digestState == kDigestMissing );
		if ( xmp->DoesPropertyExist ( xmpNS, xmpProp ) ) return;	// Check the entire array here.
	}

	std::string utf8Str;

	size_t count = iptc.GetDataSet_UTF8 ( id, &utf8Str );
	
	if ( count != 0 ) {
		NormalizeToLF ( &utf8Str );
		xmp->SetLocalizedText ( xmpNS, xmpProp, "", "x-default", utf8Str.c_str() );
	}

}	// ImportIPTC_LangAlt

// =================================================================================================
// ImportIPTC_Array
// ================

static void ImportIPTC_Array ( const IPTC_Manager & iptc, SXMPMeta * xmp, int digestState,
							   XMP_Uns8 id, const char * xmpNS, const char * xmpProp )
{
	if ( digestState == kDigestDiffers ) {
		xmp->DeleteProperty ( xmpNS, xmpProp );
	} else {
		XMP_Assert ( digestState == kDigestMissing );
		if ( xmp->DoesPropertyExist ( xmpNS, xmpProp ) ) return;
	}

	std::string utf8Str;
	size_t count = iptc.GetDataSet ( id, 0 );

	for ( size_t ds = 0; ds < count; ++ds ) {
		(void) iptc.GetDataSet_UTF8 ( id, &utf8Str, ds );
		NormalizeToLF ( &utf8Str );
		xmp->AppendArrayItem ( xmpNS, xmpProp, kXMP_PropArrayIsUnordered, utf8Str.c_str() );
	}

}	// ImportIPTC_Array

// =================================================================================================
// ImportIPTC_IntellectualGenre
// ============================
//
// Import DataSet 2:04. In the IIM this is a 3 digit number, a colon, and an optional text name.
// Even though the number is the more formal part, the IPTC4XMP rule is that the name is imported to
// XMP and the number is dropped. Also, even though IIMv4.1 says that 2:04 is repeatable, the XMP
// property to which it is mapped is simple.

static void ImportIPTC_IntellectualGenre ( const IPTC_Manager & iptc, SXMPMeta * xmp, int digestState,
										   const char * xmpNS, const char * xmpProp )
{
	if ( digestState == kDigestDiffers ) {
		xmp->DeleteProperty ( xmpNS, xmpProp );
	} else {
		XMP_Assert ( digestState == kDigestMissing );
		if ( xmp->DoesPropertyExist ( xmpNS, xmpProp ) ) return;
	}

	std::string utf8Str;
	size_t count = iptc.GetDataSet_UTF8 ( kIPTC_IntellectualGenre, &utf8Str );

	if ( count == 0 ) return;
	NormalizeToLF ( &utf8Str );
	
	XMP_StringPtr namePtr = utf8Str.c_str() + 4;
	
	if ( utf8Str.size() <= 4 ) {
		// No name in the IIM. Look up the number in our list of known genres.
		int i;
		XMP_StringPtr numPtr = utf8Str.c_str();
		for ( i = 0; kIntellectualGenreMappings[i].refNum != 0; ++i ) {
			if ( strncmp ( numPtr, kIntellectualGenreMappings[i].refNum, 3 ) == 0 ) break;
		}
		if ( kIntellectualGenreMappings[i].refNum == 0 ) return;
		namePtr = kIntellectualGenreMappings[i].name;
	}

	xmp->SetProperty ( xmpNS, xmpProp, namePtr );

}	// ImportIPTC_IntellectualGenre

// =================================================================================================
// ImportIPTC_SubjectCode
// ======================
//
// Import all 2:12 DataSets into an unordered array. In the IIM each DataSet is composed of 5 colon
// separated sections: a provider name, an 8 digit reference number, and 3 optional names for the
// levels of the reference number hierarchy. The IPTC4XMP mapping rule is that only the reference
// number is imported to XMP.

static void ImportIPTC_SubjectCode ( const IPTC_Manager & iptc, SXMPMeta * xmp, int digestState,
									 const char * xmpNS, const char * xmpProp )
{
	if ( digestState == kDigestDiffers ) {
		xmp->DeleteProperty ( xmpNS, xmpProp );
	} else {
		XMP_Assert ( digestState == kDigestMissing );
		if ( xmp->DoesPropertyExist ( xmpNS, xmpProp ) ) return;
	}

	std::string utf8Str;
	size_t count = iptc.GetDataSet_UTF8 ( kIPTC_SubjectCode, 0 );
	
	for ( size_t ds = 0; ds < count; ++ds ) {

		(void) iptc.GetDataSet_UTF8 ( kIPTC_SubjectCode, &utf8Str, ds );

		char * refNumPtr = (char*) utf8Str.c_str();
		for ( ; (*refNumPtr != ':') && (*refNumPtr != 0); ++refNumPtr ) {}
		if ( *refNumPtr == 0 ) continue;	// This DataSet is ill-formed.

		char * refNumEnd = refNumPtr + 1;
		for ( ; (*refNumEnd != ':') && (*refNumEnd != 0); ++refNumEnd ) {}
		if ( (refNumEnd - refNumPtr) != 8 ) continue;	// This DataSet is ill-formed.
		*refNumEnd = 0;	// Ensure a terminating nul for the reference number portion.

		xmp->AppendArrayItem ( xmpNS, xmpProp, kXMP_PropArrayIsUnordered, refNumPtr );

	}

}	// ImportIPTC_SubjectCode

// =================================================================================================
// ImportIPTC_DateCreated
// ======================
//
// An IPTC (IIM) date is 8 charcters YYYYMMDD. Include the time portion from 2:60 if it is present.
// The IPTC time is HHMMSSxHHMM, where 'x' is '+' or '-'. Be tolerant of some ill-formed dates and
// times. Apparently some non-Adobe apps put strings like "YYYY-MM-DD" or "HH:MM:SSxHH:MM" in the
// IPTC. Allow a missing time zone portion to mean UTC.

static void ImportIPTC_DateCreated ( const IPTC_Manager & iptc, SXMPMeta * xmp, int digestState,
									 const char * xmpNS, const char * xmpProp )
{
	if ( digestState == kDigestDiffers ) {
		xmp->DeleteProperty ( xmpNS, xmpProp );
	} else {
		XMP_Assert ( digestState == kDigestMissing );
		if ( xmp->DoesPropertyExist ( xmpNS, xmpProp ) ) return;
	}

	// First gather the date portion.
	
	IPTC_Manager::DataSetInfo dsInfo;
	size_t count = iptc.GetDataSet ( kIPTC_DateCreated, &dsInfo );
	if ( count == 0 ) return;
	
	size_t chPos, digits;
	XMP_DateTime xmpDate;
	memset ( &xmpDate, 0, sizeof(xmpDate) );
	
	for ( chPos = 0, digits = 0; digits < 4; ++digits, ++chPos ) {
		if ( (chPos >= dsInfo.dataLen) || (dsInfo.dataPtr[chPos] < '0') || (dsInfo.dataPtr[chPos] > '9') ) break;
		xmpDate.year = (xmpDate.year * 10) + (dsInfo.dataPtr[chPos] - '0');
	}
	
	if ( dsInfo.dataPtr[chPos] == '-' ) ++chPos;
	for ( digits = 0; digits < 2; ++digits, ++chPos ) {
		if ( (chPos >= dsInfo.dataLen) || (dsInfo.dataPtr[chPos] < '0') || (dsInfo.dataPtr[chPos] > '9') ) break;
		xmpDate.month = (xmpDate.month * 10) + (dsInfo.dataPtr[chPos] - '0');
	}
	if ( xmpDate.month < 1 ) xmpDate.month = 1;
	if ( xmpDate.month > 12 ) xmpDate.month = 12;
	
	if ( dsInfo.dataPtr[chPos] == '-' ) ++chPos;
	for ( digits = 0; digits < 2; ++digits, ++chPos ) {
		if ( (chPos >= dsInfo.dataLen) || (dsInfo.dataPtr[chPos] < '0') || (dsInfo.dataPtr[chPos] > '9') ) break;
		xmpDate.day = (xmpDate.day * 10) + (dsInfo.dataPtr[chPos] - '0');
	}
	if ( xmpDate.day < 1 ) xmpDate.day = 1;
	if ( xmpDate.day > 31 ) xmpDate.day = 28;	// Close enough.
	
	if ( chPos != dsInfo.dataLen ) return;	// The DataSet is ill-formed.

	// Now add the time portion if present.
	
	count = iptc.GetDataSet ( kIPTC_TimeCreated, &dsInfo );
	if ( count != 0 ) {
	
		for ( chPos = 0, digits = 0; digits < 2; ++digits, ++chPos ) {
			if ( (chPos >= dsInfo.dataLen) || (dsInfo.dataPtr[chPos] < '0') || (dsInfo.dataPtr[chPos] > '9') ) break;
			xmpDate.hour = (xmpDate.hour * 10) + (dsInfo.dataPtr[chPos] - '0');
		}
		if ( xmpDate.hour < 0 ) xmpDate.hour = 0;
		if ( xmpDate.hour > 23 ) xmpDate.hour = 23;
		
		if ( dsInfo.dataPtr[chPos] == ':' ) ++chPos;
		for ( digits = 0; digits < 2; ++digits, ++chPos ) {
			if ( (chPos >= dsInfo.dataLen) || (dsInfo.dataPtr[chPos] < '0') || (dsInfo.dataPtr[chPos] > '9') ) break;
			xmpDate.minute = (xmpDate.minute * 10) + (dsInfo.dataPtr[chPos] - '0');
		}
		if ( xmpDate.minute < 0 ) xmpDate.minute = 0;
		if ( xmpDate.minute > 59 ) xmpDate.minute = 59;
		
		if ( dsInfo.dataPtr[chPos] == ':' ) ++chPos;
		for ( digits = 0; digits < 2; ++digits, ++chPos ) {
			if ( (chPos >= dsInfo.dataLen) || (dsInfo.dataPtr[chPos] < '0') || (dsInfo.dataPtr[chPos] > '9') ) break;
			xmpDate.second = (xmpDate.second * 10) + (dsInfo.dataPtr[chPos] - '0');
		}
		if ( xmpDate.second < 0 ) xmpDate.second = 0;
		if ( xmpDate.second > 59 ) xmpDate.second = 59;
		
		if ( dsInfo.dataPtr[chPos] == '+' ) {
			xmpDate.tzSign = kXMP_TimeEastOfUTC;
		} else if ( dsInfo.dataPtr[chPos] == '-' ) {
			xmpDate.tzSign = kXMP_TimeWestOfUTC;
		} else if ( chPos != dsInfo.dataLen ) {
			return;	// The DataSet is ill-formed.
		}
		
		++chPos;	// Move past the time zone sign.
		for ( chPos = 0, digits = 0; digits < 2; ++digits, ++chPos ) {
			if ( (chPos >= dsInfo.dataLen) || (dsInfo.dataPtr[chPos] < '0') || (dsInfo.dataPtr[chPos] > '9') ) break;
			xmpDate.tzHour = (xmpDate.tzHour * 10) + (dsInfo.dataPtr[chPos] - '0');
		}
		if ( xmpDate.tzHour < 0 ) xmpDate.tzHour = 0;
		if ( xmpDate.tzHour > 23 ) xmpDate.tzHour = 23;
		
		if ( dsInfo.dataPtr[chPos] == ':' ) ++chPos;
		for ( digits = 0; digits < 2; ++digits, ++chPos ) {
			if ( (chPos >= dsInfo.dataLen) || (dsInfo.dataPtr[chPos] < '0') || (dsInfo.dataPtr[chPos] > '9') ) break;
			xmpDate.tzMinute = (xmpDate.tzMinute * 10) + (dsInfo.dataPtr[chPos] - '0');
		}
		if ( xmpDate.tzMinute < 0 ) xmpDate.tzMinute = 0;
		if ( xmpDate.tzMinute > 59 ) xmpDate.tzMinute = 59;
	
		if ( chPos != dsInfo.dataLen ) return;	// The DataSet is ill-formed.
	
	}
	
	// Finally, set the XMP property.
	
	xmp->SetProperty_Date ( xmpNS, xmpProp, xmpDate );		

}	// ImportIPTC_DateCreated

// =================================================================================================
// ReconcileUtils::ImportIPTC
// ==========================

void ReconcileUtils::ImportIPTC ( const IPTC_Manager & iptc, SXMPMeta * xmp, int digestState )
{
	if ( digestState == kDigestMatches ) return;

	for ( size_t i = 0; kKnownDataSets[i].id != 255; ++i ) {

		const DataSetCharacteristics & thisDS = kKnownDataSets[i];

		try {	// Don't let errors with one stop the others.
			
			switch ( thisDS.mapForm ) {
	
				case kIPTC_MapSimple :
					ImportIPTC_Simple ( iptc, xmp, digestState, thisDS.id, thisDS.xmpNS, thisDS.xmpProp );
					break;
	
				case kIPTC_MapLangAlt :
					ImportIPTC_LangAlt ( iptc, xmp, digestState, thisDS.id, thisDS.xmpNS, thisDS.xmpProp );
					break;
	
				case kIPTC_MapArray :
					ImportIPTC_Array ( iptc, xmp, digestState, thisDS.id, thisDS.xmpNS, thisDS.xmpProp );
					break;
	
				case kIPTC_MapSpecial :
					if ( thisDS.id == kIPTC_IntellectualGenre ) {
						ImportIPTC_IntellectualGenre ( iptc, xmp, digestState, thisDS.xmpNS, thisDS.xmpProp );
					} else if ( thisDS.id == kIPTC_SubjectCode ) {
						ImportIPTC_SubjectCode ( iptc, xmp, digestState, thisDS.xmpNS, thisDS.xmpProp );
					} else if ( thisDS.id == kIPTC_DateCreated ) {
						ImportIPTC_DateCreated ( iptc, xmp, digestState, thisDS.xmpNS, thisDS.xmpProp );
					} 
					break;
	
			}

		} catch ( ... ) {
	
			// Do nothing, let other imports proceed.
			// ? Notify client?
	
		}

	}
	
}	// ReconcileUtils::ImportIPTC;

// =================================================================================================
// ReconcileUtils::ImportPSIR
// ==========================
//
// There are only 2 standalone Photoshop image resources for XMP properties:
//    1034 - Copyright Flag - 0/1 Boolean mapped to xmpRights:Marked.
//    1035 - Copyright URL - Local OS text mapped to xmpRights:WebStatement.

// ! Photoshop does not use a true/false/missing model for PSIR 1034. Instead it essentially uses a
// ! yes/don't-know model when importing. A missing or 0 value for PSIR 1034 cause xmpRights:Marked
// ! to be deleted.

// **** What about 1008 and 1020?

void ReconcileUtils::ImportPSIR ( const PSIR_Manager & psir, SXMPMeta * xmp, int digestState )
{
	PSIR_Manager::ImgRsrcInfo rsrcInfo;
	bool import;
	
	if ( digestState == kDigestMatches ) return;
	
	if ( digestState == kDigestDiffers ) {
		// Delete the mapped XMP. This forces replacement and catches legacy deletions.
		xmp->DeleteProperty ( kXMP_NS_XMP_Rights, "Marked" );
		xmp->DeleteProperty ( kXMP_NS_XMP_Rights, "WebStatement" );
	}
	
	try {	// Don't let errors with one stop the others.
		import = psir.GetImgRsrc ( kPSIR_CopyrightFlag, &rsrcInfo );
		if ( import ) import = (! xmp->DoesPropertyExist ( kXMP_NS_XMP_Rights, "Marked" ));
		if ( import && (rsrcInfo.dataLen == 1) && (*((XMP_Uns8*)rsrcInfo.dataPtr) != 0) ) {
			xmp->SetProperty_Bool ( kXMP_NS_XMP_Rights, "Marked", true );
		}
	} catch ( ... ) {
		// Do nothing, let other imports proceed.
		// ? Notify client?
	}
	
	try {	// Don't let errors with one stop the others.
		import = psir.GetImgRsrc ( kPSIR_CopyrightURL, &rsrcInfo );
		if ( import ) import = (! xmp->DoesPropertyExist ( kXMP_NS_XMP_Rights, "WebStatement" ));
		if ( import ) {
			std::string utf8;
			ReconcileUtils::LocalToUTF8 ( rsrcInfo.dataPtr, rsrcInfo.dataLen, &utf8 );
			xmp->SetProperty ( kXMP_NS_XMP_Rights, "WebStatement", utf8.c_str() );
		}
	} catch ( ... ) {
		// Do nothing, let other imports proceed.
		// ? Notify client?
	}
	
}	// ReconcileUtils::ImportPSIR;

// =================================================================================================
// =================================================================================================

// =================================================================================================
// ExportIPTC_Simple
// =================

static void ExportIPTC_Simple ( SXMPMeta * xmp, IPTC_Manager * iptc,
								const char * xmpNS, const char * xmpProp, XMP_Uns8 id )
{
	std::string    value;
	XMP_OptionBits xmpFlags;

	bool found = xmp->GetProperty ( xmpNS, xmpProp, &value, &xmpFlags );
	if ( ! found ) {
		iptc->DeleteDataSet ( id );
		return;
	}
	
	if ( ! XMP_PropIsSimple ( xmpFlags ) ) return;	// ? Complain? Delete the DataSet?
	
	NormalizeToCR ( &value );
	
	size_t iptcCount = iptc->GetDataSet ( id, 0 );
	if ( iptcCount > 1 ) iptc->DeleteDataSet ( id );

	iptc->SetDataSet_UTF8 ( id, value.c_str(), value.size(), 0 );	// ! Don't append a 2nd DataSet!

}	// ExportIPTC_Simple

// =================================================================================================
// ExportIPTC_LangAlt
// ==================

static void ExportIPTC_LangAlt ( SXMPMeta * xmp, IPTC_Manager * iptc,
								 const char * xmpNS, const char * xmpProp, XMP_Uns8 id )
{
	std::string    value;
	XMP_OptionBits xmpFlags;

	bool found = xmp->GetProperty ( xmpNS, xmpProp, 0, &xmpFlags );
	if ( ! found ) {
		iptc->DeleteDataSet ( id );
		return;
	}

	if ( ! XMP_ArrayIsAltText ( xmpFlags ) ) return;	// ? Complain? Delete the DataSet?
	
	found = xmp->GetLocalizedText ( xmpNS, xmpProp, "", "x-default", 0, &value, 0 );
	if ( ! found ) {
		iptc->DeleteDataSet ( id );
		return;
	}

	NormalizeToCR ( &value );

	size_t iptcCount = iptc->GetDataSet ( id, 0 );
	if ( iptcCount > 1 ) iptc->DeleteDataSet ( id );

	iptc->SetDataSet_UTF8 ( id, value.c_str(), value.size(), 0 );	// ! Don't append a 2nd DataSet!

}	// ExportIPTC_LangAlt

// =================================================================================================
// ExportIPTC_Array
// ================
//
// Array exporting needs a bit of care to preserve the detection of XMP-only updates. If the current
// XMP and IPTC array sizes differ, delete the entire IPTC and append all new values. If they match,
// set the individual values in order - which lets SetDataSet apply its no-change optimization.

static void ExportIPTC_Array ( SXMPMeta * xmp, IPTC_Manager * iptc,
							   const char * xmpNS, const char * xmpProp, XMP_Uns8 id )
{
	std::string    value;
	XMP_OptionBits xmpFlags;

	bool found = xmp->GetProperty ( xmpNS, xmpProp, 0, &xmpFlags );
	if ( ! found ) {
		iptc->DeleteDataSet ( id );
		return;
	}

	if ( ! XMP_PropIsArray ( xmpFlags ) ) return;	// ? Complain? Delete the DataSet?

	size_t xmpCount  = xmp->CountArrayItems ( xmpNS, xmpProp );
	size_t iptcCount = iptc->GetDataSet ( id, 0 );
	
	if ( xmpCount != iptcCount ) iptc->DeleteDataSet ( id );

	for ( size_t ds = 0; ds < xmpCount; ++ds ) {	// ! XMP arrays are indexed from 1, IPTC from 0.

		(void) xmp->GetArrayItem ( xmpNS, xmpProp, ds+1, &value, &xmpFlags );
		if ( ! XMP_PropIsSimple ( xmpFlags ) ) continue;	// ? Complain?

		NormalizeToCR ( &value );

		iptc->SetDataSet_UTF8 ( id, value.c_str(), value.size(), ds );	// ! Appends if necessary.

	}

}	// ExportIPTC_Array

// =================================================================================================
// ExportIPTC_IntellectualGenre
// ============================
//
// Export DataSet 2:04. In the IIM this is a 3 digit number, a colon, and a text name. Even though
// the number is the more formal part, the IPTC4XMP rule is that the name is imported to XMP and the
// number is dropped. Also, even though IIMv4.1 says that 2:04 is repeatable, the XMP property to
// which it is mapped is simple. Look up the XMP value in a list of known genres to get the number.

static void ExportIPTC_IntellectualGenre ( SXMPMeta * xmp, IPTC_Manager * iptc,
										   const char * xmpNS, const char * xmpProp )
{
	std::string    xmpValue;
	XMP_OptionBits xmpFlags;

	bool found = xmp->GetProperty ( xmpNS, xmpProp, &xmpValue, &xmpFlags );
	if ( ! found ) {
		iptc->DeleteDataSet ( kIPTC_IntellectualGenre );
		return;
	}
	
	if ( ! XMP_PropIsSimple ( xmpFlags ) ) return;	// ? Complain? Delete the DataSet?
	
	NormalizeToCR ( &xmpValue );

	int i;
	XMP_StringPtr namePtr = xmpValue.c_str();
	for ( i = 0; kIntellectualGenreMappings[i].name != 0; ++i ) {
		if ( strcmp ( namePtr, kIntellectualGenreMappings[i].name ) == 0 ) break;
	}
	if ( kIntellectualGenreMappings[i].name == 0 ) return;	// Not a known genre, don't export it.
	
	std::string iimValue = kIntellectualGenreMappings[i].refNum;
	iimValue += ':';
	iimValue += xmpValue;
	
	size_t iptcCount = iptc->GetDataSet ( kIPTC_IntellectualGenre, 0 );
	if ( iptcCount > 1 ) iptc->DeleteDataSet ( kIPTC_IntellectualGenre );

	iptc->SetDataSet_UTF8 ( kIPTC_IntellectualGenre, iimValue.c_str(), iimValue.size(), 0 );	// ! Don't append a 2nd DataSet!

}	// ExportIPTC_IntellectualGenre

// =================================================================================================
// ExportIPTC_SubjectCode
// ======================
//
// Export 2:12 DataSets from an unordered array. In the IIM each DataSet is composed of 5 colon
// separated sections: a provider name, an 8 digit reference number, and 3 optional names for the
// levels of the reference number hierarchy. The IPTC4XMP mapping rule is that only the reference
// number is imported to XMP. We export with a fixed provider of "IPTC" and no optional names.

static void ExportIPTC_SubjectCode ( SXMPMeta * xmp, IPTC_Manager * iptc,
									 const char * xmpNS, const char * xmpProp )
{
	std::string    xmpValue, iimValue;
	XMP_OptionBits xmpFlags;

	bool found = xmp->GetProperty ( xmpNS, xmpProp, 0, &xmpFlags );
	if ( ! found ) {
		iptc->DeleteDataSet ( kIPTC_SubjectCode );
		return;
	}

	if ( ! XMP_PropIsArray ( xmpFlags ) ) return;	// ? Complain? Delete the DataSet?

	size_t xmpCount  = xmp->CountArrayItems ( xmpNS, xmpProp );
	size_t iptcCount = iptc->GetDataSet ( kIPTC_SubjectCode, 0 );
	
	if ( xmpCount != iptcCount ) iptc->DeleteDataSet ( kIPTC_SubjectCode );

	for ( size_t ds = 0; ds < xmpCount; ++ds ) {	// ! XMP arrays are indexed from 1, IPTC from 0.

		(void) xmp->GetArrayItem ( xmpNS, xmpProp, ds+1, &xmpValue, &xmpFlags );
		if ( ! XMP_PropIsSimple ( xmpFlags ) ) continue;	// ? Complain?
		if ( xmpValue.size() != 8 ) continue;	// ? Complain?

		iimValue = "IPTC:";
		iimValue += xmpValue;
		iimValue += ":::";	// Add the separating colons for the empty name portions.

		iptc->SetDataSet_UTF8 ( kIPTC_SubjectCode, iimValue.c_str(), iimValue.size(), ds );	// ! Appends if necessary.

	}

}	// ExportIPTC_SubjectCode

// =================================================================================================
// ExportIPTC_DateCreated
// ======================
//
// The IPTC date and time are "YYYYMMDD" and "HHMMSSxHHMM" where 'x' is '+' or '-'. Export the IPTC
// time only if already present, or if the XMP has a time portion.

static void ExportIPTC_DateCreated ( SXMPMeta * xmp, IPTC_Manager * iptc,
									 const char * xmpNS, const char * xmpProp )
{
	std::string    xmpStr;
	XMP_DateTime   xmpValue;
	XMP_OptionBits xmpFlags;

	bool xmpHasTime = false;

	bool found = xmp->GetProperty ( xmpNS, xmpProp, &xmpStr, &xmpFlags );
	if ( found ) {
		SXMPUtils::ConvertToDate ( xmpStr.c_str(), &xmpValue );
		if ( xmpStr.size() > 10 ) xmpHasTime = true;	// Date-only values are up to "YYYY-MM-DD".
	} else {
		iptc->DeleteDataSet ( kIPTC_DateCreated );
		iptc->DeleteDataSet ( kIPTC_TimeCreated );
		return;
	}

	char iimValue[16];
	
	// Set the IIM date portion.
	
	snprintf ( iimValue, sizeof(iimValue), "%.4d%.2d%.2d",	// AUDIT: Use of sizeof(iimValue) is safe.
			   xmpValue.year, xmpValue.month, xmpValue.day );
	if ( iimValue[8] != 0 ) return;	// ? Complain? Delete the DataSet?
	
	size_t iptcCount = iptc->GetDataSet ( kIPTC_DateCreated, 0 );
	if ( iptcCount > 1 ) iptc->DeleteDataSet ( kIPTC_DateCreated );

	iptc->SetDataSet_UTF8 ( kIPTC_DateCreated, iimValue, 8, 0 );	// ! Don't append a 2nd DataSet!
	
	// Set the IIM time portion.

	iptcCount = iptc->GetDataSet ( kIPTC_TimeCreated, 0 );
	
	if ( (iptcCount > 0) || xmpHasTime ) {
	
		snprintf ( iimValue, sizeof(iimValue), "%.2d%.2d%.2d%c%.2d%.2d",	// AUDIT: Use of sizeof(iimValue) is safe.
				   xmpValue.hour, xmpValue.minute, xmpValue.second,
				   ((xmpValue.tzSign == kXMP_TimeWestOfUTC) ? '-' : '+'), xmpValue.tzHour, xmpValue.tzMinute );
		if ( iimValue[11] != 0 ) return;	// ? Complain? Delete the DataSet?
		
		if ( iptcCount > 1 ) iptc->DeleteDataSet ( kIPTC_TimeCreated );
	
		iptc->SetDataSet_UTF8 ( kIPTC_TimeCreated, iimValue, 11, 0 );	// ! Don't append a 2nd DataSet!
	
	}

}	// ExportIPTC_DateCreated

// =================================================================================================
// ReconcileUtils::ExportIPTC
// ==========================

void ReconcileUtils::ExportIPTC ( SXMPMeta * xmp, IPTC_Manager * iptc )
{
	
	for ( size_t i = 0; kKnownDataSets[i].id != 255; ++i ) {
	
		try {	// Don't let errors with one stop the others.
	
			const DataSetCharacteristics & thisDS = kKnownDataSets[i];
			
			switch ( thisDS.mapForm ) {
	
				case kIPTC_MapSimple :
					ExportIPTC_Simple ( xmp, iptc, thisDS.xmpNS, thisDS.xmpProp, thisDS.id );
					break;
	
				case kIPTC_MapLangAlt :
					ExportIPTC_LangAlt ( xmp, iptc, thisDS.xmpNS, thisDS.xmpProp, thisDS.id );
					break;
	
				case kIPTC_MapArray :
					ExportIPTC_Array ( xmp, iptc, thisDS.xmpNS, thisDS.xmpProp, thisDS.id );
					break;
	
				case kIPTC_MapSpecial :
					if ( thisDS.id == kIPTC_IntellectualGenre ) {
						ExportIPTC_IntellectualGenre ( xmp, iptc, thisDS.xmpNS, thisDS.xmpProp );
					} else if ( thisDS.id == kIPTC_SubjectCode ) {
						ExportIPTC_SubjectCode ( xmp, iptc, thisDS.xmpNS, thisDS.xmpProp );
					} else if ( thisDS.id == kIPTC_DateCreated ) {
						ExportIPTC_DateCreated ( xmp, iptc, thisDS.xmpNS, thisDS.xmpProp );
					} 
					break;
	
			}

		} catch ( ... ) {
	
			// Do nothing, let other exports proceed.
			// ? Notify client?
	
		}

	}
	
}	// ReconcileUtils::ExportIPTC;

// =================================================================================================
// ReconcileUtils::ExportPSIR
// ==========================
//
// There are only 2 standalone Photoshop image resources for XMP properties:
//    1034 - Copyright Flag - 0/1 Boolean mapped to xmpRights:Marked.
//    1035 - Copyright URL - Local OS text mapped to xmpRights:WebStatement.

// ! Photoshop does not use a true/false/missing model for PSIR 1034. Instead it is always written,
// ! a missing xmpRights:Marked results in 0 for PSIR 1034.

// ! We don't bother with the CR<->LF normalization for xmpRights:WebStatement. Very little chance
// ! of having a raw CR character in a URI.

void ReconcileUtils::ExportPSIR ( const SXMPMeta & xmp, PSIR_Manager * psir )
{
	bool found;
	std::string utf8Value;
	
	try {	// Don't let errors with one stop the others.
		bool copyrighted = false;
		found = xmp.GetProperty ( kXMP_NS_XMP_Rights, "Marked", &utf8Value, 0 );
		if ( found ) copyrighted = SXMPUtils::ConvertToBool ( utf8Value );
		psir->SetImgRsrc ( kPSIR_CopyrightFlag, &copyrighted, 1 );
	} catch ( ... ) {
		// Do nothing, let other exports proceed.
		// ? Notify client?
	}
	
	try {	// Don't let errors with one stop the others.
		found = xmp.GetProperty ( kXMP_NS_XMP_Rights, "WebStatement", &utf8Value, 0 );
		if ( ! found ) {
			psir->DeleteImgRsrc ( kPSIR_CopyrightURL );
		} else {
			std::string localValue;
			ReconcileUtils::UTF8ToLocal ( utf8Value.c_str(), utf8Value.size(), &localValue );
			psir->SetImgRsrc ( kPSIR_CopyrightURL, localValue.c_str(), localValue.size() );
		}
	} catch ( ... ) {
		// Do nothing, let other exports proceed.
		// ? Notify client?
	}

}	// ReconcileUtils::ExportPSIR;
