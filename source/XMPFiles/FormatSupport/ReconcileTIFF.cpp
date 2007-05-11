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

#include "UnicodeConversions.hpp"

#include <stdio.h>
#if XMP_WinBuild
	#define snprintf _snprintf
#endif

#if XMP_WinBuild
	#pragma warning ( disable : 4996 )	// '...' was declared deprecated
#endif

// =================================================================================================
/// \file ReconcileTIFF.cpp
/// \brief Utilities to reconcile between XMP and legacy TIFF/Exif metadata.
///
// =================================================================================================

// =================================================================================================

// =================================================================================================
// Tables of the TIFF/Exif tags that are mapped into XMP. For the most part, the tags have obvious
// mappings based on their IFD, tag number, type and count. These tables do not list tags that are
// mapped as subsidiary parts of others, e.g. TIFF SubSecTime or GPS Info GPSDateStamp. Tags that
// have special mappings are marked by having an empty string for the XMP property name.

// ! These tables have the tags listed in the order of tables 3, 4, 5, and 12 of Exif 2.2, with the
// ! exception of ImageUniqueID (which is listed at the end of the Exif mappings). This order is
// ! very important to consistent checking of the legacy status. The NativeDigest properties list
// ! all possible mapped tags in this order. The NativeDigest strings are compared as a whole, so
// ! the same tags listed in a different order would compare as different.

// ! The sentinel tag value can't be 0, that is a valid GPS Info tag, 0xFFFF is unused so far.

struct TIFF_MappingToXMP {
	XMP_Uns16    id;
	XMP_Uns16    type;
	XMP_Uns32    count;	// Zero means any.
	const char * name;	// The name of the mapped XMP property. The namespace is implicit.
};

enum { kAnyCount = 0 };

static const TIFF_MappingToXMP sPrimaryIFDMappings[] = {
	{ /*   256 */ kTIFF_ImageWidth,                kTIFF_ShortOrLongType, 1,         "ImageWidth" },
	{ /*   257 */ kTIFF_ImageLength,               kTIFF_ShortOrLongType, 1,         "ImageLength" },
	{ /*   258 */ kTIFF_BitsPerSample,             kTIFF_ShortType,       3,         "BitsPerSample" },
	{ /*   259 */ kTIFF_Compression,               kTIFF_ShortType,       1,         "Compression" },
	{ /*   262 */ kTIFF_PhotometricInterpretation, kTIFF_ShortType,       1,         "PhotometricInterpretation" },
	{ /*   274 */ kTIFF_Orientation,               kTIFF_ShortType,       1,         "Orientation" },
	{ /*   277 */ kTIFF_SamplesPerPixel,           kTIFF_ShortType,       1,         "SamplesPerPixel" },
	{ /*   284 */ kTIFF_PlanarConfiguration,       kTIFF_ShortType,       1,         "PlanarConfiguration" },
	{ /*   530 */ kTIFF_YCbCrSubSampling,          kTIFF_ShortType,       2,         "YCbCrSubSampling" },
	{ /*   531 */ kTIFF_YCbCrPositioning,          kTIFF_ShortType,       1,         "YCbCrPositioning" },
	{ /*   282 */ kTIFF_XResolution,               kTIFF_RationalType,    1,         "XResolution" },
	{ /*   283 */ kTIFF_YResolution,               kTIFF_RationalType,    1,         "YResolution" },
	{ /*   296 */ kTIFF_ResolutionUnit,            kTIFF_ShortType,       1,         "ResolutionUnit" },
	{ /*   301 */ kTIFF_TransferFunction,          kTIFF_ShortType,       3*256,     "TransferFunction" },
	{ /*   318 */ kTIFF_WhitePoint,                kTIFF_RationalType,    2,         "WhitePoint" },
	{ /*   319 */ kTIFF_PrimaryChromaticities,     kTIFF_RationalType,    6,         "PrimaryChromaticities" },
	{ /*   529 */ kTIFF_YCbCrCoefficients,         kTIFF_RationalType,    3,         "YCbCrCoefficients" },
	{ /*   532 */ kTIFF_ReferenceBlackWhite,       kTIFF_RationalType,    6,         "ReferenceBlackWhite" },
	{ /*   306 */ kTIFF_DateTime,                  kTIFF_ASCIIType,       20,        "" },	// ! Has a special mapping.
	{ /*   270 */ kTIFF_ImageDescription,          kTIFF_ASCIIType,       kAnyCount, "" },	// ! Has a special mapping.
	{ /*   271 */ kTIFF_Make,                      kTIFF_ASCIIType,       kAnyCount, "Make" },
	{ /*   272 */ kTIFF_Model,                     kTIFF_ASCIIType,       kAnyCount, "Model" },
	{ /*   305 */ kTIFF_Software,                  kTIFF_ASCIIType,       kAnyCount, "Software" },	// Has alias to xmp:CreatorTool.
	{ /*   315 */ kTIFF_Artist,                    kTIFF_ASCIIType,       kAnyCount, "" },	// ! Has a special mapping.
	{ /* 33432 */ kTIFF_Copyright,                 kTIFF_ASCIIType,       kAnyCount, "" },
	{ 0xFFFF, 0, 0, 0 }	// ! Must end with sentinel.
};

static const TIFF_MappingToXMP sExifIFDMappings[] = {
	{ /* 36864 */ kTIFF_ExifVersion,               kTIFF_UndefinedType,   4,         "" },	// ! Has a special mapping.
	{ /* 40960 */ kTIFF_FlashpixVersion,           kTIFF_UndefinedType,   4,         "" },	// ! Has a special mapping.
	{ /* 40961 */ kTIFF_ColorSpace,                kTIFF_ShortType,       1,         "ColorSpace" },
	{ /* 37121 */ kTIFF_ComponentsConfiguration,   kTIFF_UndefinedType,   4,         "" },	// ! Has a special mapping.
	{ /* 37122 */ kTIFF_CompressedBitsPerPixel,    kTIFF_RationalType,    1,         "CompressedBitsPerPixel" },
	{ /* 40962 */ kTIFF_PixelXDimension,           kTIFF_ShortOrLongType, 1,         "PixelXDimension" },
	{ /* 40963 */ kTIFF_PixelYDimension,           kTIFF_ShortOrLongType, 1,         "PixelYDimension" },
	{ /* 37510 */ kTIFF_UserComment,               kTIFF_UndefinedType,   kAnyCount, "" },	// ! Has a special mapping.
	{ /* 40964 */ kTIFF_RelatedSoundFile,          kTIFF_ASCIIType,       13,        "RelatedSoundFile" },
	{ /* 36867 */ kTIFF_DateTimeOriginal,          kTIFF_ASCIIType,       20,        "" },	// ! Has a special mapping.
	{ /* 36868 */ kTIFF_DateTimeDigitized,         kTIFF_ASCIIType,       20,        "" },	// ! Has a special mapping.
	{ /* 33434 */ kTIFF_ExposureTime,              kTIFF_RationalType,    1,         "ExposureTime" },
	{ /* 33437 */ kTIFF_FNumber,                   kTIFF_RationalType,    1,         "FNumber" },
	{ /* 34850 */ kTIFF_ExposureProgram,           kTIFF_ShortType,       1,         "ExposureProgram" },
	{ /* 34852 */ kTIFF_SpectralSensitivity,       kTIFF_ASCIIType,       kAnyCount, "SpectralSensitivity" },
	{ /* 34855 */ kTIFF_ISOSpeedRatings,           kTIFF_ShortType,       kAnyCount, "ISOSpeedRatings" },
	{ /* 34856 */ kTIFF_OECF,                      kTIFF_UndefinedType,   kAnyCount, "" },	// ! Has a special mapping.
	{ /* 37377 */ kTIFF_ShutterSpeedValue,         kTIFF_SRationalType,   1,         "ShutterSpeedValue" },
	{ /* 37378 */ kTIFF_ApertureValue,             kTIFF_RationalType,    1,         "ApertureValue" },
	{ /* 37379 */ kTIFF_BrightnessValue,           kTIFF_SRationalType,   1,         "BrightnessValue" },
	{ /* 37380 */ kTIFF_ExposureBiasValue,         kTIFF_SRationalType,   1,         "ExposureBiasValue" },
	{ /* 37381 */ kTIFF_MaxApertureValue,          kTIFF_RationalType,    1,         "MaxApertureValue" },
	{ /* 37382 */ kTIFF_SubjectDistance,           kTIFF_RationalType,    1,         "SubjectDistance" },
	{ /* 37383 */ kTIFF_MeteringMode,              kTIFF_ShortType,       1,         "MeteringMode" },
	{ /* 37384 */ kTIFF_LightSource,               kTIFF_ShortType,       1,         "LightSource" },
	{ /* 37385 */ kTIFF_Flash,                     kTIFF_ShortType,       1,         "" },	// ! Has a special mapping.
	{ /* 37386 */ kTIFF_FocalLength,               kTIFF_RationalType,    1,         "FocalLength" },
	{ /* 37396 */ kTIFF_SubjectArea,               kTIFF_ShortType,       kAnyCount, "SubjectArea" },	// ! Actually 2..4.
	{ /* 41483 */ kTIFF_FlashEnergy,               kTIFF_RationalType,    1,         "FlashEnergy" },
	{ /* 41484 */ kTIFF_SpatialFrequencyResponse,  kTIFF_UndefinedType,   kAnyCount, "" },	// ! Has a special mapping.
	{ /* 41486 */ kTIFF_FocalPlaneXResolution,     kTIFF_RationalType,    1,         "FocalPlaneXResolution" },
	{ /* 41487 */ kTIFF_FocalPlaneYResolution,     kTIFF_RationalType,    1,         "FocalPlaneYResolution" },
	{ /* 41488 */ kTIFF_FocalPlaneResolutionUnit,  kTIFF_ShortType,       1,         "FocalPlaneResolutionUnit" },
	{ /* 41492 */ kTIFF_SubjectLocation,           kTIFF_ShortType,       2,         "SubjectLocation" },
	{ /* 41493 */ kTIFF_ExposureIndex,             kTIFF_RationalType,    1,         "ExposureIndex" },
	{ /* 41495 */ kTIFF_SensingMethod,             kTIFF_ShortType,       1,         "SensingMethod" },
	{ /* 41728 */ kTIFF_FileSource,                kTIFF_UndefinedType,   1,         "" },	// ! Has a special mapping.
	{ /* 41729 */ kTIFF_SceneType,                 kTIFF_UndefinedType,   1,         "" },	// ! Has a special mapping.
	{ /* 41730 */ kTIFF_CFAPattern,                kTIFF_UndefinedType,   kAnyCount, "" },	// ! Has a special mapping.
	{ /* 41985 */ kTIFF_CustomRendered,            kTIFF_ShortType,       1,         "CustomRendered" },
	{ /* 41986 */ kTIFF_ExposureMode,              kTIFF_ShortType,       1,         "ExposureMode" },
	{ /* 41987 */ kTIFF_WhiteBalance,              kTIFF_ShortType,       1,         "WhiteBalance" },
	{ /* 41988 */ kTIFF_DigitalZoomRatio,          kTIFF_RationalType,    1,         "DigitalZoomRatio" },
	{ /* 41989 */ kTIFF_FocalLengthIn35mmFilm,     kTIFF_ShortType,       1,         "FocalLengthIn35mmFilm" },
	{ /* 41990 */ kTIFF_SceneCaptureType,          kTIFF_ShortType,       1,         "SceneCaptureType" },
	{ /* 41991 */ kTIFF_GainControl,               kTIFF_ShortType,       1,         "GainControl" },
	{ /* 41992 */ kTIFF_Contrast,                  kTIFF_ShortType,       1,         "Contrast" },
	{ /* 41993 */ kTIFF_Saturation,                kTIFF_ShortType,       1,         "Saturation" },
	{ /* 41994 */ kTIFF_Sharpness,                 kTIFF_ShortType,       1,         "Sharpness" },
	{ /* 41995 */ kTIFF_DeviceSettingDescription,  kTIFF_UndefinedType,   kAnyCount, "" },	// ! Has a special mapping.
	{ /* 41996 */ kTIFF_SubjectDistanceRange,      kTIFF_ShortType,       1,         "SubjectDistanceRange" },
	{ /* 42016 */ kTIFF_ImageUniqueID,             kTIFF_ASCIIType,       33,        "ImageUniqueID" },
	{ 0xFFFF, 0, 0, 0 }	// ! Must end with sentinel.
};

