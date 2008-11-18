// =================================================================================================
// Copyright 2005-2008 Adobe Systems Incorporated
// All Rights Reserved.
//
// NOTICE:  Adobe permits you to use, modify, and distribute this file in accordance with the terms
// of the Adobe license agreement accompanying it.
//
// =================================================================================================

#include <string>
#include <stdarg.h>

#include "OutputUtils.h"
#include "Log.h"


using namespace std;

namespace Utils {
	/* hand over the fileFormat and get a fileFormatName (2-3 Letters ie. PDF) 
	* and a Description (i.e. "Portable Document Format") back.
	* see also public/include/XMP_Const.h
	*/
	void fileFormatNameToString ( XMP_FileFormat fileFormat, std::string & fileFormatName, std::string & fileFormatDesc ) 
	{
		fileFormatName.erase();
		fileFormatDesc.erase();
		switch(fileFormat) {
		case kXMP_PDFFile:
			fileFormatName = "PDF ";fileFormatDesc = "Portable Document Format";break;
		case kXMP_PostScriptFile:
			fileFormatName = "PS  ";fileFormatDesc = "Post Script";break;
		case kXMP_EPSFile:
			fileFormatName = "EPS ";fileFormatDesc = "Encapsulated Post Script";break;
		case kXMP_JPEGFile:
			fileFormatName = "JPEG";fileFormatDesc = "Joint Photographic Experts Group";break;
		case kXMP_JPEG2KFile:
			fileFormatName = "JPX ";fileFormatDesc = "JPEG 2000";break;
		case kXMP_TIFFFile:
			fileFormatName = "TIFF";fileFormatDesc = "Tagged Image File Format";break;
		case kXMP_GIFFile:
			fileFormatName = "GIF ";fileFormatDesc = "Graphics Interchange Format";break;
		case kXMP_PNGFile:
			fileFormatName = "PNG ";fileFormatDesc = "Portable Network Graphic";break;
		case kXMP_MOVFile:
			fileFormatName = "MOV ";fileFormatDesc = "Quicktime";break;
		case kXMP_AVIFile:
			fileFormatName = "AVI ";fileFormatDesc = "Quicktime";break;
		case kXMP_CINFile:
			fileFormatName = "CIN ";fileFormatDesc = "Cineon";break;
		case kXMP_WAVFile:
			fileFormatName = "WAV ";fileFormatDesc = "WAVE Form Audio Format";break;
		case kXMP_MP3File:
			fileFormatName = "MP3 ";fileFormatDesc = "MPEG-1 Audio Layer 3";break;
		case kXMP_SESFile:
			fileFormatName = "SES ";fileFormatDesc = "Audition session";break;
		case kXMP_CELFile:
			fileFormatName = "CEL ";fileFormatDesc = "Audition loop";break;
		case kXMP_MPEGFile:
			fileFormatName = "MPEG";fileFormatDesc = "Motion Pictures Experts Group";break;
		case kXMP_MPEG2File:
			fileFormatName = "MP2 ";fileFormatDesc = "MPEG-2";break;
		case kXMP_WMAVFile:
			fileFormatName = "WMAV";fileFormatDesc = "Windows Media Audio and Video";break;
		case kXMP_HTMLFile:
			fileFormatName = "HTML";fileFormatDesc = "HyperText Markup Language";break;
		case kXMP_XMLFile:
			fileFormatName = "XML ";fileFormatDesc = "Extensible Markup Language";break;
		case kXMP_TextFile:
			fileFormatName = "TXT ";fileFormatDesc = "text";break;
		case kXMP_PhotoshopFile:
			fileFormatName = "PSD ";fileFormatDesc = "Photoshop Document";break;
		case kXMP_IllustratorFile:
			fileFormatName = "AI  ";fileFormatDesc = "Adobe Illustrator";break;
		case kXMP_InDesignFile:
			fileFormatName = "INDD";fileFormatDesc = "Adobe InDesign";break;
		case kXMP_AEProjectFile:
			fileFormatName = "AEP ";fileFormatDesc = "AfterEffects Project";break;
		case kXMP_AEFilterPresetFile:
			fileFormatName = "FFX ";fileFormatDesc = "AfterEffects Filter Preset";break;
		case kXMP_EncoreProjectFile:
			fileFormatName = "NCOR";fileFormatDesc = "Encore Project";break;
		case kXMP_PremiereProjectFile:
			fileFormatName = "PPRJ";fileFormatDesc = "Premier Project";break;
		case kXMP_SWFFile:
			fileFormatName = "SWF ";fileFormatDesc = "Shockwave Flash";break;
		case kXMP_PremiereTitleFile:
			fileFormatName = "PRTL";fileFormatDesc = "Premier Title";break;
		default:
			fileFormatName = "    ";fileFormatDesc = "Unkown file format";break;
		}
	} //fileFormatNameToString

