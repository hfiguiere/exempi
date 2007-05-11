// =================================================================================================
// ADOBE SYSTEMS INCORPORATED
// Copyright 2006-2007 Adobe Systems Incorporated
// All Rights Reserved
//
// NOTICE: Adobe permits you to use, modify, and distribute this file in accordance with the terms
// of the Adobe license agreement accompanying it.
// =================================================================================================

#include "XMP_Environment.h"	// ! This must be the first include.

#include "ReconcileLegacy.hpp"
#include "Reconcile_Impl.hpp"

// =================================================================================================
/// \file ReconcileLegacy.cpp
/// \brief Top level parts of utilities to reconcile between XMP and legacy metadata forms such as 
/// TIFF/Exif and IPTC.
///
// =================================================================================================

// =================================================================================================
// ImportJTPtoXMP
// ==============
//
// Import legacy metadata for JPEG, TIFF, and Photoshop files into the XMP. The caller must have
// already done the file specific processing to select the appropriate sources of the TIFF stream,
// the Photoshop image resources, and the IPTC.

// ! Note that kLegacyJTP_None does not literally mean no legacy. It means no IPTC-like legacy, i.e.
// ! stuff that Photoshop pre-7 would reconcile into the IPTC and thus affect the import order below.

void ImportJTPtoXMP ( XMP_FileFormat		srcFormat,
					  RecJTP_LegacyPriority lastLegacy,
					  TIFF_Manager *        tiff,	// ! Need for UserComment and RelatedSoundFile hack.
					  const PSIR_Manager &  psir,
					  IPTC_Manager *        iptc,	// ! Need to call UpdateDataSets.
					  SXMPMeta *			xmp,
					  XMP_OptionBits		options /* = 0 */ )
{
	bool haveXMP  = XMP_OptionIsSet ( options, k2XMP_FileHadXMP );
	bool haveIPTC = XMP_OptionIsSet ( options, k2XMP_FileHadIPTC );
	bool haveExif = XMP_OptionIsSet ( options, k2XMP_FileHadExif );
	
	int iptcDigestState = kDigestMatches;	// Default is to do no imports.
	int tiffDigestState = kDigestMatches;
	int exifDigestState = kDigestMatches;

	if ( ! haveXMP ) {
	
		// If there is no XMP then what we have differs.
		if ( haveIPTC) iptcDigestState = kDigestDiffers;
		if ( haveExif ) tiffDigestState = exifDigestState = kDigestDiffers;
	
	} else {

		// If there is XMP then check the digests for what we have. No legacy at all means the XMP
		// is OK, and the CheckXyzDigest routines return true when there is no digest. This matches
		// Photoshop, and avoids importing when an app adds XMP but does not export to the legacy or
		// write a digest.

		if ( haveIPTC ) iptcDigestState = ReconcileUtils::CheckIPTCDigest ( iptc, psir );
		if ( iptcDigestState == kDigestMissing ) {
			// *** Temporary hack to approximate Photoshop's behavior. Need fully documented policies!
			tiffDigestState = exifDigestState = kDigestMissing;
		} else if ( haveExif ) {
			tiffDigestState = ReconcileUtils::CheckTIFFDigest ( *tiff, *xmp );
			exifDigestState = ReconcileUtils::CheckExifDigest ( *tiff, *xmp );	// ! Yes, the Exif is in the TIFF stream.
		}

	}
	
	if ( lastLegacy > kLegacyJTP_TIFF_IPTC ) {
		XMP_Throw ( "Invalid JTP legacy priority", kXMPErr_InternalFailure );
	}

	// A TIFF file with tags 270, 315, or 33432 is currently the only case where the IPTC is less
	// important than the TIFF/Exif. If there is no IPTC or no TIFF/Exif then the order does not
	// matter. The order only affects collisions between those 3 TIFF tags and their IPTC counterparts.
	
	if ( lastLegacy == kLegacyJTP_TIFF_TIFF_Tags ) {

		if ( iptcDigestState != kDigestMatches ) {
			ReconcileUtils::ImportIPTC ( *iptc, xmp, iptcDigestState );
			ReconcileUtils::ImportPSIR ( psir, xmp, iptcDigestState );
		}
		if ( tiffDigestState != kDigestMatches ) ReconcileUtils::ImportTIFF ( *tiff, xmp, tiffDigestState, srcFormat );
		if ( exifDigestState != kDigestMatches ) ReconcileUtils::ImportExif ( *tiff, xmp, exifDigestState );

	} else {

		if ( tiffDigestState != kDigestMatches ) ReconcileUtils::ImportTIFF ( *tiff, xmp, tiffDigestState, srcFormat );
		if ( exifDigestState != kDigestMatches ) ReconcileUtils::ImportExif ( *tiff, xmp, exifDigestState );
		if ( iptcDigestState != kDigestMatches ) {
			ReconcileUtils::ImportIPTC ( *iptc, xmp, iptcDigestState );
			ReconcileUtils::ImportPSIR ( psir, xmp, iptcDigestState );
		}

	}
	
	// ! Older versions of Photoshop did not import the UserComment or RelatedSoundFile tags. Note
	// ! whether the initial XMP has these tags. Don't delete them from the TIFF when saving unless
	// ! they were in the XMP to begin with. Can't do this in ReconcileUtils::ImportExif, that is
	// ! only called when the Exif is newer than the XMP.
	
	tiff->xmpHadUserComment = xmp->DoesPropertyExist ( kXMP_NS_EXIF, "UserComment" );
	tiff->xmpHadRelatedSoundFile = xmp->DoesPropertyExist ( kXMP_NS_EXIF, "RelatedSoundFile" );

}	// ImportJTPtoXMP