static const TIFF_MappingToXMP sGPSInfoIFDMappings[] = {
	{ /*     0 */ kTIFF_GPSVersionID,              kTIFF_ByteType,        4,         "" },	// ! Has a special mapping.
	{ /*     2 */ kTIFF_GPSLatitude,               kTIFF_RationalType,    3,         "" },	// ! Has a special mapping.
	{ /*     4 */ kTIFF_GPSLongitude,              kTIFF_RationalType,    3,         "" },	// ! Has a special mapping.
	{ /*     5 */ kTIFF_GPSAltitudeRef,            kTIFF_ByteType,        1,         "GPSAltitudeRef" },
	{ /*     6 */ kTIFF_GPSAltitude,               kTIFF_RationalType,    1,         "GPSAltitude" },
	{ /*     7 */ kTIFF_GPSTimeStamp,              kTIFF_RationalType,    3,         "" },	// ! Has a special mapping.
	{ /*     8 */ kTIFF_GPSSatellites,             kTIFF_ASCIIType,       kAnyCount, "GPSSatellites" },
	{ /*     9 */ kTIFF_GPSStatus,                 kTIFF_ASCIIType,       2,         "GPSStatus" },
	{ /*    10 */ kTIFF_GPSMeasureMode,            kTIFF_ASCIIType,       2,         "GPSMeasureMode" },
	{ /*    11 */ kTIFF_GPSDOP,                    kTIFF_RationalType,    1,         "GPSDOP" },
	{ /*    12 */ kTIFF_GPSSpeedRef,               kTIFF_ASCIIType,       2,         "GPSSpeedRef" },
	{ /*    13 */ kTIFF_GPSSpeed,                  kTIFF_RationalType,    1,         "GPSSpeed" },
	{ /*    14 */ kTIFF_GPSTrackRef,               kTIFF_ASCIIType,       2,         "GPSTrackRef" },
	{ /*    15 */ kTIFF_GPSTrack,                  kTIFF_RationalType,    1,         "GPSTrack" },
	{ /*    16 */ kTIFF_GPSImgDirectionRef,        kTIFF_ASCIIType,       2,         "GPSImgDirectionRef" },
	{ /*    17 */ kTIFF_GPSImgDirection,           kTIFF_RationalType,    1,         "GPSImgDirection" },
	{ /*    18 */ kTIFF_GPSMapDatum,               kTIFF_ASCIIType,       kAnyCount, "GPSMapDatum" },
	{ /*    20 */ kTIFF_GPSDestLatitude,           kTIFF_RationalType,    3,         "" },	// ! Has a special mapping.
	{ /*    22 */ kTIFF_GPSDestLongitude,          kTIFF_RationalType,    3,         "" },	// ! Has a special mapping.
	{ /*    23 */ kTIFF_GPSDestBearingRef,         kTIFF_ASCIIType,       2,         "GPSDestBearingRef" },
	{ /*    24 */ kTIFF_GPSDestBearing,            kTIFF_RationalType,    1,         "GPSDestBearing" },
	{ /*    25 */ kTIFF_GPSDestDistanceRef,        kTIFF_ASCIIType,       2,         "GPSDestDistanceRef" },
	{ /*    26 */ kTIFF_GPSDestDistance,           kTIFF_RationalType,    1,         "GPSDestDistance" },
	{ /*    27 */ kTIFF_GPSProcessingMethod,       kTIFF_UndefinedType,   kAnyCount, "" },	// ! Has a special mapping.
	{ /*    28 */ kTIFF_GPSAreaInformation,        kTIFF_UndefinedType,   kAnyCount, "" },	// ! Has a special mapping.
	{ /*    30 */ kTIFF_GPSDifferential,           kTIFF_ShortType,       1,         "GPSDifferential" },
	{ 0xFFFF, 0, 0, 0 }	// ! Must end with sentinel.
};

// =================================================================================================

static XMP_Uns32 GatherInt ( const char * strPtr, size_t count )
{
	XMP_Uns32 value = 0;
	const char * strEnd = strPtr + count;

	while ( strPtr < strEnd ) {
		char ch = *strPtr;
		if ( (ch < '0') || (ch > '9') ) break;
		value = value*10 + (ch - '0');
		++strPtr;
	}

	return value;

}

// =================================================================================================
// =================================================================================================

// =================================================================================================
// ComputeTIFFDigest
// =================
//
// Compute a 128 bit (16 byte) MD5 digest of the mapped TIFF tags and format it as a string like:
//    256,257,...;A0FCE844924381619820B6F7117C8B83
// The first portion is a decimal list of the tags from sPrimaryIFDMappings, the last part is the
// MD5 digest as 32 hex digits using capital A-F.

// ! The order of listing for the tags is crucial for the change comparisons to work!

static void
ComputeTIFFDigest ( const TIFF_Manager & tiff, std::string * digestStr )
{
	MD5_CTX    context;
	MD5_Digest digest;
	char       buffer[40];
	size_t     in, out;

	TIFF_Manager::TagInfo tagInfo;

	MD5Init ( &context );
	digestStr->clear();
	digestStr->reserve ( 160 );	// The current length is 134.

	for ( size_t i = 0; sPrimaryIFDMappings[i].id != 0xFFFF; ++i ) {
		snprintf ( buffer, sizeof(buffer), "%d,", sPrimaryIFDMappings[i].id );	// AUDIT: Use of sizeof(buffer) is safe.
		digestStr->append ( buffer );
		bool found = tiff.GetTag ( kTIFF_PrimaryIFD, sPrimaryIFDMappings[i].id, &tagInfo );
		if ( found ) MD5Update ( &context, (XMP_Uns8*)tagInfo.dataPtr, tagInfo.dataLen );
	}

	size_t endPos = digestStr->size() - 1;
	(*digestStr)[endPos] = ';';

	MD5Final ( digest, &context );

	for ( in = 0, out = 0; in < 16; in += 1, out += 2 ) {
		XMP_Uns8 byte = digest[in];
		buffer[out]   = ReconcileUtils::kHexDigits [ byte >> 4 ];
		buffer[out+1] = ReconcileUtils::kHexDigits [ byte & 0xF ];
	}
	buffer[32] = 0;

	digestStr->append ( buffer );

}	// ComputeTIFFDigest;

// =================================================================================================
// ComputeExifDigest
// =================
//
// Compute a 128 bit (16 byte) MD5 digest of the mapped Exif andf GPS tags and format it as a string like:
//    36864,40960,...;A0FCE844924381619820B6F7117C8B83
// The first portion is a decimal list of the tags, the last part is the MD5 digest as 32 hex
// digits using capital A-F. The listed tags are those from sExifIFDMappings followed by those from
// sGPSInfoIFDMappings.

// ! The order of listing for the tags is crucial for the change comparisons to work!

static void
ComputeExifDigest ( const TIFF_Manager & exif, std::string * digestStr )
{
	MD5_CTX    context;
	MD5_Digest digest;
	char       buffer[40];
	size_t     in, out;

	TIFF_Manager::TagInfo tagInfo;

	MD5Init ( &context );
	digestStr->clear();
	digestStr->reserve ( 440 );	// The current length is 414.

	for ( size_t i = 0; sExifIFDMappings[i].id != 0xFFFF; ++i ) {
		snprintf ( buffer, sizeof(buffer), "%d,", sExifIFDMappings[i].id );	// AUDIT: Use of sizeof(buffer) is safe.
		digestStr->append ( buffer );
		bool found = exif.GetTag ( kTIFF_ExifIFD, sExifIFDMappings[i].id, &tagInfo );
		if ( found ) MD5Update ( &context, (XMP_Uns8*)tagInfo.dataPtr, tagInfo.dataLen );
	}

	for ( size_t i = 0; sGPSInfoIFDMappings[i].id != 0xFFFF; ++i ) {
		snprintf ( buffer, sizeof(buffer), "%d,", sGPSInfoIFDMappings[i].id );	// AUDIT: Use of sizeof(buffer) is safe.
		digestStr->append ( buffer );
		bool found = exif.GetTag ( kTIFF_GPSInfoIFD, sGPSInfoIFDMappings[i].id, &tagInfo );
		if ( found ) MD5Update ( &context, (XMP_Uns8*)tagInfo.dataPtr, tagInfo.dataLen );
	}

	size_t endPos = digestStr->size() - 1;
	(*digestStr)[endPos] = ';';

	MD5Final ( digest, &context );

	for ( in = 0, out = 0; in < 16; in += 1, out += 2 ) {
		XMP_Uns8 byte = digest[in];
		buffer[out]   = ReconcileUtils::kHexDigits [ byte >> 4 ];
		buffer[out+1] = ReconcileUtils::kHexDigits [ byte & 0xF ];
	}
	buffer[32] = 0;

	digestStr->append ( buffer );

}	// ComputeExifDigest;

// =================================================================================================
// ReconcileUtils::CheckTIFFDigest
// ===============================

int
ReconcileUtils::CheckTIFFDigest ( const TIFF_Manager & tiff, const SXMPMeta & xmp )
{
	std::string newDigest, oldDigest;

	ComputeTIFFDigest ( tiff, &newDigest );
	bool found = xmp.GetProperty ( kXMP_NS_TIFF, "NativeDigest", &oldDigest, 0 );

	if ( ! found ) return kDigestMissing;
	if ( newDigest == oldDigest ) return kDigestMatches;
	return kDigestDiffers;

}	// ReconcileUtils::CheckTIFFDigest;

// =================================================================================================
// ReconcileUtils::CheckExifDigest
// ===============================

int
ReconcileUtils::CheckExifDigest ( const TIFF_Manager & tiff, const SXMPMeta & xmp )
{
	std::string newDigest, oldDigest;

	ComputeExifDigest ( tiff, &newDigest );
	bool found = xmp.GetProperty ( kXMP_NS_EXIF, "NativeDigest", &oldDigest, 0 );

	if ( ! found ) return kDigestMissing;
	if ( newDigest == oldDigest ) return kDigestMatches;
	return kDigestDiffers;

}	// ReconcileUtils::CheckExifDigest;

// =================================================================================================
// ReconcileUtils::SetTIFFDigest
// =============================

void
ReconcileUtils::SetTIFFDigest ( const TIFF_Manager & tiff, SXMPMeta * xmp )
{
	std::string newDigest;

	ComputeTIFFDigest ( tiff, &newDigest );
	xmp->SetProperty ( kXMP_NS_TIFF, "NativeDigest", newDigest.c_str() );

}	// ReconcileUtils::SetTIFFDigest;

// =================================================================================================
// ReconcileUtils::SetExifDigest
// =============================

void
ReconcileUtils::SetExifDigest ( const TIFF_Manager & tiff, SXMPMeta * xmp )
{
	std::string newDigest;

	ComputeExifDigest ( tiff, &newDigest );
	xmp->SetProperty ( kXMP_NS_EXIF, "NativeDigest", newDigest.c_str() );

}	// ReconcileUtils::SetExifDigest;

// =================================================================================================
// =================================================================================================

// =================================================================================================
// ImportSingleTIFF_Short
// ======================

static void
ImportSingleTIFF_Short ( const TIFF_Manager::TagInfo & tagInfo, const bool nativeEndian,
						 SXMPMeta * xmp, const char * xmpNS, const char * xmpProp )
{
	try {	// Don't let errors with one stop the others.

		XMP_Uns16 binValue = *((XMP_Uns16*)tagInfo.dataPtr);
		if ( ! nativeEndian ) binValue = Flip2 ( binValue );
	
		char strValue[20];
		snprintf ( strValue, sizeof(strValue), "%hu", binValue );	// AUDIT: Using sizeof(strValue) is safe.
	
		xmp->SetProperty ( xmpNS, xmpProp, strValue );

	} catch ( ... ) {
		// Do nothing, let other imports proceed.
		// ? Notify client?
	}

}	// ImportSingleTIFF_Short

// =================================================================================================
// ImportSingleTIFF_Long
// =====================

static void
ImportSingleTIFF_Long ( const TIFF_Manager::TagInfo & tagInfo, const bool nativeEndian,
						SXMPMeta * xmp, const char * xmpNS, const char * xmpProp )
{
	try {	// Don't let errors with one stop the others.

		XMP_Uns32 binValue = *((XMP_Uns32*)tagInfo.dataPtr);
		if ( ! nativeEndian ) binValue = Flip4 ( binValue );
	
		char strValue[20];
		snprintf ( strValue, sizeof(strValue), "%lu", binValue );	// AUDIT: Using sizeof(strValue) is safe.
	
		xmp->SetProperty ( xmpNS, xmpProp, strValue );

	} catch ( ... ) {
		// Do nothing, let other imports proceed.
		// ? Notify client?
	}

}	// ImportSingleTIFF_Long

// =================================================================================================
// ImportSingleTIFF_Rational
// =========================

static void
ImportSingleTIFF_Rational ( const TIFF_Manager::TagInfo & tagInfo, const bool nativeEndian,
							SXMPMeta * xmp, const char * xmpNS, const char * xmpProp )
{
	try {	// Don't let errors with one stop the others.

		XMP_Uns32 * binPtr = (XMP_Uns32*)tagInfo.dataPtr;
		XMP_Uns32 binNum   = binPtr[0];
		XMP_Uns32 binDenom = binPtr[1];
		if ( ! nativeEndian ) {
			binNum = Flip4 ( binNum );
			binDenom = Flip4 ( binDenom );
		}
	
		char strValue[40];
		snprintf ( strValue, sizeof(strValue), "%lu/%lu", binNum, binDenom );	// AUDIT: Using sizeof(strValue) is safe.
	
		xmp->SetProperty ( xmpNS, xmpProp, strValue );

	} catch ( ... ) {
		// Do nothing, let other imports proceed.
		// ? Notify client?
	}

}	// ImportSingleTIFF_Rational

// =================================================================================================
// ImportSingleTIFF_SRational
// ==========================

static void
ImportSingleTIFF_SRational ( const TIFF_Manager::TagInfo & tagInfo, const bool nativeEndian,
							 SXMPMeta * xmp, const char * xmpNS, const char * xmpProp )
{
	try {	// Don't let errors with one stop the others.

		XMP_Int32 * binPtr = (XMP_Int32*)tagInfo.dataPtr;
		XMP_Int32 binNum   = binPtr[0];
		XMP_Int32 binDenom = binPtr[1];
		if ( ! nativeEndian ) {
			Flip4 ( &binNum );
			Flip4 ( &binDenom );
		}
	
		char strValue[40];
		snprintf ( strValue, sizeof(strValue), "%ld/%ld", binNum, binDenom );	// AUDIT: Using sizeof(strValue) is safe.
	
		xmp->SetProperty ( xmpNS, xmpProp, strValue );

	} catch ( ... ) {
		// Do nothing, let other imports proceed.
		// ? Notify client?
	}

}	// ImportSingleTIFF_SRational

// =================================================================================================
// ImportSingleTIFF_ASCII
// ======================