	/**
	* GetFormatInfo-Flags  (aka Handler-Flags)
	* find this in XMP_Const.h under "Options for GetFormatInfo"
	*/
	std::string XMPFiles_FormatInfoToString ( const XMP_OptionBits options) {
		std::string outString;

		if( options & kXMPFiles_CanInjectXMP )
			outString.append(" CanInjectXMP");
		if( options & kXMPFiles_CanExpand )
			outString.append(" CanExpand");
		if( options & kXMPFiles_CanRewrite )
			outString.append(" CanRewrite");
		if( options & kXMPFiles_PrefersInPlace )
			outString.append(" PrefersInPlace");
		if( options & kXMPFiles_CanReconcile )
			outString.append(" CanReconcile");
		if( options & kXMPFiles_AllowsOnlyXMP )
			outString.append(" AllowsOnlyXMP");
		if( options & kXMPFiles_ReturnsRawPacket )
			outString.append(" ReturnsRawPacket");
		if( options & kXMPFiles_ReturnsTNail )
			outString.append(" ReturnsTNail");
		if( options & kXMPFiles_HandlerOwnsFile )
			outString.append(" HandlerOwnsFile");
		if( options & kXMPFiles_AllowsSafeUpdate )
			outString.append(" AllowsSafeUpdate");

		if (outString.empty()) outString=" (none)";
		return outString;
	}

	/**
	* openOptions to String, find this in 
	* XMP_Const.h under "Options for OpenFile"
	*/
	std::string XMPFiles_OpenOptionsToString ( const XMP_OptionBits options) {
		std::string outString;
		if( options & kXMPFiles_OpenForRead)
			outString.append(" OpenForRead");
		if( options & kXMPFiles_OpenForUpdate)
			outString.append(" OpenForUpdate");
		if( options & kXMPFiles_OpenOnlyXMP)
			outString.append(" OpenOnlyXMP");
		if( options & kXMPFiles_OpenCacheTNail)
			outString.append(" OpenCacheTNail");
		if( options & kXMPFiles_OpenStrictly)
			outString.append(" OpenStrictly");
		if( options & kXMPFiles_OpenUseSmartHandler)
			outString.append(" OpenUseSmartHandler");
		if( options & kXMPFiles_OpenUsePacketScanning)
			outString.append(" OpenUsePacketScanning");
		if( options & kXMPFiles_OpenLimitedScanning)
			outString.append(" OpenLimitedScanning");

		if (outString.empty()) outString=" (none)";
		return outString;
	}

	

	std::string fromArgs(const char* format, ...)
	{
		//note: format and ... are somehow "used up", i.e. dumping them 
		//      via vsprintf _and_ via printf brought up errors on Mac (only)
		//      i.e. %d %X stuff looking odd (roughly like signed vs unsigned...)
		//      buffer reuse is fine, just dont use format/... twice.

		char buffer[4096];  //should be big enough but no guarantees..
		va_list args;
		va_start(args, format);
			vsprintf(buffer, format, args);
		va_end(args);

		return std::string(buffer);
	}

	

	//just the callback-Function
	XMP_Status DumpToString( void * refCon, XMP_StringPtr outStr, XMP_StringLen outLen )
	{
		XMP_Status status	= 0; 
		std::string* dumpString = static_cast < std::string* > ( refCon );
		dumpString->append (outStr, outLen); // less effective: ( std::string(outStr).substr(0,outLen) );
		return status;
	}

	//full output to Log
	void dumpXMP(SXMPMeta* xmpMeta){
		std::string dump;
		xmpMeta->Sort();
		xmpMeta->DumpObject(DumpToString, &dump);
		Log::info("%s",dump.c_str());
	}

	//same but with Comment
	void dumpXMP(std::string comment, SXMPMeta* xmpMeta){
		Log::info("%s",comment.c_str());
		std::string dump;
		xmpMeta->Sort();
		xmpMeta->DumpObject(DumpToString, &dump);
		Log::info("%s",dump.c_str());
	}

	// dump Aliases ****************************
	//full output to Log
	void dumpAliases()
	{
		std::string dump;
		SXMPMeta::DumpAliases(DumpToString,&dump);
		Log::info("List of aliases:");
		Log::info("%s",dump.c_str());
	}

	void DumpXMP_DateTime( const XMP_DateTime & date ) {
		static std::string tempStr;
		tempStr.clear();
		SXMPUtils::ConvertFromDate( date, &tempStr );
		Log::info("%s\nYear: '%d' Month: '%d' Day: '%d' Hour: '%d' Min: '%d' Sec: '%d' Nanos: '%d' TZSign: '%d' TZHour: '%d' TZMinute: '%d'\n", tempStr.c_str(), date.year, date.month, date.day, date.hour, date.minute, date.second, date.nanoSecond, date.tzSign, date.tzHour, date.tzMinute );
	}


	

	void removeNLines(std::string* s, int n){
		for(int i=0;i<n;i++)
			s->erase(0,s->find_first_of("\n")+1);
	}

	

} /*namespace Utils*/

