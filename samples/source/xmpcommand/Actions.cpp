// =================================================================================================
// Copyright 2004-2008 Adobe Systems Incorporated
// All Rights Reserved.
//
// NOTICE:  Adobe permits you to use, modify, and distribute this file in accordance with the terms
// of the Adobe license agreement accompanying it.
// =================================================================================================

// actions.cpp|h
// performs one of the XMPFiles actions as indicated by the paramters (and evaluated in main)
//

const char * XMP_EXE_VERSION= "4.4";

#include <stdexcept>
#include <stdarg.h>

//XMP related
#define TXMP_STRING_TYPE std::string
#define XMP_INCLUDE_XMPFILES 1
#include "XMP.hpp"			//NB: no XMP.incl_cpp here on purpose, gets compiled in main...
#include "XMP_Const.h"

#include "globals.h"
#include "Log.h"
#include "OutputUtils.h"

#include "Actions.h" //must come after XMP.hpp/XMP_Const.h

// utility functions **************************************************************

	/* does NOT include path fixing (unix vs wxp) on purpose
	* being readable of course implies existing
	*  - note: this utility is also used by cppunit sanity checks
	*/
	bool fileIsReadable(const std::string filename)
	{
		FILE* fp = NULL;
		fp = fopen( filename.c_str(), "rb" );
		if( fp != NULL )
		{
			fclose( fp );
			return true;
		}
		return false;
	}

	bool fileIsWritable(const std::string filename)
	{
		FILE* fp = NULL;
		fp = fopen( filename.c_str(), "rb+" );
		if( fp != NULL )
		{
			fclose( fp );
			return true;
		}
		return false;
	}

	void verifyFileIsReadable(const std::string filename)
	{
		if (! fileIsReadable(filename))
			Log::error("file %s is not readable or not existing.",filename.c_str());

	}

	void verifyFileIsWritable(const std::string filename)
	{
		if (! fileIsWritable(filename))
			Log::error("file %s is not readwritable or not existing.",filename.c_str());
	}

	
	std::string getStringFromFile(const std::string& filename)
	{
		verifyFileIsReadable(filename);

		//figure out length
		FILE * file = fopen ( filename.c_str(), "rb" );
		fseek ( file, 0, SEEK_END );
		XMP_Uns32 length = ftell( file );
		fseek ( file, 0, SEEK_SET );
		
		//write into string
		std::string content;
		content.reserve ( (XMP_Uns32) length );
		content.append ( length, ' ' );
		fread ( (char*)content.data(), 1, length, file );
		fclose ( file );
		return content;
	}

// ********************************************************************************

void Actions::version() {
	XMP_VersionInfo coreVersion, filesVersion;
	SXMPMeta::GetVersionInfo ( &coreVersion );
	SXMPFiles::GetVersionInfo ( &filesVersion );
	Log::info("%s", coreVersion.message);
	Log::info("%s", filesVersion.message);
	Log::info("Executable Version:%s", XMP_EXE_VERSION);

#ifdef WIN_ENV
	Log::info("compiled on Windows on %s, %s",__DATE__,__TIME__);
#endif
#ifdef MAC_ENV
	Log::info("compiled on Mac on %s, %s",__DATE__,__TIME__);
#endif
#ifndef NDEBUG
	Log::info("Configuration: debug");
#else
	Log::info("Configuration: release");
#endif
	Log::info("Edition: Public SDK");
#if defined(XMPQE_BIG_ENDIAN)
	Log::info("Big endian machine");
#elif defined(XMPQE_LITTLE_ENDIAN)
	Log::info("Little endian machine");
#else
	Log::warn("unknown Endian !!!");
#endif
}