static void
ImportSingleTIFF_ASCII ( const TIFF_Manager::TagInfo & tagInfo,
						 SXMPMeta * xmp, const char * xmpNS, const char * xmpProp )
{
	try {	// Don't let errors with one stop the others.

		const char * chPtr  = (const char *)tagInfo.dataPtr;
		const bool   hasNul = (chPtr[tagInfo.dataLen-1] == 0);
		const bool   isUTF8 = ReconcileUtils::IsUTF8 ( chPtr, tagInfo.dataLen );
	
		if ( isUTF8 && hasNul ) {
			xmp->SetProperty ( xmpNS, xmpProp, chPtr );
		} else {
			std::string strValue;
			if ( isUTF8 ) {
				strValue.assign ( chPtr, tagInfo.dataLen );
			} else {
				ReconcileUtils::LocalToUTF8 ( chPtr, tagInfo.dataLen, &strValue );
			}
			xmp->SetProperty ( xmpNS, xmpProp, strValue.c_str() );
		}

	} catch ( ... ) {
		// Do nothing, let other imports proceed.
		// ? Notify client?
	}

}	// ImportSingleTIFF_ASCII

// =================================================================================================
// ImportSingleTIFF_Byte
// =====================

static void
ImportSingleTIFF_Byte ( const TIFF_Manager::TagInfo & tagInfo,
						SXMPMeta * xmp, const char * xmpNS, const char * xmpProp )
{
	try {	// Don't let errors with one stop the others.

		XMP_Uns8 binValue = *((XMP_Uns8*)tagInfo.dataPtr);
	
		char strValue[20];
		snprintf ( strValue, sizeof(strValue), "%hu", binValue );	// AUDIT: Using sizeof(strValue) is safe.
	
		xmp->SetProperty ( xmpNS, xmpProp, strValue );

	} catch ( ... ) {
		// Do nothing, let other imports proceed.
		// ? Notify client?
	}

}	// ImportSingleTIFF_Byte

// =================================================================================================
// ImportSingleTIFF_SByte
// ======================

static void
ImportSingleTIFF_SByte ( const TIFF_Manager::TagInfo & tagInfo,
						 SXMPMeta * xmp, const char * xmpNS, const char * xmpProp )
{
	try {	// Don't let errors with one stop the others.

		XMP_Int8 binValue = *((XMP_Int8*)tagInfo.dataPtr);
	
		char strValue[20];
		snprintf ( strValue, sizeof(strValue), "%hd", binValue );	// AUDIT: Using sizeof(strValue) is safe.
	
		xmp->SetProperty ( xmpNS, xmpProp, strValue );

	} catch ( ... ) {
		// Do nothing, let other imports proceed.
		// ? Notify client?
	}

}	// ImportSingleTIFF_SByte

// =================================================================================================
// ImportSingleTIFF_SShort
// =======================

static void
ImportSingleTIFF_SShort ( const TIFF_Manager::TagInfo & tagInfo, const bool nativeEndian,
						  SXMPMeta * xmp, const char * xmpNS, const char * xmpProp )
{
	try {	// Don't let errors with one stop the others.

		XMP_Int16 binValue = *((XMP_Int16*)tagInfo.dataPtr);
		if ( ! nativeEndian ) Flip2 ( &binValue );
	
		char strValue[20];
		snprintf ( strValue, sizeof(strValue), "%hd", binValue );	// AUDIT: Using sizeof(strValue) is safe.
	
		xmp->SetProperty ( xmpNS, xmpProp, strValue );

	} catch ( ... ) {
		// Do nothing, let other imports proceed.
		// ? Notify client?
	}

}	// ImportSingleTIFF_SShort

// =================================================================================================
// ImportSingleTIFF_SLong
// ======================

static void
ImportSingleTIFF_SLong ( const TIFF_Manager::TagInfo & tagInfo, const bool nativeEndian,
						 SXMPMeta * xmp, const char * xmpNS, const char * xmpProp )
{
	try {	// Don't let errors with one stop the others.

		XMP_Int32 binValue = *((XMP_Int32*)tagInfo.dataPtr);
		if ( ! nativeEndian ) Flip4 ( &binValue );
	
		char strValue[20];
		snprintf ( strValue, sizeof(strValue), "%ld", binValue );	// AUDIT: Using sizeof(strValue) is safe.
	
		xmp->SetProperty ( xmpNS, xmpProp, strValue );

	} catch ( ... ) {
		// Do nothing, let other imports proceed.
		// ? Notify client?
	}

}	// ImportSingleTIFF_SLong

// =================================================================================================
// ImportSingleTIFF_Float
// ======================

static void
ImportSingleTIFF_Float ( const TIFF_Manager::TagInfo & tagInfo, const bool nativeEndian,
						 SXMPMeta * xmp, const char * xmpNS, const char * xmpProp )
{
	try {	// Don't let errors with one stop the others.

		float binValue = *((float*)tagInfo.dataPtr);
		if ( ! nativeEndian ) Flip4 ( &binValue );
	
		xmp->SetProperty_Float ( xmpNS, xmpProp, binValue );

	} catch ( ... ) {
		// Do nothing, let other imports proceed.
		// ? Notify client?
	}

}	// ImportSingleTIFF_Float

// =================================================================================================
// ImportSingleTIFF_Double
// =======================

static void
ImportSingleTIFF_Double ( const TIFF_Manager::TagInfo & tagInfo, const bool nativeEndian,
						  SXMPMeta * xmp, const char * xmpNS, const char * xmpProp )
{
	try {	// Don't let errors with one stop the others.

		double binValue = *((double*)tagInfo.dataPtr);
		if ( ! nativeEndian ) Flip8 ( &binValue );
	
		xmp->SetProperty_Float ( xmpNS, xmpProp, binValue );	// ! Yes, SetProperty_Float.

	} catch ( ... ) {
		// Do nothing, let other imports proceed.
		// ? Notify client?
	}

}	// ImportSingleTIFF_Double

// =================================================================================================
// ImportSingleTIFF
// ================

static void
ImportSingleTIFF ( const TIFF_Manager::TagInfo & tagInfo, const bool nativeEndian,
				   SXMPMeta * xmp, const char * xmpNS, const char * xmpProp )
{

	// We've got a tag to map to XMP, decide how based on actual type and the expected count. Using
	// the actual type eliminates a ShortOrLong case. Using the expected count is needed to know
	// whether to create an XMP array. The actual count for an array could be 1. Put the most
	// common cases first for better iCache utilization.

	switch ( tagInfo.type ) {

		case kTIFF_ShortType :
			ImportSingleTIFF_Short ( tagInfo, nativeEndian, xmp, xmpNS, xmpProp );
			break;

		case kTIFF_LongType :
			ImportSingleTIFF_Long ( tagInfo, nativeEndian, xmp, xmpNS, xmpProp );
			break;

		case kTIFF_RationalType :
			ImportSingleTIFF_Rational ( tagInfo, nativeEndian, xmp, xmpNS, xmpProp );
			break;

		case kTIFF_SRationalType :
			ImportSingleTIFF_SRational ( tagInfo, nativeEndian, xmp, xmpNS, xmpProp );
			break;

		case kTIFF_ASCIIType :
			ImportSingleTIFF_ASCII ( tagInfo, xmp, xmpNS, xmpProp );
			break;

		case kTIFF_ByteType :
			ImportSingleTIFF_Byte ( tagInfo, xmp, xmpNS, xmpProp );
			break;

		case kTIFF_SByteType :
			ImportSingleTIFF_SByte ( tagInfo, xmp, xmpNS, xmpProp );
			break;

		case kTIFF_SShortType :
			ImportSingleTIFF_SShort ( tagInfo, nativeEndian, xmp, xmpNS, xmpProp );
			break;

		case kTIFF_SLongType :
			ImportSingleTIFF_SLong ( tagInfo, nativeEndian, xmp, xmpNS, xmpProp );
			break;

		case kTIFF_FloatType :
			ImportSingleTIFF_Float ( tagInfo, nativeEndian, xmp, xmpNS, xmpProp );
			break;

		case kTIFF_DoubleType :
			ImportSingleTIFF_Double ( tagInfo, nativeEndian, xmp, xmpNS, xmpProp );
			break;

	}

}	// ImportSingleTIFF

// =================================================================================================
// =================================================================================================

// =================================================================================================
// ImportArrayTIFF_Short
// =====================

static void
ImportArrayTIFF_Short ( const TIFF_Manager::TagInfo & tagInfo, const bool nativeEndian,
						SXMPMeta * xmp, const char * xmpNS, const char * xmpProp )
{
	try {	// Don't let errors with one stop the others.

		XMP_Uns16 * binPtr = (XMP_Uns16*)tagInfo.dataPtr;
	
		for ( size_t i = 0; i < tagInfo.count; ++i, ++binPtr ) {
	
			XMP_Uns16 binValue = *binPtr;
			if ( ! nativeEndian ) binValue = Flip2 ( binValue );
	
			char strValue[20];
			snprintf ( strValue, sizeof(strValue), "%hu", binValue );	// AUDIT: Using sizeof(strValue) is safe.
	
			xmp->AppendArrayItem ( xmpNS, xmpProp, kXMP_PropArrayIsOrdered, strValue );
	
		}

	} catch ( ... ) {
		// Do nothing, let other imports proceed.
		// ? Notify client?
	}

}	// ImportArrayTIFF_Short

// =================================================================================================
// ImportArrayTIFF_Long
// ====================

static void
ImportArrayTIFF_Long ( const TIFF_Manager::TagInfo & tagInfo, const bool nativeEndian,
					   SXMPMeta * xmp, const char * xmpNS, const char * xmpProp )
{
	try {	// Don't let errors with one stop the others.

		XMP_Uns32 * binPtr = (XMP_Uns32*)tagInfo.dataPtr;
	
		for ( size_t i = 0; i < tagInfo.count; ++i, ++binPtr ) {
	
			XMP_Uns32 binValue = *binPtr;
			if ( ! nativeEndian ) binValue = Flip4 ( binValue );
	
			char strValue[20];
			snprintf ( strValue, sizeof(strValue), "%lu", binValue );	// AUDIT: Using sizeof(strValue) is safe.
	
			xmp->AppendArrayItem ( xmpNS, xmpProp, kXMP_PropArrayIsOrdered, strValue );
	
		}

	} catch ( ... ) {
		// Do nothing, let other imports proceed.
		// ? Notify client?
	}

}	// ImportArrayTIFF_Long

// =================================================================================================
// ImportArrayTIFF_Rational
// ========================

static void
ImportArrayTIFF_Rational ( const TIFF_Manager::TagInfo & tagInfo, const bool nativeEndian,
						   SXMPMeta * xmp, const char * xmpNS, const char * xmpProp )
{
	try {	// Don't let errors with one stop the others.

		XMP_Uns32 * binPtr = (XMP_Uns32*)tagInfo.dataPtr;
	
		for ( size_t i = 0; i < tagInfo.count; ++i, binPtr += 2 ) {
	
			XMP_Uns32 binNum   = binPtr[0];
			XMP_Uns32 binDenom = binPtr[1];
			if ( ! nativeEndian ) {
				binNum = Flip4 ( binNum );
				binDenom = Flip4 ( binDenom );
			}
	
			char strValue[40];
			snprintf ( strValue, sizeof(strValue), "%lu/%lu", binNum, binDenom );	// AUDIT: Using sizeof(strValue) is safe.
	
			xmp->AppendArrayItem ( xmpNS, xmpProp, kXMP_PropArrayIsOrdered, strValue );
	
		}

	} catch ( ... ) {
		// Do nothing, let other imports proceed.
		// ? Notify client?
	}

}	// ImportArrayTIFF_Rational

// =================================================================================================
// ImportArrayTIFF_SRational
// =========================

static void
ImportArrayTIFF_SRational ( const TIFF_Manager::TagInfo & tagInfo, const bool nativeEndian,
							SXMPMeta * xmp, const char * xmpNS, const char * xmpProp )
{
	try {	// Don't let errors with one stop the others.

		XMP_Int32 * binPtr = (XMP_Int32*)tagInfo.dataPtr;
	
		for ( size_t i = 0; i < tagInfo.count; ++i, binPtr += 2 ) {
	
			XMP_Int32 binNum   = binPtr[0];
			XMP_Int32 binDenom = binPtr[1];
			if ( ! nativeEndian ) {
				Flip4 ( &binNum );
				Flip4 ( &binDenom );
			}
	
			char strValue[40];
			snprintf ( strValue, sizeof(strValue), "%ld/%ld", binNum, binDenom );	// AUDIT: Using sizeof(strValue) is safe.
	
			xmp->AppendArrayItem ( xmpNS, xmpProp, kXMP_PropArrayIsOrdered, strValue );
	
		}

	} catch ( ... ) {
		// Do nothing, let other imports proceed.
		// ? Notify client?
	}

}	// ImportArrayTIFF_SRational

// =================================================================================================
// ImportArrayTIFF_ASCII
// =====================

static void
ImportArrayTIFF_ASCII ( const TIFF_Manager::TagInfo & tagInfo,
						SXMPMeta * xmp, const char * xmpNS, const char * xmpProp )
{
	try {	// Don't let errors with one stop the others.

		const char * chPtr  = (const char *)tagInfo.dataPtr;
		const char * chEnd  = chPtr + tagInfo.dataLen;
		const bool   hasNul = (chPtr[tagInfo.dataLen-1] == 0);
		const bool   isUTF8 = ReconcileUtils::IsUTF8 ( chPtr, tagInfo.dataLen );
	
		std::string  strValue;
	
		if ( (! isUTF8) || (! hasNul) ) {
			if ( isUTF8 ) {
				strValue.assign ( chPtr, tagInfo.dataLen );
			} else {
				ReconcileUtils::LocalToUTF8 ( chPtr, tagInfo.dataLen, &strValue );
			}
			chPtr = strValue.c_str();
			chEnd = chPtr + strValue.size();
		}
	
		for ( ; chPtr < chEnd; chPtr += (strlen(chPtr) + 1) ) {
			xmp->AppendArrayItem ( xmpNS, xmpProp, kXMP_PropArrayIsOrdered, chPtr );
		}

	} catch ( ... ) {
		// Do nothing, let other imports proceed.
		// ? Notify client?
	}

}	// ImportArrayTIFF_ASCII

