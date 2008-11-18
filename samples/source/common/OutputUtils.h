// =================================================================================================
// Copyright 2005-2008 Adobe Systems Incorporated
// All Rights Reserved.
//
// NOTICE:  Adobe permits you to use, modify, and distribute this file in accordance with the terms
// of the Adobe license agreement accompanying it.
//
// =================================================================================================

#ifndef __XMPQE_QEOUTPUTUTILS_h__
#define __XMPQE_QEOUTPUTUTILS_h__ 1

//XMP related
#define  TXMP_STRING_TYPE std::string

#include "XMP.hpp"

namespace Utils{	

	
	void fileFormatNameToString( XMP_FileFormat fileFormat, std::string &fileFormatName, std::string &fileFormatDesc );

	std::string XMPFiles_FormatInfoToString ( const XMP_OptionBits options);
	std::string XMPFiles_OpenOptionsToString ( const XMP_OptionBits options);

	std::string fromArgs(const char* format, ...);

	//just the DumpToString Callback routine
	//   - (taken over from dfranzen resp. DevTestProto)
	//   - /*outLen*/ ==> avoid unused warning
	XMP_Status DumpToString ( void * refCon, XMP_StringPtr outStr, XMP_StringLen /*outLen*/ );

	// dumps xmp (using Log::info())
	// based on dfranzen's conglomerate of dump-functions in XMPTestCase.cpp
	// no return value, will Log::error() on problems
	void dumpXMP(SXMPMeta* xmpMeta);
	void dumpXMP(std::string comment, SXMPMeta* xmpMeta);
	void dumpAliases();

	//remove n first lines from a string
	// handy i.e. for first line of xmp-rdf packets prior to KGO,
	// since firt line may contain -date- or actual date,
	// (debug) or 7 spaces, etc...
	void removeNLines(std::string* s, int n=1);

	void DumpXMP_DateTime( const XMP_DateTime & date );

} 

#endif