///////////////////////////////////////////////////////////////////////////////////////
void Actions::info() {
	if(!this->switch_nocheck) verifyFileIsReadable(this->mediafile);
	XMP_FileFormat xmpFileFormat=kXMP_UnknownFile;
	SXMPFiles xmpFile;

	if ( !xmpFile.OpenFile(this->mediafile,xmpFileFormat,generalOpenFlags) )
		Log::error("error opening file %s",this->mediafile.c_str());

	std::string out_filepath;
	XMP_OptionBits out_openFlags;
	XMP_FileFormat out_fileFormat;
	XMP_OptionBits out_handlerFlags;

	if (!xmpFile.GetFileInfo(&out_filepath,&out_openFlags,&out_fileFormat,&out_handlerFlags))
		Log::error("error doing GetFileInfo %s",this->mediafile.c_str());

	//FilePath (just verify that identical to what's been used)
	if ( strcmp(out_filepath.c_str(),this->mediafile.c_str()) ) //that's if it NOT matches (!=0 ...)
		Log::warn("media filepath %s does not matches filepath %s obtained from GetFileInfo",
		out_filepath.c_str(),this->mediafile.c_str());

	//openOptions (also verify that identical to what's been used)
	std::string openFlagsString=Utils::XMPFiles_OpenOptionsToString(out_openFlags);
	Log::info("openFlags: %s (%X)", openFlagsString.c_str(), out_openFlags);
	if ( generalOpenFlags != out_openFlags )
		Log::warn("out_openFlags (0x%X) differ from those used for open (0x%X)",
		out_openFlags,generalOpenFlags);

	//FileFormat (resolves, usually by way of extension unless specified)
	std::string fileFormatName, fileFormatDescription;
	Utils::fileFormatNameToString(out_fileFormat,fileFormatName,fileFormatDescription);
	Log::info("fileFormat: %s (%s,0x%X)",
		fileFormatDescription.c_str(),fileFormatName.c_str(),out_fileFormat);

	//FormatInfo aka "HandlerFlags" (show what is possible with this format)
	std::string formatInfoString=Utils::XMPFiles_FormatInfoToString(out_handlerFlags);
	Log::info("formatInfo:%s (0x%X)", formatInfoString.c_str(), out_handlerFlags);

	xmpFile.CloseFile(generalCloseFlags); //(enabled again now that bug 1352603 is fixed)
}

///////////////////////////////////////////////////////////////////////////////////////
void Actions::get() {
	if(!this->switch_nocheck) verifyFileIsReadable(this->mediafile);
	XMP_FileFormat xmpFileFormat=kXMP_UnknownFile;

	SXMPFiles xmpFile;
	std::string xmpDump; //really just the raw string here

	if ( !xmpFile.OpenFile(this->mediafile,xmpFileFormat,generalOpenFlags) )
		Log::error("error opening file %s",this->mediafile.c_str());

	if ( this->switch_compact ) {
		if ( !xmpFile.GetXMP(0,&xmpDump,0))
			Log::warn("file contains no XMP. - says xmpFile.GetXMP()");
		else
			Log::info("%s",xmpDump.c_str());
	} else {
		SXMPMeta xmpMeta; //get meta-object first, then get serialization
		if ( !xmpFile.GetXMP(&xmpMeta,0,0))
			Log::warn("file contains no XMP. - says xmpFile.GetXMP()");
		else {
			xmpMeta.SerializeToBuffer(&xmpDump,
				0, //NOT using kXMP_UseCompactFormat - Use a highly compact RDF syntax and layout.
				0);		//receiving string, options, 0(default) padding
			Log::info("%s",xmpDump.c_str());
		}
	}

	xmpFile.CloseFile(generalCloseFlags); //(enabled again now that bug 1352603 is fixed)
}
///////////////////////////////////////////////////////////////////////////////////////

void Actions::dump() {
	if(!this->switch_nocheck) verifyFileIsReadable(this->mediafile);
	XMP_FileFormat xmpFileFormat=kXMP_UnknownFile;

	SXMPFiles xmpFile;
	std::string xmpDump; //really just the raw string here

	if ( !xmpFile.OpenFile(this->mediafile,xmpFileFormat,generalOpenFlags) )
		Log::error("error opening file %s",this->mediafile.c_str());

	SXMPMeta xmpMeta; //get meta-object first, then get serialization
	if ( !xmpFile.GetXMP(&xmpMeta,0,0))
		Log::warn("file contains no XMP. - says xmpFile.GetXMP()");
	else {
		Utils::dumpXMP(&xmpMeta);
	}
	
	xmpFile.CloseFile(generalCloseFlags); //(enabled again now that bug 1352603 is fixed)
}