// =================================================================================================
// ImportArrayTIFF_Byte
// ====================

static void
ImportArrayTIFF_Byte ( const TIFF_Manager::TagInfo & tagInfo,
					   SXMPMeta * xmp, const char * xmpNS, const char * xmpProp )
{
	try {	// Don't let errors with one stop the others.

		XMP_Uns8 * binPtr = (XMP_Uns8*)tagInfo.dataPtr;
	
		for ( size_t i = 0; i < tagInfo.count; ++i, ++binPtr ) {
	
			XMP_Uns8 binValue = *binPtr;
	
			char strValue[20];
			snprintf ( strValue, sizeof(strValue), "%hu", binValue );	// AUDIT: Using sizeof(strValue) is safe.
	
			xmp->AppendArrayItem ( xmpNS, xmpProp, kXMP_PropArrayIsOrdered, strValue );
	
		}

	} catch ( ... ) {
		// Do nothing, let other imports proceed.
		// ? Notify client?
	}

}	// ImportArrayTIFF_Byte

// =================================================================================================
// ImportArrayTIFF_SByte
// =====================

static void
ImportArrayTIFF_SByte ( const TIFF_Manager::TagInfo & tagInfo,
						SXMPMeta * xmp, const char * xmpNS, const char * xmpProp )
{
	try {	// Don't let errors with one stop the others.

		XMP_Int8 * binPtr = (XMP_Int8*)tagInfo.dataPtr;
	
		for ( size_t i = 0; i < tagInfo.count; ++i, ++binPtr ) {
	
			XMP_Int8 binValue = *binPtr;
	
			char strValue[20];
			snprintf ( strValue, sizeof(strValue), "%hd", binValue );	// AUDIT: Using sizeof(strValue) is safe.
	
			xmp->AppendArrayItem ( xmpNS, xmpProp, kXMP_PropArrayIsOrdered, strValue );
	
		}

	} catch ( ... ) {
		// Do nothing, let other imports proceed.
		// ? Notify client?
	}

}	// ImportArrayTIFF_SByte

// =================================================================================================
// ImportArrayTIFF_SShort
// ======================

static void
ImportArrayTIFF_SShort ( const TIFF_Manager::TagInfo & tagInfo, const bool nativeEndian,
						 SXMPMeta * xmp, const char * xmpNS, const char * xmpProp )
{
	try {	// Don't let errors with one stop the others.

		XMP_Int16 * binPtr = (XMP_Int16*)tagInfo.dataPtr;
	
		for ( size_t i = 0; i < tagInfo.count; ++i, ++binPtr ) {
	
			XMP_Int16 binValue = *binPtr;
			if ( ! nativeEndian ) Flip2 ( &binValue );
	
			char strValue[20];
			snprintf ( strValue, sizeof(strValue), "%hd", binValue );	// AUDIT: Using sizeof(strValue) is safe.
	
			xmp->AppendArrayItem ( xmpNS, xmpProp, kXMP_PropArrayIsOrdered, strValue );
	
		}

	} catch ( ... ) {
		// Do nothing, let other imports proceed.
		// ? Notify client?
	}

}	// ImportArrayTIFF_SShort

// =================================================================================================
// ImportArrayTIFF_SLong
// =====================

static void
ImportArrayTIFF_SLong ( const TIFF_Manager::TagInfo & tagInfo, const bool nativeEndian,
						SXMPMeta * xmp, const char * xmpNS, const char * xmpProp )
{
	try {	// Don't let errors with one stop the others.

		XMP_Int32 * binPtr = (XMP_Int32*)tagInfo.dataPtr;
	
		for ( size_t i = 0; i < tagInfo.count; ++i, ++binPtr ) {
	
			XMP_Int32 binValue = *binPtr;
			if ( ! nativeEndian ) Flip4 ( &binValue );
	
			char strValue[20];
			snprintf ( strValue, sizeof(strValue), "%ld", binValue );	// AUDIT: Using sizeof(strValue) is safe.
	
			xmp->AppendArrayItem ( xmpNS, xmpProp, kXMP_PropArrayIsOrdered, strValue );
	
		}

	} catch ( ... ) {
		// Do nothing, let other imports proceed.
		// ? Notify client?
	}

}	// ImportArrayTIFF_SLong

// =================================================================================================
// ImportArrayTIFF_Float
// =====================

static void
ImportArrayTIFF_Float ( const TIFF_Manager::TagInfo & tagInfo, const bool nativeEndian,
						SXMPMeta * xmp, const char * xmpNS, const char * xmpProp )
{
	try {	// Don't let errors with one stop the others.

		float * binPtr = (float*)tagInfo.dataPtr;
	
		for ( size_t i = 0; i < tagInfo.count; ++i, ++binPtr ) {
	
			float binValue = *binPtr;
			if ( ! nativeEndian ) Flip4 ( &binValue );
	
			std::string strValue;
			SXMPUtils::ConvertFromFloat ( binValue, "", &strValue );
	
			xmp->AppendArrayItem ( xmpNS, xmpProp, kXMP_PropArrayIsOrdered, strValue.c_str() );
	
		}

	} catch ( ... ) {
		// Do nothing, let other imports proceed.
		// ? Notify client?
	}

}	// ImportArrayTIFF_Float

// =================================================================================================
// ImportArrayTIFF_Double
// ======================

static void
ImportArrayTIFF_Double ( const TIFF_Manager::TagInfo & tagInfo, const bool nativeEndian,
						 SXMPMeta * xmp, const char * xmpNS, const char * xmpProp )
{
	try {	// Don't let errors with one stop the others.

		double * binPtr = (double*)tagInfo.dataPtr;
	
		for ( size_t i = 0; i < tagInfo.count; ++i, ++binPtr ) {
	
			double binValue = *binPtr;
			if ( ! nativeEndian ) Flip8 ( &binValue );
	
			std::string strValue;
			SXMPUtils::ConvertFromFloat ( binValue, "", &strValue );	// ! Yes, ConvertFromFloat.
	
			xmp->AppendArrayItem ( xmpNS, xmpProp, kXMP_PropArrayIsOrdered, strValue.c_str() );
	
		}

	} catch ( ... ) {
		// Do nothing, let other imports proceed.
		// ? Notify client?
	}

}	// ImportArrayTIFF_Double

// =================================================================================================
// ImportArrayTIFF
// ===============

static void
ImportArrayTIFF ( const TIFF_Manager::TagInfo & tagInfo, const bool nativeEndian,
				  SXMPMeta * xmp, const char * xmpNS, const char * xmpProp )
{

	// We've got a tag to map to XMP, decide how based on actual type and the expected count. Using
	// the actual type eliminates a ShortOrLong case. Using the expected count is needed to know
	// whether to create an XMP array. The actual count for an array could be 1. Put the most
	// common cases first for better iCache utilization.

	switch ( tagInfo.type ) {

		case kTIFF_ShortType :
			ImportArrayTIFF_Short ( tagInfo, nativeEndian, xmp, xmpNS, xmpProp );
			break;

		case kTIFF_LongType :
			ImportArrayTIFF_Long ( tagInfo, nativeEndian, xmp, xmpNS, xmpProp );
			break;

		case kTIFF_RationalType :
			ImportArrayTIFF_Rational ( tagInfo, nativeEndian, xmp, xmpNS, xmpProp );
			break;

		case kTIFF_SRationalType :
			ImportArrayTIFF_SRational ( tagInfo, nativeEndian, xmp, xmpNS, xmpProp );
			break;

		case kTIFF_ASCIIType :
			ImportArrayTIFF_ASCII ( tagInfo, xmp, xmpNS, xmpProp );
			break;

		case kTIFF_ByteType :
			ImportArrayTIFF_Byte ( tagInfo, xmp, xmpNS, xmpProp );
			break;

		case kTIFF_SByteType :
			ImportArrayTIFF_SByte ( tagInfo, xmp, xmpNS, xmpProp );
			break;

		case kTIFF_SShortType :
			ImportArrayTIFF_SShort ( tagInfo, nativeEndian, xmp, xmpNS, xmpProp );
			break;

		case kTIFF_SLongType :
			ImportArrayTIFF_SLong ( tagInfo, nativeEndian, xmp, xmpNS, xmpProp );
			break;

		case kTIFF_FloatType :
			ImportArrayTIFF_Float ( tagInfo, nativeEndian, xmp, xmpNS, xmpProp );
			break;

		case kTIFF_DoubleType :
			ImportArrayTIFF_Double ( tagInfo, nativeEndian, xmp, xmpNS, xmpProp );
			break;

	}

}	// ImportArrayTIFF

// =================================================================================================
// ImportTIFF_VerifyImport
// =======================
//
// Decide whether to proceed with the import based on the digest state and presence of the legacy
// and XMP. Will also delete existing XMP if appropriate.

static bool
ImportTIFF_VerifyImport ( const TIFF_Manager & tiff, SXMPMeta * xmp, int digestState,
						  XMP_Uns8 tiffIFD, XMP_Uns16 tiffID, const char * xmpNS, const char * xmpProp,
						  TIFF_Manager::TagInfo * tagInfo )
{
	bool found = false;
	
	try {	// Don't let errors with one stop the others.

		if ( digestState == kDigestDiffers ) {
			xmp->DeleteProperty ( xmpNS, xmpProp );
		} else {
			XMP_Assert ( digestState == kDigestMissing );
			if ( xmp->DoesPropertyExist ( xmpNS, xmpProp ) ) return false;
		}
	
		found = tiff.GetTag ( tiffIFD, tiffID, tagInfo );

	} catch ( ... ) {
		found = false;
	}

	return found;

}	// ImportTIFF_VerifyImport

// =================================================================================================
// ImportTIFF_CheckStandardMapping
// ===============================

static bool
ImportTIFF_CheckStandardMapping ( const TIFF_Manager::TagInfo & tagInfo,
								  const TIFF_MappingToXMP & mapInfo )
{
	XMP_Assert ( (kTIFF_ByteType <= tagInfo.type) && (tagInfo.type <= kTIFF_LastType) );
	XMP_Assert ( mapInfo.type <= kTIFF_LastType );

	if ( (tagInfo.type < kTIFF_ByteType) || (tagInfo.type > kTIFF_LastType) ) return false;

	if ( tagInfo.type != mapInfo.type ) {
		if ( mapInfo.type != kTIFF_ShortOrLongType ) return false;
		if ( (tagInfo.type != kTIFF_ShortType) && (tagInfo.type != kTIFF_LongType) ) return false;
	}

	if ( (tagInfo.count != mapInfo.count) && (mapInfo.count != kAnyCount) ) return false;

	return true;

}	// ImportTIFF_CheckStandardMapping

// =================================================================================================
// ImportTIFF_StandardMappings
// ===========================

static void
ImportTIFF_StandardMappings ( XMP_Uns8 ifd, const TIFF_Manager & tiff, SXMPMeta * xmp, int digestState )
{
	const bool nativeEndian = tiff.IsNativeEndian();
	TIFF_Manager::TagInfo tagInfo;

	const TIFF_MappingToXMP * mappings = 0;
	const char * xmpNS = 0;

	if ( ifd == kTIFF_PrimaryIFD ) {
		mappings = sPrimaryIFDMappings;
		xmpNS    = kXMP_NS_TIFF;
	} else if ( ifd == kTIFF_ExifIFD ) {
		mappings = sExifIFDMappings;
		xmpNS    = kXMP_NS_EXIF;
	} else if ( ifd == kTIFF_GPSInfoIFD ) {
		mappings = sGPSInfoIFDMappings;
		xmpNS    = kXMP_NS_EXIF;	// ! Yes, the GPS Info tags go into the exif: namespace.
	} else {
		XMP_Throw ( "Invalid IFD for standard mappings", kXMPErr_InternalFailure );
	}

	for ( size_t i = 0; mappings[i].id != 0xFFFF; ++i ) {

		try {	// Don't let errors with one stop the others.

			const TIFF_MappingToXMP & mapInfo =  mappings[i];
			const bool mapSingle = ((mapInfo.count == 1) || (mapInfo.type == kTIFF_ASCIIType));
	
			// Skip tags that have special mappings, they are handled individually later. Delete any
			// existing XMP property before going further. But after the special mapping check since we
			// don't have the XMP property name for those. This lets legacy deletions propagate and
			// eliminates any problems with existing XMP property form. Make sure the actual tag has
			// the expected type and count, ignore it (pretend it is not present) if not.
	
			if ( mapInfo.name[0] == 0 ) continue;	// Skip special mappings.
	
			bool ok = ImportTIFF_VerifyImport ( tiff, xmp, digestState, ifd, mapInfo.id, xmpNS, mapInfo.name, &tagInfo );
			if (! ok ) continue;
	
			XMP_Assert ( tagInfo.type != kTIFF_UndefinedType );	// These have a special mapping.
			if ( tagInfo.type == kTIFF_UndefinedType ) continue;
			if ( ! ImportTIFF_CheckStandardMapping ( tagInfo, mapInfo ) ) continue;
	
			if ( mapSingle ) {
				ImportSingleTIFF ( tagInfo, nativeEndian, xmp, xmpNS, mapInfo.name );
			} else {
				ImportArrayTIFF ( tagInfo, nativeEndian, xmp, xmpNS, mapInfo.name );
			}

		} catch ( ... ) {
	
			// Do nothing, let other imports proceed.
			// ? Notify client?
	
		}

	}

}	// ImportTIFF_StandardMappings

// =================================================================================================
// =================================================================================================

// =================================================================================================
// ImportTIFF_Date
// ===============
//
// Convert an Exif 2.2 master date/time tag plus associated fractional seconds to an XMP date/time.
// The Exif date/time part is a 20 byte ASCII value formatted as "YYYY:MM:DD HH:MM:SS" with a
// terminating nul. Any of the numeric portions can be blanks if unknown. The fractional seconds
// are a nul terminated ASCII string with possible space padding. They are literally the fractional
// part, the digits that would be to the right of the decimal point.

