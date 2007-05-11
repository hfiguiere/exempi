#ifndef __Reconcile_Impl_hpp__
#define __Reconcile_Impl_hpp__ 1

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
#include "MD5.h"

// =================================================================================================
/// \file Reconcile_Impl.hpp
/// \brief Implementation utilities for the legacy metadata reconciliation support.
///
// =================================================================================================

typedef XMP_Uns8 MD5_Digest[16];	// ! Should be in MD5.h.

enum {
	kDigestMissing = -1,	// A partial import is done, existing XMP is left alone.
	kDigestDiffers =  0,	// A full import is done, existing XMP is deleted or replaced.
	kDigestMatches = +1		// No importing is done.
};

namespace ReconcileUtils {
	
	static const char * kHexDigits = "0123456789ABCDEF";
	
	bool IsUTF8       ( const void * _utf8Ptr, size_t utf8Len );
	void UTF8ToLocal  ( const void * _utf8Ptr, size_t utf8Len, std::string * local );
	void UTF8ToLatin1 ( const void * _utf8Ptr, size_t utf8Len, std::string * latin1 );
	void LocalToUTF8  ( const void * _localPtr, size_t localLen, std::string * utf8 );
	void Latin1ToUTF8 ( const void * _latin1Ptr, size_t latin1Len, std::string * utf8 );
		// *** These ought to be with the Unicode conversions.

	int CheckIPTCDigest ( IPTC_Manager * iptc, const PSIR_Manager & psir );
	int CheckTIFFDigest ( const TIFF_Manager & tiff, const SXMPMeta & xmp );
	int CheckExifDigest ( const TIFF_Manager & tiff, const SXMPMeta & xmp );

	void SetIPTCDigest ( IPTC_Manager * iptc, PSIR_Manager * psir );
	void SetTIFFDigest ( const TIFF_Manager & tiff, SXMPMeta * xmp );
	void SetExifDigest ( const TIFF_Manager & tiff, SXMPMeta * xmp );
	
	void ImportIPTC ( const IPTC_Manager & iptc, SXMPMeta * xmp, int digestState );
	void ImportPSIR ( const PSIR_Manager & psir, SXMPMeta * xmp, int digestState );
	void ImportTIFF ( const TIFF_Manager & tiff, SXMPMeta * xmp, int digestState, XMP_FileFormat srcFormat );
	void ImportExif ( const TIFF_Manager & tiff, SXMPMeta * xmp, int digestState );
	
	void ExportIPTC ( SXMPMeta * xmp, IPTC_Manager * iptc );	// ! Has XMP side effects!
	void ExportPSIR ( const SXMPMeta & xmp, PSIR_Manager * psir );
	void ExportTIFF ( const SXMPMeta & xmp, TIFF_Manager * tiff );
	void ExportExif ( const SXMPMeta & xmp, TIFF_Manager * tiff );

};	// ReconcileUtils

#endif	// __Reconcile_Impl_hpp__