///////////////////////////////////////////////////////////////////////////////////////
void Actions::thumb() {
	if(!this->switch_nocheck) verifyFileIsReadable(this->mediafile); //need readability
	XMP_FileFormat xmpFileFormat=kXMP_UnknownFile;

	SXMPFiles xmpFile;
	XMP_ThumbnailInfo tnailInfo;
	if ( !xmpFile.OpenFile(this->mediafile,
							xmpFileFormat,
							 generalOpenFlags | kXMPFiles_OpenCacheTNail) )
		Log::error("error opening file %s",this->mediafile.c_str());

	if ( !xmpFile.GetThumbnail(&tnailInfo)) {
		Log::warn("no thumbnail found. - says xmpFile.GetThumbnail()");
		//TODO: comment-in again once bug 1352603 fixed
		//xmpFile.CloseFile(generalCloseFlags);
		return;
	}
	else {
		//FileFormat (resolves, usually by way of extension unless specified)
		std::string fileFormatName, fileFormatDescription;
		Utils::fileFormatNameToString(tnailInfo.fileFormat,fileFormatName,fileFormatDescription);
		Log::info("fileFormat: %s (%s,0x%X)",
			fileFormatDescription.c_str(),fileFormatName.c_str(),tnailInfo.fileFormat);

		Log::info("full image size:%dx%d",tnailInfo.fullWidth,tnailInfo.fullHeight);
		Log::info("full orientation:0x%X",tnailInfo.fullOrientation);
		Log::info("thumbnail size:%dx%d (%d bytes)",tnailInfo.tnailWidth,tnailInfo.tnailHeight,tnailInfo.tnailSize);
		Log::info("thumbnail orientation:0x%X",tnailInfo.tnailOrientation);
		Log::info("thumbnail format 0x%X",tnailInfo.tnailFormat);
	}

	xmpFile.CloseFile(generalCloseFlags); //(enabled again now that bug 1352603 is fixed)
}


///////////////////////////////////////////////////////////////////////////////////////
void Actions::put() {
	//need read and write
	if(!this->switch_nocheck) verifyFileIsWritable(this->mediafile);

	//first open regular to see if injection possible
	XMP_FileFormat xmpFileFormat=kXMP_UnknownFile;
	SXMPFiles xmpFile;

	if ( !xmpFile.OpenFile(this->mediafile,xmpFileFormat,generalOpenFlags | kXMPFiles_OpenForUpdate) )
		Log::error("error opening file %s",this->mediafile.c_str());

	std::string out_filepath;
	XMP_FileFormat out_fileFormat;
	XMP_OptionBits out_handlerFlags;

	if (!xmpFile.GetFileInfo(0,0,&out_fileFormat,&out_handlerFlags))
		Log::error("error doing GetFileInfo %s",this->mediafile.c_str());

	bool foundXMP = xmpFile.GetXMP( 0, 0, 0);

	//construct the to-be-injected snippet
	std::string xmpMetaString=getStringFromFile(this->xmpsnippet);
	SXMPMeta xmpMeta(xmpMetaString.c_str(),(XMP_StringLen)xmpMetaString.length());

	// append metadata if we found XMP or if we didn't but can inject
	if ( foundXMP == true || (out_handlerFlags & kXMPFiles_CanInjectXMP ) ) {

		if ( !xmpFile.CanPutXMP(xmpMeta) )
			Log::warn("canPutXMP denies I can inject: (xmpFile.CanPutXMP()==false)");

		try {
			xmpFile.PutXMP(xmpMeta); //void unfortunatley, must close file to figure out result...
			xmpFile.CloseFile(generalCloseFlags); // close the file

		} catch ( XMP_Error& e ) {
			Log::error("puttingXMP failed: %s",e.GetErrMsg());
		}
	} else { //handle the error
		//FileFormat (resolves, usually by way of extension unless specified TODO)
		std::string fileFormatName, fileFormatDescription;
		Utils::fileFormatNameToString(out_fileFormat,fileFormatName,fileFormatDescription);
		Log::error("the File %s of type %s neither contains XMP nor can it be injected (at least kXMPFiles_CanInjectXMP not returned)",
			this->mediafile.c_str(), fileFormatName.c_str());
	};
}

void Actions::doAction() {
	//do open and close-flags one for all (add ThumbFlag if needed above)
	generalOpenFlags=kXMPFiles_OpenForRead;
	if (this->switch_smart) generalOpenFlags|=kXMPFiles_OpenUseSmartHandler;
	if (this->switch_scan) generalOpenFlags|=kXMPFiles_OpenUsePacketScanning;

	generalCloseFlags=0;
	if (this->switch_safe) generalOpenFlags|=kXMPFiles_UpdateSafely;

	switch (this->actiontype){
		case Actions::VERSION:
			version();
			break;
		case Actions::INFO:
			info();
			break;
		case Actions::THUMB: //GetThumbnail()
			thumb();
			break;
		case Actions::PUT: //PutXMP()
			put();
			break;
		case Actions::GET: //GetXMP()
			get();
			break;
		case Actions::DUMP:
			dump();
			break;
		default:
			Log::error("unknown command or not implemented!!");
			break;
	}
}