static void
ImportTIFF_Date ( const TIFF_Manager & tiff, const TIFF_Manager::TagInfo & dateInfo, XMP_Uns16 secID,
				  SXMPMeta * xmp, const char * xmpNS, const char * xmpProp )
{
	try {	// Don't let errors with one stop the others.

		const char * dateStr = (const char *) dateInfo.dataPtr;
		if ( (dateStr[4] != ':')  || (dateStr[7] != ':')  ||
			 (dateStr[10] != ' ') || (dateStr[13] != ':') || (dateStr[16] != ':') ) return;
	
		XMP_DateTime binValue;
	
		binValue.year   = GatherInt ( &dateStr[0], 4 );
		binValue.month  = GatherInt ( &dateStr[5], 2 );
		binValue.day    = GatherInt ( &dateStr[8], 2 );
		binValue.hour   = GatherInt ( &dateStr[11], 2 );
		binValue.minute = GatherInt ( &dateStr[14], 2 );
		binValue.second = GatherInt ( &dateStr[17], 2 );
		binValue.nanoSecond = 0;	// Get the fractional seconds later.
		binValue.tzSign = binValue.tzHour = binValue.tzMinute = 0;
		SXMPUtils::SetTimeZone ( &binValue );	// Assume local time.
	
		TIFF_Manager::TagInfo secInfo;
		bool found = tiff.GetTag ( kTIFF_ExifIFD, secID, &secInfo );
	
		if ( found && (secInfo.type == kTIFF_ASCIIType) ) {
			const char * fracPtr = (const char *) secInfo.dataPtr;
			binValue.nanoSecond = GatherInt ( fracPtr, secInfo.dataLen );
			size_t digits = 0;
			for ( ; (('0' <= *fracPtr) && (*fracPtr <= '9')); ++fracPtr ) ++digits;
			for ( ; digits < 9; ++digits ) binValue.nanoSecond *= 10;
		}
	
		xmp->SetProperty_Date ( xmpNS, xmpProp, binValue );

	} catch ( ... ) {
		// Do nothing, let other imports proceed.
		// ? Notify client?
	}

}	// ImportTIFF_Date

// =================================================================================================
// ImportTIFF_LocTextASCII
// =======================

static void
ImportTIFF_LocTextASCII ( const TIFF_Manager & tiff, XMP_Uns8 ifd, XMP_Uns16 tagID,
						  SXMPMeta * xmp, const char * xmpNS, const char * xmpProp )
{
	try {	// Don't let errors with one stop the others.

		TIFF_Manager::TagInfo tagInfo;
	
		bool found = tiff.GetTag ( ifd, tagID, &tagInfo );
		if ( (! found) || (tagInfo.type != kTIFF_ASCIIType) ) return;
	
		const char * chPtr  = (const char *)tagInfo.dataPtr;
		const bool   hasNul = (chPtr[tagInfo.dataLen-1] == 0);
		const bool   isUTF8 = ReconcileUtils::IsUTF8 ( chPtr, tagInfo.dataLen );
	
		if ( isUTF8 && hasNul ) {
			xmp->SetLocalizedText ( xmpNS, xmpProp, "", "x-default", chPtr );
		} else {
			std::string strValue;
			if ( isUTF8 ) {
				strValue.assign ( chPtr, tagInfo.dataLen );
			} else {
				ReconcileUtils::LocalToUTF8 ( chPtr, tagInfo.dataLen, &strValue );
			}
			xmp->SetLocalizedText ( xmpNS, xmpProp, "", "x-default", strValue.c_str() );
		}

	} catch ( ... ) {
		// Do nothing, let other imports proceed.
		// ? Notify client?
	}

}	// ImportTIFF_LocTextASCII

// =================================================================================================
// ImportTIFF_EncodedString
// ========================

static void
ImportTIFF_EncodedString ( const TIFF_Manager & tiff, const TIFF_Manager::TagInfo & tagInfo,
						   SXMPMeta * xmp, const char * xmpNS, const char * xmpProp )
{
	try {	// Don't let errors with one stop the others.

		std::string strValue;
	
		bool ok = tiff.DecodeString ( tagInfo.dataPtr, tagInfo.dataLen, &strValue );
		if ( ok ) xmp->SetProperty ( xmpNS, xmpProp, strValue.c_str() );

	} catch ( ... ) {
		// Do nothing, let other imports proceed.
		// ? Notify client?
	}
	
}	// ImportTIFF_EncodedString

// =================================================================================================
// ImportTIFF_Flash
// ================

static void
ImportTIFF_Flash ( const TIFF_Manager::TagInfo & tagInfo, bool nativeEndian,
				   SXMPMeta * xmp, const char * xmpNS, const char * xmpProp )
{
	try {	// Don't let errors with one stop the others.

		XMP_Uns16 binValue = *((XMP_Uns16*)tagInfo.dataPtr);
		if ( ! nativeEndian ) binValue = Flip2 ( binValue );
		
		bool fired    = (bool)(binValue & 1);	// Avoid implicit 0/1 conversion.
		int  rtrn     = (binValue >> 1) & 3;
		int  mode     = (binValue >> 3) & 3;
		bool function = (bool)((binValue >> 5) & 1);
		bool redEye   = (bool)((binValue >> 6) & 1);
		
		static const char * sTwoBits[] = { "0", "1", "2", "3" };
	
		xmp->SetStructField ( kXMP_NS_EXIF, "Flash", kXMP_NS_EXIF, "Fired", (fired ? kXMP_TrueStr : kXMP_FalseStr) );
		xmp->SetStructField ( kXMP_NS_EXIF, "Flash", kXMP_NS_EXIF, "Return", sTwoBits[rtrn] );
		xmp->SetStructField ( kXMP_NS_EXIF, "Flash", kXMP_NS_EXIF, "Mode", sTwoBits[mode] );
		xmp->SetStructField ( kXMP_NS_EXIF, "Flash", kXMP_NS_EXIF, "Function", (function ? kXMP_TrueStr : kXMP_FalseStr) );
		xmp->SetStructField ( kXMP_NS_EXIF, "Flash", kXMP_NS_EXIF, "RedEyeMode", (redEye ? kXMP_TrueStr : kXMP_FalseStr) );

	} catch ( ... ) {
		// Do nothing, let other imports proceed.
		// ? Notify client?
	}

}	// ImportTIFF_Flash

// =================================================================================================
// ImportTIFF_OECFTable
// ====================
// 
// Although the XMP for the OECF and SFR tables is the same, the Exif is not. The OECF table has
// signed rational values and the SFR table has unsigned.

static void
ImportTIFF_OECFTable ( const TIFF_Manager::TagInfo & tagInfo, bool nativeEndian,
					   SXMPMeta * xmp, const char * xmpNS, const char * xmpProp )
{
	try {	// Don't let errors with one stop the others.

		const XMP_Uns8 * bytePtr = (XMP_Uns8*)tagInfo.dataPtr;
		const XMP_Uns8 * byteEnd = bytePtr + tagInfo.dataLen;
		
		XMP_Uns16 columns = *((XMP_Uns16*)bytePtr);	
		XMP_Uns16 rows    = *((XMP_Uns16*)(bytePtr+2));
		if ( ! nativeEndian ) {
			columns = Flip2 ( columns );
			rows = Flip2 ( rows );
		}
		
		char buffer[40];
		
		snprintf ( buffer, sizeof(buffer), "%d", columns );	// AUDIT: Use of sizeof(buffer) is safe.
		xmp->SetStructField ( xmpNS, xmpProp, kXMP_NS_EXIF, "Columns", buffer );
		snprintf ( buffer, sizeof(buffer), "%d", rows );	// AUDIT: Use of sizeof(buffer) is safe.
		xmp->SetStructField ( xmpNS, xmpProp, kXMP_NS_EXIF, "Rows", buffer );
		
		std::string arrayPath;
		
		SXMPUtils::ComposeStructFieldPath ( xmpNS, xmpProp, kXMP_NS_EXIF, "Names", &arrayPath );
		
		bytePtr += 4;	// Move to the list of names.
		for ( size_t i = columns; i > 0; --i ) {
			size_t nameLen = strlen((XMP_StringPtr)bytePtr) + 1;	// ! Include the terminating nul.
			if ( (bytePtr + nameLen) > byteEnd ) goto BadExif;
			xmp->AppendArrayItem ( xmpNS, arrayPath.c_str(), kXMP_PropArrayIsOrdered, (XMP_StringPtr)bytePtr );
			bytePtr += nameLen;
		}
		
		if ( (byteEnd - bytePtr) != (8 * columns * rows) ) goto BadExif;	// Make sure the values are present.
		SXMPUtils::ComposeStructFieldPath ( xmpNS, xmpProp, kXMP_NS_EXIF, "Values", &arrayPath );
	
		XMP_Int32 * binPtr = (XMP_Int32*)bytePtr;
		for ( size_t i = (columns * rows); i > 0; --i, binPtr += 2 ) {
	
			XMP_Int32 binNum   = binPtr[0];
			XMP_Int32 binDenom = binPtr[1];
			if ( ! nativeEndian ) {
				Flip4 ( &binNum );
				Flip4 ( &binDenom );
			}
	
			snprintf ( buffer, sizeof(buffer), "%ld/%ld", binNum, binDenom );	// AUDIT: Use of sizeof(buffer) is safe.
	
			xmp->AppendArrayItem ( xmpNS, arrayPath.c_str(), kXMP_PropArrayIsOrdered, buffer );
	
		}
		
		return;
		
	BadExif:	// Ignore the tag if the table is ill-formed.
		xmp->DeleteProperty ( xmpNS, xmpProp );
		return;

	} catch ( ... ) {
		// Do nothing, let other imports proceed.
		// ? Notify client?
	}

}	// ImportTIFF_OECFTable

// =================================================================================================
// ImportTIFF_SFRTable
// ===================
// 
// Although the XMP for the OECF and SFR tables is the same, the Exif is not. The OECF table has
// signed rational values and the SFR table has unsigned.

static void
ImportTIFF_SFRTable ( const TIFF_Manager::TagInfo & tagInfo, bool nativeEndian,
					  SXMPMeta * xmp, const char * xmpNS, const char * xmpProp )
{
	try {	// Don't let errors with one stop the others.

		const XMP_Uns8 * bytePtr = (XMP_Uns8*)tagInfo.dataPtr;
		const XMP_Uns8 * byteEnd = bytePtr + tagInfo.dataLen;
		
		XMP_Uns16 columns = *((XMP_Uns16*)bytePtr);	
		XMP_Uns16 rows    = *((XMP_Uns16*)(bytePtr+2));
		if ( ! nativeEndian ) {
			columns = Flip2 ( columns );
			rows = Flip2 ( rows );
		}
		
		char buffer[40];
		
		snprintf ( buffer, sizeof(buffer), "%d", columns );	// AUDIT: Use of sizeof(buffer) is safe.
		xmp->SetStructField ( xmpNS, xmpProp, kXMP_NS_EXIF, "Columns", buffer );
		snprintf ( buffer, sizeof(buffer), "%d", rows );	// AUDIT: Use of sizeof(buffer) is safe.
		xmp->SetStructField ( xmpNS, xmpProp, kXMP_NS_EXIF, "Rows", buffer );
		
		std::string arrayPath;
		
		SXMPUtils::ComposeStructFieldPath ( xmpNS, xmpProp, kXMP_NS_EXIF, "Names", &arrayPath );
		
		bytePtr += 4;	// Move to the list of names.
		for ( size_t i = columns; i > 0; --i ) {
			size_t nameLen = strlen((XMP_StringPtr)bytePtr) + 1;	// ! Include the terminating nul.
			if ( (bytePtr + nameLen) > byteEnd ) goto BadExif;
			xmp->AppendArrayItem ( xmpNS, arrayPath.c_str(), kXMP_PropArrayIsOrdered, (XMP_StringPtr)bytePtr );
			bytePtr += nameLen;
		}
		
		if ( (byteEnd - bytePtr) != (8 * columns * rows) ) goto BadExif;	// Make sure the values are present.
		SXMPUtils::ComposeStructFieldPath ( xmpNS, xmpProp, kXMP_NS_EXIF, "Values", &arrayPath );
	
		XMP_Uns32 * binPtr = (XMP_Uns32*)bytePtr;
		for ( size_t i = (columns * rows); i > 0; --i, binPtr += 2 ) {
	
			XMP_Uns32 binNum   = binPtr[0];
			XMP_Uns32 binDenom = binPtr[1];
			if ( ! nativeEndian ) {
				binNum   = Flip4 ( binNum );
				binDenom = Flip4 ( binDenom );
			}
	
			snprintf ( buffer, sizeof(buffer), "%lu/%lu", binNum, binDenom );	// AUDIT: Use of sizeof(buffer) is safe.
	
			xmp->AppendArrayItem ( xmpNS, arrayPath.c_str(), kXMP_PropArrayIsOrdered, buffer );
	
		}
		
		return;
		
	BadExif:	// Ignore the tag if the table is ill-formed.
		xmp->DeleteProperty ( xmpNS, xmpProp );
		return;

	} catch ( ... ) {
		// Do nothing, let other imports proceed.
		// ? Notify client?
	}

}	// ImportTIFF_SFRTable

// =================================================================================================
// ImportTIFF_CFATable
// ===================