// =================================================================================================
// ExportXMPtoJTP
// ==============

void ExportXMPtoJTP ( XMP_FileFormat destFormat,
					  SXMPMeta *     xmp,
					  TIFF_Manager * tiff,
					  PSIR_Manager * psir,
					  IPTC_Manager * iptc,
					  XMP_OptionBits options /* = 0 */ )
{
	XMP_Assert ( xmp != 0 );
	XMP_Assert ( (destFormat == kXMP_JPEGFile) || (destFormat == kXMP_TIFFFile) || (destFormat == kXMP_PhotoshopFile) );

	// Save the IPTC changed flag specially. SetIPTCDigest will call UpdateMemoryDataSets, which
	// will clear the IsChanged flag. Also, UpdateMemoryDataSets can be called twice, once for the
	// general IPTC-in-PSIR case and once for the IPTC-as-TIFF-tag case.
	
	bool iptcChanged = false;
	
	// Export the individual metadata items to the legacy forms. The PSIR and IPTC must be done
	// before the TIFF and Exif. The PSIR and IPTC have side effects that can modify the XMP, and
	// thus the values written to TIFF and Exif. The side effects are the CR<->LF normalization that
	// is done to match Photoshop.
	
	if ( psir != 0) ReconcileUtils::ExportPSIR ( *xmp, psir );

	if ( iptc != 0 ) {
		ReconcileUtils::ExportIPTC ( xmp, iptc );
		iptcChanged = iptc->IsChanged();	// ! Do after calling ExportIPTC and before calling SetIPTCDigest.
		if ( psir != 0 ) ReconcileUtils::SetIPTCDigest ( iptc, psir );	// ! Do always, in case the digest was missing before.
	}

	if ( tiff != 0 ) {
		ReconcileUtils::ExportTIFF ( *xmp, tiff );
		ReconcileUtils::ExportExif ( *xmp, tiff );
		ReconcileUtils::SetTIFFDigest ( *tiff, xmp );	// ! Do always, in case the digest was missing before.
		ReconcileUtils::SetExifDigest ( *tiff, xmp );	// ! Do always, in case the digest was missing before.
	}
	
	// Now update the collections of metadata, e.g. the IPTC in PSIR 1028 or XMP in TIFF tag 700.
	// - All of the formats have the IPTC in the PSIR portion.
	// - JPEG has nothing else special.
	// - PSD has the XMP and Exif in the PSIR portion.
	// - TIFF has the XMP, IPTC, and PSIR in primary IFD tags. Yes, a 2nd copy of the IPTC.
	
	if ( (iptc != 0) && (psir != 0) && iptcChanged ) {
		void*     iptcPtr;
		XMP_Uns32 iptcLen = iptc->UpdateMemoryDataSets ( &iptcPtr );
		psir->SetImgRsrc ( kPSIR_IPTC, iptcPtr, iptcLen );
	}
	
	if ( destFormat == kXMP_PhotoshopFile ) {

		XMP_Assert ( psir != 0 );

		if ( (tiff != 0) && tiff->IsChanged() ) {
			void* exifPtr;
			XMP_Uns32 exifLen = tiff->UpdateMemoryStream ( &exifPtr );
			psir->SetImgRsrc ( kPSIR_Exif, exifPtr, exifLen );
		}

	} else if ( destFormat == kXMP_TIFFFile ) {
	
		XMP_Assert ( tiff != 0 );	

		if ( (iptc != 0) && iptcChanged ) {
			void* iptcPtr;
			XMP_Uns32 iptcLen = iptc->UpdateMemoryDataSets ( &iptcPtr );
			tiff->SetTag ( kTIFF_PrimaryIFD, kTIFF_IPTC, kTIFF_UndefinedType, iptcLen, iptcPtr );
		}

		if ( (psir != 0) && psir->IsChanged() ) {
			void* psirPtr;
			XMP_Uns32 psirLen = psir->UpdateMemoryResources ( &psirPtr );
			tiff->SetTag ( kTIFF_PrimaryIFD, kTIFF_PSIR, kTIFF_UndefinedType, psirLen, psirPtr );
		}
		
	}

}	// ExportXMPtoJTP