static void
ImportTIFF_CFATable ( const TIFF_Manager::TagInfo & tagInfo, bool nativeEndian,
					  SXMPMeta * xmp, const char * xmpNS, const char * xmpProp )
{
	try {	// Don't let errors with one stop the others.

		const XMP_Uns8 * bytePtr = (XMP_Uns8*)tagInfo.dataPtr;
		const XMP_Uns8 * byteEnd = bytePtr + tagInfo.dataLen;
		
		XMP_Uns16 columns = *((XMP_Uns16*)bytePtr);	
		XMP_Uns16 rows    = *((XMP_Uns16*)(bytePtr+2));
		if ( ! nativeEndian ) {
			columns = Flip2 ( columns );
			rows = Flip2 ( rows );
		}
		
		char buffer[20];
		std::string arrayPath;
		
		snprintf ( buffer, sizeof(buffer), "%d", columns );	// AUDIT: Use of sizeof(buffer) is safe.
		xmp->SetStructField ( xmpNS, xmpProp, kXMP_NS_EXIF, "Columns", buffer );
		snprintf ( buffer, sizeof(buffer), "%d", rows );	// AUDIT: Use of sizeof(buffer) is safe.
		xmp->SetStructField ( xmpNS, xmpProp, kXMP_NS_EXIF, "Rows", buffer );
		
		bytePtr += 4;	// Move to the matrix of values.
		if ( (byteEnd - bytePtr) != (columns * rows) ) goto BadExif;	// Make sure the values are present.
		
		SXMPUtils::ComposeStructFieldPath ( xmpNS, xmpProp, kXMP_NS_EXIF, "Values", &arrayPath );
	
		for ( size_t i = (columns * rows); i > 0; --i, ++bytePtr ) {
			snprintf ( buffer, sizeof(buffer), "%hu", *bytePtr );	// AUDIT: Use of sizeof(buffer) is safe.
			xmp->AppendArrayItem ( xmpNS, arrayPath.c_str(), kXMP_PropArrayIsOrdered, buffer );
		}
		
		return;
		
	BadExif:	// Ignore the tag if the table is ill-formed.
		xmp->DeleteProperty ( xmpNS, xmpProp );
		return;

	} catch ( ... ) {
		// Do nothing, let other imports proceed.
		// ? Notify client?
	}

}	// ImportTIFF_CFATable

// =================================================================================================
// ImportTIFF_DSDTable
// ===================

static void
ImportTIFF_DSDTable ( const TIFF_Manager & tiff, const TIFF_Manager::TagInfo & tagInfo,
					  SXMPMeta * xmp, const char * xmpNS, const char * xmpProp )
{
	try {	// Don't let errors with one stop the others.

		const XMP_Uns8 * bytePtr = (XMP_Uns8*)tagInfo.dataPtr;
		const XMP_Uns8 * byteEnd = bytePtr + tagInfo.dataLen;
		
		XMP_Uns16 columns = *((XMP_Uns16*)bytePtr);	
		XMP_Uns16 rows    = *((XMP_Uns16*)(bytePtr+2));
		if ( ! tiff.IsNativeEndian() ) {
			columns = Flip2 ( columns );
			rows = Flip2 ( rows );
		}
		
		char buffer[20];
		
		snprintf ( buffer, sizeof(buffer), "%d", columns );	// AUDIT: Use of sizeof(buffer) is safe.
		xmp->SetStructField ( xmpNS, xmpProp, kXMP_NS_EXIF, "Columns", buffer );
		snprintf ( buffer, sizeof(buffer), "%d", rows );	// AUDIT: Use of sizeof(buffer) is safe.
		xmp->SetStructField ( xmpNS, xmpProp, kXMP_NS_EXIF, "Rows", buffer );
		
		std::string arrayPath;
		SXMPUtils::ComposeStructFieldPath ( xmpNS, xmpProp, kXMP_NS_EXIF, "Settings", &arrayPath );
		
		bytePtr += 4;	// Move to the list of settings.
		UTF16Unit * utf16Ptr = (UTF16Unit*)bytePtr;
		UTF16Unit * utf16End = (UTF16Unit*)byteEnd;
		
		std::string utf8;
	
		// Figure 17 in the Exif 2.2 spec is unclear. It has counts for rows and columns, but the
		// settings are listed as 1..n, not as a rectangular matrix. So, ignore the counts and copy
		// strings until the end of the Exif value.
		
		while ( utf16Ptr < utf16End ) {
	
			size_t nameLen = 0;
			while ( utf16Ptr[nameLen] != 0 ) ++nameLen;
			++nameLen;	// ! Include the terminating nul.
			if ( (utf16Ptr + nameLen) > utf16End ) goto BadExif;
	
			try {
				FromUTF16 ( utf16Ptr, nameLen, &utf8, tiff.IsBigEndian() );
			} catch ( ... ) {
				goto BadExif; // Ignore the tag if there are conversion errors.
			}
	
			xmp->AppendArrayItem ( xmpNS, arrayPath.c_str(), kXMP_PropArrayIsOrdered, utf8.c_str() );
	
			utf16Ptr += nameLen;
	
		}
		
		return;
		
	BadExif:	// Ignore the tag if the table is ill-formed.
		xmp->DeleteProperty ( xmpNS, xmpProp );
		return;

	} catch ( ... ) {
		// Do nothing, let other imports proceed.
		// ? Notify client?
	}

}	// ImportTIFF_DSDTable

// =================================================================================================
// ImportTIFF_GPSCoordinate
// ========================

static void
ImportTIFF_GPSCoordinate ( const TIFF_Manager & tiff, const TIFF_Manager::TagInfo & posInfo,
						   SXMPMeta * xmp, const char * xmpNS, const char * xmpProp )
{
	try {	// Don't let errors with one stop the others.

		const bool nativeEndian = tiff.IsNativeEndian();
		
		XMP_Uns16 refID = posInfo.id - 1;	// ! The GPS refs and coordinates are all tag n and n+1.
		TIFF_Manager::TagInfo refInfo;
		bool found = tiff.GetTag ( kTIFF_GPSInfoIFD, refID, &refInfo );
		if ( (! found) || (refInfo.type != kTIFF_ASCIIType) || (refInfo.count != 2) ) return;
		char ref = *((char*)refInfo.dataPtr);
		
		XMP_Uns32 * binPtr = (XMP_Uns32*)posInfo.dataPtr;
		XMP_Uns32 degNum   = binPtr[0];
		XMP_Uns32 degDenom = binPtr[1];
		XMP_Uns32 minNum   = binPtr[2];
		XMP_Uns32 minDenom = binPtr[3];
		XMP_Uns32 secNum   = binPtr[4];
		XMP_Uns32 secDenom = binPtr[5];
		if ( ! nativeEndian ) {
			degNum = Flip4 ( degNum );
			degDenom = Flip4 ( degDenom );
			minNum = Flip4 ( minNum );
			minDenom = Flip4 ( minDenom );
			secNum = Flip4 ( secNum );
			secDenom = Flip4 ( secDenom );
		}
		
		char buffer[40];
		
		if ( (degDenom == 1) && (minDenom == 1) && (secDenom == 1) ) {
		
			snprintf ( buffer, sizeof(buffer), "%lu,%lu,%lu%c", degNum, minNum, secNum, ref );	// AUDIT: Using sizeof(buffer is safe.
		
		} else {
		
			XMP_Uns32 maxDenom = degDenom;
			if ( minDenom > degDenom ) maxDenom = minDenom;
			if ( secDenom > degDenom ) maxDenom = secDenom;
			
			int fracDigits = 1;
			while ( maxDenom > 10 ) { ++fracDigits; maxDenom = maxDenom/10; }
			
			double temp    = (double)degNum / (double)degDenom;
			double degrees = (double)((XMP_Uns32)temp);	// Just the integral number of degrees.
			double minutes = ((temp - degrees) * 60.0) +
							 ((double)minNum / (double)minDenom) +
							 (((double)secNum / (double)secDenom) / 60.0);
		
			snprintf ( buffer, sizeof(buffer), "%.0f,%.*f%c", degrees, fracDigits, minutes, ref );	// AUDIT: Using sizeof(buffer is safe.
		
		}
		
		xmp->SetProperty ( xmpNS, xmpProp, buffer );

	} catch ( ... ) {
		// Do nothing, let other imports proceed.
		// ? Notify client?
	}

}	// ImportTIFF_GPSCoordinate

// =================================================================================================
// ImportTIFF_GPSTimeStamp
// =======================

static void
ImportTIFF_GPSTimeStamp ( const TIFF_Manager & tiff, const TIFF_Manager::TagInfo & timeInfo,
						  SXMPMeta * xmp, const char * xmpNS, const char * xmpProp )
{
	try {	// Don't let errors with one stop the others.

		const bool nativeEndian = tiff.IsNativeEndian();
	
		bool haveDate;
		TIFF_Manager::TagInfo dateInfo;
		haveDate = tiff.GetTag ( kTIFF_GPSInfoIFD, kTIFF_GPSDateStamp, &dateInfo );
		if ( ! haveDate ) haveDate = tiff.GetTag ( kTIFF_ExifIFD, kTIFF_DateTimeOriginal, &dateInfo );
		if ( ! haveDate ) haveDate = tiff.GetTag ( kTIFF_ExifIFD, kTIFF_DateTimeDigitized, &dateInfo );
		if ( ! haveDate ) return;
	
		const char * dateStr = (const char *) dateInfo.dataPtr;
		if ( (dateStr[4] != ':') || (dateStr[7] != ':') ) return;
		if ( (dateStr[10] != 0)  && (dateStr[10] != ' ') ) return;
		
		XMP_Uns32 * binPtr = (XMP_Uns32*)timeInfo.dataPtr;
		XMP_Uns32 hourNum   = binPtr[0];
		XMP_Uns32 hourDenom = binPtr[1];
		XMP_Uns32 minNum    = binPtr[2];
		XMP_Uns32 minDenom  = binPtr[3];
		XMP_Uns32 secNum    = binPtr[4];
		XMP_Uns32 secDenom  = binPtr[5];
		if ( ! nativeEndian ) {
			hourNum = Flip4 ( hourNum );
			hourDenom = Flip4 ( hourDenom );
			minNum = Flip4 ( minNum );
			minDenom = Flip4 ( minDenom );
			secNum = Flip4 ( secNum );
			secDenom = Flip4 ( secDenom );
		}
	
		double fHour, fMin, fSec, fNano, temp;	
		fSec  =  (double)secNum / (double)secDenom;
		temp  =  (double)minNum / (double)minDenom;
		fMin  =  (double)((XMP_Uns32)temp);
		fSec  += (temp - fMin) * 60.0;
		temp  =  (double)hourNum / (double)hourDenom;
		fHour =  (double)((XMP_Uns32)temp);
		fSec  += (temp - fHour) * 3600.0;
		temp  =  (double)((XMP_Uns32)fSec);
		fNano =  (fSec - temp) * (1000.0*1000.0*1000.0);
		fSec  =  temp;
		
		XMP_DateTime binStamp;
		binStamp.tzSign = kXMP_TimeIsUTC;
		binStamp.tzHour = binStamp.tzMinute = 0;
		binStamp.year   = GatherInt ( dateStr, 4 );
		binStamp.month  = GatherInt ( dateStr+5, 2 );
		binStamp.day    = GatherInt ( dateStr+8, 2 );
		binStamp.hour   = (XMP_Int32)fHour;
		binStamp.minute = (XMP_Int32)fMin;
		binStamp.second = (XMP_Int32)fSec;
		binStamp.nanoSecond = (XMP_Int32)fNano;
		
		xmp->SetProperty_Date ( xmpNS, xmpProp, binStamp );

	} catch ( ... ) {
		// Do nothing, let other imports proceed.
		// ? Notify client?
	}

}	// ImportTIFF_GPSTimeStamp

// =================================================================================================
// =================================================================================================

// =================================================================================================
// ReconcileUtils::ImportTIFF
// ==========================

void
ReconcileUtils::ImportTIFF ( const TIFF_Manager & tiff, SXMPMeta * xmp, int digestState, XMP_FileFormat srcFormat )
{
	TIFF_Manager::TagInfo tagInfo;
	bool ok;

	ImportTIFF_StandardMappings ( kTIFF_PrimaryIFD, tiff, xmp, digestState );

	// 306 DateTime is a date master with 37520 SubSecTime and is mapped to xmp:ModifyDate.
	ok = ImportTIFF_VerifyImport ( tiff, xmp, digestState, kTIFF_PrimaryIFD, kTIFF_DateTime,
								   kXMP_NS_XMP, "ModifyDate", &tagInfo );
	if ( ok && (tagInfo.type == kTIFF_ASCIIType) && (tagInfo.count == 20) ) {
		ImportTIFF_Date ( tiff, tagInfo, kTIFF_SubSecTime, xmp, kXMP_NS_XMP, "ModifyDate" );
	}

	if ( srcFormat != kXMP_PhotoshopFile ) {
	
		// ! TIFF tags 270, 315, and 33432 are ignored for Photoshop files.
		
		XMP_Assert ( (srcFormat == kXMP_JPEGFile) || (srcFormat == kXMP_TIFFFile) );
	
		// 270 ImageDescription is an ASCII tag and is mapped to dc:description["x-default"].
		// Call ImportTIFF_VerifyImport using the x-default item path, don't delete the whole array.
		ok = ImportTIFF_VerifyImport ( tiff, xmp, digestState, kTIFF_PrimaryIFD, kTIFF_ImageDescription,
									   kXMP_NS_DC, "description[?xml:lang='x-default']", &tagInfo );
		if ( ok ) ImportTIFF_LocTextASCII ( tiff, kTIFF_PrimaryIFD, kTIFF_ImageDescription,
											xmp, kXMP_NS_DC, "description" );

		// 315 Artist is an ASCII tag and is mapped to dc:creator[*].
		ok = ImportTIFF_VerifyImport ( tiff, xmp, digestState, kTIFF_PrimaryIFD, kTIFF_Artist,
									   kXMP_NS_DC, "creator", &tagInfo );
		if ( ok && (tagInfo.type == kTIFF_ASCIIType) ) {
			ImportArrayTIFF_ASCII ( tagInfo, xmp, kXMP_NS_DC, "creator" );
		}

		// 33432 Copyright is mapped to dc:rights["x-default"].
		// Call ImportTIFF_VerifyImport using the x-default item path, don't delete the whole array.
		ok = ImportTIFF_VerifyImport ( tiff, xmp, digestState, kTIFF_PrimaryIFD, kTIFF_Copyright,
									   kXMP_NS_DC, "rights[?xml:lang='x-default']", &tagInfo );
		if ( ok ) ImportTIFF_LocTextASCII ( tiff, kTIFF_PrimaryIFD, kTIFF_Copyright, xmp, kXMP_NS_DC, "rights" );
	
	}

}	// ReconcileUtils::ImportTIFF;

// =================================================================================================
// ReconcileUtils::ImportExif
// ==========================

void
ReconcileUtils::ImportExif ( const TIFF_Manager & tiff, SXMPMeta * xmp, int digestState )
{
	const bool nativeEndian = tiff.IsNativeEndian();

	TIFF_Manager::TagInfo tagInfo;
	bool ok;

	ImportTIFF_StandardMappings ( kTIFF_ExifIFD, tiff, xmp, digestState );
	ImportTIFF_StandardMappings ( kTIFF_GPSInfoIFD, tiff, xmp, digestState );

	// ------------------------------------------------------
	// Here are the Exif IFD tags that have special mappings:

	// 36864 ExifVersion is 4 "undefined" ASCII characters.
	ok = ImportTIFF_VerifyImport ( tiff, xmp, digestState, kTIFF_ExifIFD, kTIFF_ExifVersion,
								   kXMP_NS_EXIF, "ExifVersion", &tagInfo );
	if ( ok && (tagInfo.type == kTIFF_UndefinedType) && (tagInfo.count == 4) ) {
		char str[5];
		*((XMP_Uns32*)str) = *((XMP_Uns32*)tagInfo.dataPtr);
		str[4] = 0;
		xmp->SetProperty ( kXMP_NS_EXIF, "ExifVersion", str );
	}

	// 40960 FlashpixVersion is 4 "undefined" ASCII characters.
	ok = ImportTIFF_VerifyImport ( tiff, xmp, digestState, kTIFF_ExifIFD, kTIFF_FlashpixVersion,
								   kXMP_NS_EXIF, "FlashpixVersion", &tagInfo );
	if ( ok && (tagInfo.type == kTIFF_UndefinedType) && (tagInfo.count == 4) ) {
		char str[5];
		*((XMP_Uns32*)str) = *((XMP_Uns32*)tagInfo.dataPtr);
		str[4] = 0;
		xmp->SetProperty ( kXMP_NS_EXIF, "FlashpixVersion", str );
	}

	// 37121 ComponentsConfiguration is an array of 4 "undefined" UInt8 bytes.
	ok = ImportTIFF_VerifyImport ( tiff, xmp, digestState, kTIFF_ExifIFD, kTIFF_ComponentsConfiguration,
								   kXMP_NS_EXIF, "ComponentsConfiguration", &tagInfo );
	if ( ok && (tagInfo.type == kTIFF_UndefinedType) && (tagInfo.count == 4) ) {
		ImportArrayTIFF_Byte ( tagInfo, xmp, kXMP_NS_EXIF, "ComponentsConfiguration" );
	}

	// 37510 UserComment is a string with explicit encoding.
	ok = ImportTIFF_VerifyImport ( tiff, xmp, digestState, kTIFF_ExifIFD, kTIFF_UserComment,
								   kXMP_NS_EXIF, "UserComment", &tagInfo );
	if ( ok ) {
		ImportTIFF_EncodedString ( tiff, tagInfo, xmp, kXMP_NS_EXIF, "UserComment" );
	}

	// 36867 DateTimeOriginal is a date master with 37521 SubSecTimeOriginal.
	ok = ImportTIFF_VerifyImport ( tiff, xmp, digestState, kTIFF_ExifIFD, kTIFF_DateTimeOriginal,
								   kXMP_NS_EXIF, "DateTimeOriginal", &tagInfo );
	if ( ok && (tagInfo.type == kTIFF_ASCIIType) && (tagInfo.count == 20) ) {
		ImportTIFF_Date ( tiff, tagInfo, kTIFF_SubSecTimeOriginal, xmp, kXMP_NS_EXIF, "DateTimeOriginal" );
	}

	// 36868 DateTimeDigitized is a date master with 37522 SubSecTimeDigitized.
	ok = ImportTIFF_VerifyImport ( tiff, xmp, digestState, kTIFF_ExifIFD, kTIFF_DateTimeDigitized,
								   kXMP_NS_EXIF, "DateTimeDigitized", &tagInfo );
	if ( ok && (tagInfo.type == kTIFF_ASCIIType) && (tagInfo.count == 20) ) {
		ImportTIFF_Date ( tiff, tagInfo, kTIFF_SubSecTimeDigitized, xmp, kXMP_NS_EXIF, "DateTimeDigitized" );
	}

	// 34856 OECF is an OECF/SFR table.
	ok = ImportTIFF_VerifyImport ( tiff, xmp, digestState, kTIFF_ExifIFD, kTIFF_OECF,
								   kXMP_NS_EXIF, "OECF", &tagInfo );
	if ( ok ) {
		ImportTIFF_OECFTable ( tagInfo, nativeEndian, xmp, kXMP_NS_EXIF, "OECF" );
	}

	// 37385 Flash is a UInt16 collection of bit fields and is mapped to a struct in XMP.
	ok = ImportTIFF_VerifyImport ( tiff, xmp, digestState, kTIFF_ExifIFD, kTIFF_Flash,
								   kXMP_NS_EXIF, "Flash", &tagInfo );
	if ( ok && (tagInfo.type == kTIFF_ShortType) && (tagInfo.count == 1) ) {
		ImportTIFF_Flash ( tagInfo, nativeEndian, xmp, kXMP_NS_EXIF, "Flash" );
	}

	// 41484 SpatialFrequencyResponse is an OECF/SFR table.
	ok = ImportTIFF_VerifyImport ( tiff, xmp, digestState, kTIFF_ExifIFD, kTIFF_SpatialFrequencyResponse,
								   kXMP_NS_EXIF, "SpatialFrequencyResponse", &tagInfo );
	if ( ok ) {
		ImportTIFF_SFRTable ( tagInfo, nativeEndian, xmp, kXMP_NS_EXIF, "SpatialFrequencyResponse" );
	}

	// 41728 FileSource is an "undefined" UInt8.
	ok = ImportTIFF_VerifyImport ( tiff, xmp, digestState, kTIFF_ExifIFD, kTIFF_FileSource,
								   kXMP_NS_EXIF, "FileSource", &tagInfo );
	if ( ok && (tagInfo.type == kTIFF_UndefinedType) && (tagInfo.count == 1) ) {
		ImportSingleTIFF_Byte ( tagInfo, xmp, kXMP_NS_EXIF, "FileSource" );
	}

	// 41729 SceneType is an "undefined" UInt8.
	ok = ImportTIFF_VerifyImport ( tiff, xmp, digestState, kTIFF_ExifIFD, kTIFF_SceneType,
								   kXMP_NS_EXIF, "SceneType", &tagInfo );
	if ( ok && (tagInfo.type == kTIFF_UndefinedType) && (tagInfo.count == 1) ) {
		ImportSingleTIFF_Byte ( tagInfo, xmp, kXMP_NS_EXIF, "SceneType" );
	}

	// 41730 CFAPattern is a custom table.
	ok = ImportTIFF_VerifyImport ( tiff, xmp, digestState, kTIFF_ExifIFD, kTIFF_CFAPattern,
								   kXMP_NS_EXIF, "CFAPattern", &tagInfo );
	if ( ok ) {
		ImportTIFF_CFATable ( tagInfo, nativeEndian, xmp, kXMP_NS_EXIF, "CFAPattern" );
	}

	// 41995 DeviceSettingDescription is a custom table.
	ok = ImportTIFF_VerifyImport ( tiff, xmp, digestState, kTIFF_ExifIFD, kTIFF_DeviceSettingDescription,
								   kXMP_NS_EXIF, "DeviceSettingDescription", &tagInfo );
	if ( ok ) {
		ImportTIFF_DSDTable ( tiff, tagInfo, xmp, kXMP_NS_EXIF, "DeviceSettingDescription" );
	}

	// ----------------------------------------------------------
	// Here are the GPS Info IFD tags that have special mappings:

	// 0 GPSVersionID is 4 UInt8 bytes and mapped as "n.n.n.n".
	ok = ImportTIFF_VerifyImport ( tiff, xmp, digestState, kTIFF_GPSInfoIFD, kTIFF_GPSVersionID,
								   kXMP_NS_EXIF, "GPSVersionID", &tagInfo );
	if ( ok && (tagInfo.type == kTIFF_ByteType) && (tagInfo.count == 4) ) {
		const char * strIn = (const char *) tagInfo.dataPtr;
		char strOut[8];
		strOut[0] = strIn[0];
		strOut[2] = strIn[1];
		strOut[4] = strIn[2];
		strOut[6] = strIn[3];
		strOut[1] = strOut[3] = strOut[5] = '.';
		strOut[7] = 0;
		xmp->SetProperty ( kXMP_NS_EXIF, "GPSVersionID", strOut );
	}

	// 2 GPSLatitude is a GPS coordinate master.
	ok = ImportTIFF_VerifyImport ( tiff, xmp, digestState, kTIFF_GPSInfoIFD, kTIFF_GPSLatitude,
								   kXMP_NS_EXIF, "GPSLatitude", &tagInfo );
	if ( ok ) {
		ImportTIFF_GPSCoordinate ( tiff, tagInfo, xmp, kXMP_NS_EXIF, "GPSLatitude" );
	}

	// 4 GPSLongitude is a GPS coordinate master.
	ok = ImportTIFF_VerifyImport ( tiff, xmp, digestState, kTIFF_GPSInfoIFD, kTIFF_GPSLongitude,
								   kXMP_NS_EXIF, "GPSLongitude", &tagInfo );
	if ( ok ) {
		ImportTIFF_GPSCoordinate ( tiff, tagInfo, xmp, kXMP_NS_EXIF, "GPSLongitude" );
	}

	// 7 GPSTimeStamp is a UTC time as 3 rationals, mated with the optional GPSDateStamp.
	ok = ImportTIFF_VerifyImport ( tiff, xmp, digestState, kTIFF_GPSInfoIFD, kTIFF_GPSTimeStamp,
								   kXMP_NS_EXIF, "GPSTimeStamp", &tagInfo );
	if ( ok && (tagInfo.type == kTIFF_RationalType) && (tagInfo.count == 3) ) {
		ImportTIFF_GPSTimeStamp ( tiff, tagInfo, xmp, kXMP_NS_EXIF, "GPSTimeStamp" );
	}

	// 20 GPSDestLatitude is a GPS coordinate master.
	ok = ImportTIFF_VerifyImport ( tiff, xmp, digestState, kTIFF_GPSInfoIFD, kTIFF_GPSDestLatitude,
								   kXMP_NS_EXIF, "GPSDestLatitude", &tagInfo );
	if ( ok ) {
		ImportTIFF_GPSCoordinate ( tiff, tagInfo, xmp, kXMP_NS_EXIF, "GPSDestLatitude" );
	}

	// 22 GPSDestLongitude is a GPS coordinate master.
	ok = ImportTIFF_VerifyImport ( tiff, xmp, digestState, kTIFF_GPSInfoIFD, kTIFF_GPSDestLongitude,
								   kXMP_NS_EXIF, "GPSDestLongitude", &tagInfo );
	if ( ok ) {
		ImportTIFF_GPSCoordinate ( tiff, tagInfo, xmp, kXMP_NS_EXIF, "GPSDestLongitude" );
	}

	// 27 GPSProcessingMethod is a string with explicit encoding.
	ok = ImportTIFF_VerifyImport ( tiff, xmp, digestState, kTIFF_GPSInfoIFD, kTIFF_GPSProcessingMethod,
								   kXMP_NS_EXIF, "GPSProcessingMethod", &tagInfo );
	if ( ok ) {
		ImportTIFF_EncodedString ( tiff, tagInfo, xmp, kXMP_NS_EXIF, "GPSProcessingMethod" );
	}

	// 28 GPSAreaInformation is a string with explicit encoding.
	ok = ImportTIFF_VerifyImport ( tiff, xmp, digestState, kTIFF_GPSInfoIFD, kTIFF_GPSAreaInformation,
								   kXMP_NS_EXIF, "GPSAreaInformation", &tagInfo );
	if ( ok ) {
		ImportTIFF_EncodedString ( tiff, tagInfo, xmp, kXMP_NS_EXIF, "GPSAreaInformation" );
	}

}	// ReconcileUtils::ImportExif;

// =================================================================================================
// =================================================================================================

// =================================================================================================
// ExportSingleTIFF_Short
// ======================

static void
ExportSingleTIFF_Short ( const SXMPMeta & xmp, const char * xmpNS, const char * xmpProp,
						 TIFF_Manager * tiff, XMP_Uns8 ifd, XMP_Uns16 id )
{
	try {	// Don't let errors with one stop the others.

		long xmpValue;

		bool foundXMP = xmp.GetProperty_Int ( xmpNS, xmpProp, &xmpValue, 0 );
		if ( ! foundXMP ) {
			tiff->DeleteTag ( ifd, id );
			return;
		}

		if ( (xmpValue < 0) || (xmpValue > 0xFFFF) ) return;	// ? Complain? Peg to limit? Delete the tag?

		tiff->SetTag_Short ( ifd, id, (XMP_Uns16)xmpValue );

	} catch ( ... ) {
		// Do nothing, let other exports proceed.
		// ? Notify client?
	}
	
}	// ExportSingleTIFF_Short

// =================================================================================================
// ExportSingleTIFF_Rational
// =========================
//
// An XMP (unsigned) rational is supposed to be written as a string "num/denom".

static void
ExportSingleTIFF_Rational ( const SXMPMeta & xmp, const char * xmpNS, const char * xmpProp,
							TIFF_Manager * tiff, XMP_Uns8 ifd, XMP_Uns16 id )
{
	try {	// Don't let errors with one stop the others.

		std::string    strValue;
		XMP_OptionBits xmpFlags;

		bool foundXMP = xmp.GetProperty ( xmpNS, xmpProp, &strValue, &xmpFlags );
		if ( ! foundXMP ) {
			tiff->DeleteTag ( ifd, id );
			return;
		}
		
		if ( ! XMP_PropIsSimple ( xmpFlags ) ) return;	// ? Complain? Delete the tag?
		
		XMP_Uns32 newNum, newDenom;
		const char* partPtr;
		size_t partLen;
		
		partPtr = strValue.c_str();
		for ( partLen = 0; partPtr[partLen] != 0; ++partLen ) {
			if ( (partPtr[partLen] < '0') || (partPtr[partLen] > '9') ) break;
		}
		if ( partLen == 0 ) return;	// ? Complain? Delete the tag?
		newNum = GatherInt ( partPtr, partLen );
		
		if ( partPtr[partLen] == 0 ) {
			newDenom = 1;	// Tolerate bad XMP that just has the numerator.
		} else if ( partPtr[partLen] != '/' ) {
			return;	// ? Complain? Delete the tag?
		} else {
			partPtr += partLen+1;
			for ( partLen = 0; partPtr[partLen] != 0; ++partLen ) {
				if ( (partPtr[partLen] < '0') || (partPtr[partLen] > '9') ) break;
			}
			if ( (partLen == 0) || (partPtr[partLen] != 0) ) return;	// ? Complain? Delete the tag?
			newDenom = GatherInt ( partPtr, partLen );
		}
			
		tiff->SetTag_Rational ( ifd, id, newNum, newDenom );

	} catch ( ... ) {
		// Do nothing, let other exports proceed.
		// ? Notify client?
	}
	
}	// ExportSingleTIFF_Rational

// =================================================================================================
// ExportSingleTIFF_ASCII
// ======================

static void
ExportSingleTIFF_ASCII ( const SXMPMeta & xmp, const char * xmpNS, const char * xmpProp,
						 TIFF_Manager * tiff, XMP_Uns8 ifd, XMP_Uns16 id )
{
	try {	// Don't let errors with one stop the others.

		std::string    xmpValue;
		XMP_OptionBits xmpFlags;

		bool foundXMP = xmp.GetProperty ( xmpNS, xmpProp, &xmpValue, &xmpFlags );
		if ( ! foundXMP ) {
			tiff->DeleteTag ( ifd, id );
			return;
		}

		if ( ! XMP_PropIsSimple ( xmpFlags ) ) return;	// ? Complain? Delete the tag?

		tiff->SetTag ( ifd, id, kTIFF_ASCIIType, xmpValue.size()+1, xmpValue.c_str() );
		
	} catch ( ... ) {
		// Do nothing, let other exports proceed.
		// ? Notify client?
	}
	
}	// ExportSingleTIFF_ASCII

// =================================================================================================
// ExportArrayTIFF_ASCII
// =====================
//
// Catenate all of the XMP array values into a string with separating nul characters.

static void
ExportArrayTIFF_ASCII ( const SXMPMeta & xmp, const char * xmpNS, const char * xmpProp,
						TIFF_Manager * tiff, XMP_Uns8 ifd, XMP_Uns16 id )
{
	try {	// Don't let errors with one stop the others.

		std::string    itemValue, fullValue;
		XMP_OptionBits xmpFlags;

		bool foundXMP = xmp.GetProperty ( xmpNS, xmpProp, 0, &xmpFlags );
		if ( ! foundXMP ) {
			tiff->DeleteTag ( ifd, id );
			return;
		}

		if ( ! XMP_PropIsArray ( xmpFlags ) ) return;	// ? Complain? Delete the tag?
		
		size_t count = xmp.CountArrayItems ( xmpNS, xmpProp );
		for ( size_t i = 1; i <= count; ++i ) {	// ! XMP arrays are indexed from 1.
			(void) xmp.GetArrayItem ( xmpNS, xmpProp, i, &itemValue, &xmpFlags );
			if ( ! XMP_PropIsSimple ( xmpFlags ) ) continue;	// ? Complain?
			fullValue.append ( itemValue );
			fullValue.append ( 1, '\x0' );
		}
		
		tiff->SetTag ( ifd, id, kTIFF_ASCIIType, fullValue.size(), fullValue.c_str() );	// ! Already have trailing nul.

	} catch ( ... ) {
		// Do nothing, let other exports proceed.
		// ? Notify client?
	}
	
}	// ExportArrayTIFF_ASCII

// =================================================================================================
// ExportTIFF_Date
// ===============
//
// Convert  an XMP date/time to an Exif 2.2 master date/time tag plus associated fractional seconds.
// The Exif date/time part is a 20 byte ASCII value formatted as "YYYY:MM:DD HH:MM:SS" with a
// terminating nul. The fractional seconds are a nul terminated ASCII string with possible space
// padding. They are literally the fractional part, the digits that would be to the right of the
// decimal point.

static void
ExportTIFF_Date ( const SXMPMeta & xmp, const char * xmpNS, const char * xmpProp,
				  TIFF_Manager * tiff, XMP_Uns8 mainIFD, XMP_Uns16 mainID, XMP_Uns8 fracIFD, XMP_Uns16 fracID )
{
	try {	// Don't let errors with one stop the others.

		XMP_DateTime xmpValue;

		bool foundXMP = xmp.GetProperty_Date ( xmpNS, xmpProp, &xmpValue, 0 );
		if ( ! foundXMP ) {
			tiff->DeleteTag ( mainIFD, mainID );
			tiff->DeleteTag ( fracIFD, fracID );
			return;
		}
		
		char buffer[24];
		snprintf ( buffer, sizeof(buffer), "%.4d:%.2d:%.2d %.2d:%.2d:%.2d",	// AUDIT: Use of sizeof(buffer) is safe.
				   xmpValue.year, xmpValue.month, xmpValue.day, xmpValue.hour, xmpValue.minute, xmpValue.second );

		tiff->SetTag_ASCII ( mainIFD, mainID, buffer );
		
		if ( xmpValue.nanoSecond != 0 ) {

			snprintf ( buffer, sizeof(buffer), "%09d", xmpValue.nanoSecond );	// AUDIT: Use of sizeof(buffer) is safe.
			for ( size_t i = strlen(buffer)-1; i > 0; --i ) {
				if ( buffer[i] != '0' ) break;
				buffer[i] = 0;	// Strip trailing zero digits.
			}

			tiff->SetTag_ASCII ( fracIFD, fracID, buffer );

		}

	} catch ( ... ) {
		// Do nothing, let other exports proceed.
		// ? Notify client?
	}
	
}	// ExportTIFF_Date

// =================================================================================================
// ExportTIFF_LocTextASCII
// ======================

static void
ExportTIFF_LocTextASCII ( const SXMPMeta & xmp, const char * xmpNS, const char * xmpProp,
						  TIFF_Manager * tiff, XMP_Uns8 ifd, XMP_Uns16 id )
{
	try {	// Don't let errors with one stop the others.

		std::string xmpValue;

		bool foundXMP = xmp.GetLocalizedText ( xmpNS, xmpProp, "", "x-default", 0, &xmpValue, 0 );
		if ( ! foundXMP ) {
			tiff->DeleteTag ( ifd, id );
			return;
		}
		
		tiff->SetTag ( ifd, id, kTIFF_ASCIIType, xmpValue.size()+1, xmpValue.c_str() );

	} catch ( ... ) {
		// Do nothing, let other exports proceed.
		// ? Notify client?
	}
	
}	// ExportTIFF_LocTextASCII

// =================================================================================================
// ExportTIFF_EncodedString
// ========================

static void
ExportTIFF_EncodedString ( const SXMPMeta & xmp, const char * xmpNS, const char * xmpProp,
						   TIFF_Manager * tiff, XMP_Uns8 ifd, XMP_Uns16 id )
{
	try {	// Don't let errors with one stop the others.

		std::string    xmpValue;
		XMP_OptionBits xmpFlags;

		bool foundXMP = xmp.GetProperty ( xmpNS, xmpProp, &xmpValue, &xmpFlags );
		if ( ! foundXMP ) {
			tiff->DeleteTag ( ifd, id );
			return;
		}

		if ( ! XMP_PropIsSimple ( xmpFlags ) ) return;	// ? Complain? Delete the tag?
		
		XMP_Uns8 encoding = kTIFF_EncodeASCII;
		for ( size_t i = 0; i < xmpValue.size(); ++i ) {
			if ( xmpValue[i] >= 0x80 ) {
				encoding = kTIFF_EncodeUnicode;
				break;
			}
		}
		
		tiff->SetTag_EncodedString ( ifd, id, xmpValue.c_str(), encoding );

	} catch ( ... ) {
		// Do nothing, let other exports proceed.
		// ? Notify client?
	}
	
}	// ExportTIFF_EncodedString

// =================================================================================================
// =================================================================================================

// =================================================================================================
// ReconcileUtils::ExportTIFF
// ==========================
//
// Only a few tags are written back from XMP to the primary IFD, they are each handled explicitly.
// The writeback tags are:
//   270 - ImageDescription
//   274 - Orientation
//   282 - XResolution
//   283 - YResolution
//   296 - ResolutionUnit
//   305 - Software
//   306 - DateTime
//   315 - Artist
//   33432 - Copyright

// *** need to determine if the XMP has changed - only export when necessary

void
ReconcileUtils::ExportTIFF ( const SXMPMeta & xmp, TIFF_Manager * tiff )
{
	
	ExportTIFF_LocTextASCII ( xmp, kXMP_NS_DC, "description",
							  tiff, kTIFF_PrimaryIFD, kTIFF_ImageDescription );

	ExportSingleTIFF_Short ( xmp, kXMP_NS_TIFF, "Orientation",
							 tiff, kTIFF_PrimaryIFD, kTIFF_Orientation );

	ExportSingleTIFF_Rational ( xmp, kXMP_NS_TIFF, "XResolution",
								tiff, kTIFF_PrimaryIFD, kTIFF_XResolution );

	ExportSingleTIFF_Rational ( xmp, kXMP_NS_TIFF, "YResolution",
								tiff, kTIFF_PrimaryIFD, kTIFF_YResolution );

	ExportSingleTIFF_Short ( xmp, kXMP_NS_TIFF, "ResolutionUnit",
							 tiff, kTIFF_PrimaryIFD, kTIFF_ResolutionUnit );

	ExportSingleTIFF_ASCII ( xmp, kXMP_NS_XMP, "CreatorTool",
							 tiff, kTIFF_PrimaryIFD, kTIFF_Software );

	ExportTIFF_Date ( xmp, kXMP_NS_XMP, "ModifyDate",
					  tiff, kTIFF_PrimaryIFD, kTIFF_DateTime, kTIFF_ExifIFD, kTIFF_SubSecTime );
	
	ExportArrayTIFF_ASCII ( xmp, kXMP_NS_DC, "creator",
							tiff, kTIFF_PrimaryIFD, kTIFF_Artist );

	ExportTIFF_LocTextASCII ( xmp, kXMP_NS_DC, "rights",
							  tiff, kTIFF_PrimaryIFD, kTIFF_Copyright );

}	// ReconcileUtils::ExportTIFF;

// =================================================================================================
// ReconcileUtils::ExportExif
// ==========================
//
// Only a few tags are written back from XMP to the Exif IFD, they are each handled explicitly.
// The writeback tags are:
//   36867 - DateTimeOriginal (plus 37521 SubSecTimeOriginal)
//   36868 - DateTimeDigitized (plus 37522 SubSecTimeDigitized)
//   37510 - UserComment
//   40964 - RelatedSoundFile

// ! Older versions of Photoshop did not import the UserComment or RelatedSoundFile tags. Don't
// ! export the current XMP unless the original XMP had the tag or the current XMP has the tag.
// ! That is, don't delete the Exif tag if the XMP never had the property.

void
ReconcileUtils::ExportExif ( const SXMPMeta & xmp, TIFF_Manager * tiff )
{

	if ( xmp.DoesPropertyExist ( kXMP_NS_EXIF, "DateTimeOriginal" ) ) {
		ExportTIFF_Date ( xmp, kXMP_NS_EXIF, "DateTimeOriginal",
						  tiff, kTIFF_ExifIFD, kTIFF_DateTimeOriginal, kTIFF_ExifIFD, kTIFF_SubSecTimeOriginal );
	}

	if ( xmp.DoesPropertyExist ( kXMP_NS_EXIF, "DateTimeDigitized" ) ) {
		ExportTIFF_Date ( xmp, kXMP_NS_EXIF, "DateTimeDigitized",
						  tiff, kTIFF_ExifIFD, kTIFF_DateTimeDigitized, kTIFF_ExifIFD, kTIFF_SubSecTimeDigitized );
	}

	if ( tiff->xmpHadUserComment || xmp.DoesPropertyExist ( kXMP_NS_EXIF, "UserComment" ) ) {
		ExportTIFF_EncodedString ( xmp, kXMP_NS_EXIF, "UserComment",
								   tiff, kTIFF_ExifIFD, kTIFF_UserComment );
	}

	if ( tiff->xmpHadRelatedSoundFile || xmp.DoesPropertyExist ( kXMP_NS_EXIF, "RelatedSoundFile" ) ) {
		ExportSingleTIFF_ASCII ( xmp, kXMP_NS_EXIF, "RelatedSoundFile",
								 tiff, kTIFF_ExifIFD, kTIFF_RelatedSoundFile );
	}

}	// ReconcileUtils::ExportExif;
