/*
 * exempi - exempi.cpp
 *
 * Copyright (C) 2007 Hubert Figuiere
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1 Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 * 
 * 2 Redistributions in binary form must reproduce the above copyright
 * notice, this list of conditions and the following disclaimer in the
 * documentation and/or other materials provided with the
 * distribution.
 *
 * 3 Neither the name of the Authors, nor the names of its
 * contributors may be used to endorse or promote products derived
 * from this software wit hout specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 * COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
 * OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/** @brief this file implement the glue for XMP API 
 */

#include "xmp.h"
#include "xmpconsts.h"

#include <string>
#include <iostream>

#define XMP_INCLUDE_XMPFILES 1
#define UNIX_ENV 1
#define TXMP_STRING_TYPE std::string
#include "XMP.hpp"
#include "XMP.incl_cpp"


#ifdef __cplusplus
extern "C" {
#endif


const char NS_XMP_META[] = "adobe:ns:meta/";
const char NS_RDF[] = kXMP_NS_RDF;
const char NS_EXIF[] = kXMP_NS_EXIF;
const char NS_TIFF[] = kXMP_NS_TIFF;
const char NS_XAP[] = kXMP_NS_XMP;
const char NS_XAP_RIGHTS[] = kXMP_NS_XMP_Rights;
const char NS_DC[] = kXMP_NS_DC;
const char NS_EXIF_AUX[] = kXMP_NS_EXIF_Aux;
const char NS_CRS[] = kXMP_NS_CameraRaw;
const char NS_LIGHTROOM[] = "http://ns.adobe.com/lightroom/1.0/";
const char NS_PHOTOSHOP[] = kXMP_NS_Photoshop;
const char NS_IPTC4XMP[] = kXMP_NS_IPTCCore;
const char NS_TPG[] = kXMP_NS_XMP_PagedFile;
const char NS_DIMENSIONS_TYPE[] = kXMP_NS_XMP_Dimensions;


#define STRING(x) reinterpret_cast<std::string*>(x)


bool xmp_init()
{
	bool ret = SXMPMeta::Initialize();
	if (ret)
		ret = SXMPFiles::Initialize();
	return ret;
}

void xmp_terminate()
{
	SXMPFiles::Terminate();
	SXMPMeta::Terminate();
}


XmpFilePtr xmp_files_new()
{
	SXMPFiles *txf = new SXMPFiles();
	return (XmpFilePtr)txf;
}

XmpFilePtr xmp_files_open_new(const char *path, uint32_t options)
{
	SXMPFiles *txf = new SXMPFiles(path, kXMP_UnknownFile, options);

	return (XmpFilePtr)txf;
}


bool xmp_files_open(XmpFilePtr xf, const char *path, uint32_t options)
{
	SXMPFiles *txf = (SXMPFiles*)xf;
	try {
		return txf->OpenFile(path, kXMP_UnknownFile, options);
	}
	catch(...)
	{
	}
	return false;
}


void xmp_files_close(XmpFilePtr xf, uint32_t options)
{
	SXMPFiles *txf = (SXMPFiles*)xf;
	txf->CloseFile(options);
}


XmpPtr xmp_files_get_new_xmp(XmpFilePtr xf)
{
	SXMPMeta *xmp = new SXMPMeta();
	SXMPFiles *txf = (SXMPFiles*)xf;

	bool result = txf->GetXMP(xmp);
	if(!result) {
		delete xmp;
		return NULL;
	}
	return (XmpPtr)xmp;
}


bool xmp_files_get_xmp(XmpFilePtr xf, XmpPtr xmp)
{
	SXMPFiles *txf = (SXMPFiles*)xf;

	return txf->GetXMP((SXMPMeta*)xmp);
}


bool xmp_files_can_put_xmp(XmpFilePtr xf, XmpPtr xmp)
{
	SXMPFiles *txf = (SXMPFiles*)xf;

	return txf->CanPutXMP(*(SXMPMeta*)xmp);
}


void xmp_files_put_xmp(XmpFilePtr xf, XmpPtr xmp)
{
	SXMPFiles *txf = (SXMPFiles*)xf;
	
	txf->PutXMP(*(SXMPMeta*)xmp);
}


void xmp_files_free(XmpFilePtr xf)
{
	SXMPFiles *txf = (SXMPFiles*)xf;
	try {
		delete txf;
	}
	catch(...) {
	}
}


XmpPtr xmp_new_empty()
{
	SXMPMeta *txmp = new SXMPMeta;
	return (XmpPtr)txmp;
}


XmpPtr xmp_new(const char *buffer, size_t len)
{
	SXMPMeta *txmp = new SXMPMeta(buffer, len);
	return (XmpPtr)txmp;
}

bool xmp_parse(XmpPtr xmp, const char *buffer, size_t len)
{
	SXMPMeta *txmp = (SXMPMeta *)xmp;
	try {
		txmp->ParseFromBuffer(buffer, len, kXMP_RequireXMPMeta );
	}
	catch(...)
	{
		return false;
	}
	return true;
}

void xmp_free(XmpPtr xmp)
{
	SXMPMeta *txmp = (SXMPMeta *)xmp;
	delete txmp;
}



bool xmp_get_property(XmpPtr xmp, const char *schema, 
															const char *name, XmpStringPtr property)
{
	try {
		SXMPMeta *txmp = (SXMPMeta *)xmp;
		XMP_OptionBits options = 0;
		std::string pref;
		return txmp->GetProperty(schema, name, STRING(property), &options);
	}
	catch(const XMP_Error & e)
	{
		std::cerr << e.GetErrMsg() << std::endl;
	}
	return false;
}


void xmp_set_property(XmpPtr xmp, const char *schema, 
											const char *name, const char *value)
{
	SXMPMeta *txmp = (SXMPMeta *)xmp;
	XMP_OptionBits options = 0;
	txmp->SetProperty(schema, name, value, options);
}


XmpStringPtr xmp_string_new()
{
	return (XmpStringPtr)new std::string;
}

void xmp_string_free(XmpStringPtr s)
{
	std::string *str = reinterpret_cast<std::string*>(s);
	delete str;
}

const char * xmp_string_cstr(XmpStringPtr s)
{
	return reinterpret_cast<std::string*>(s)->c_str();
}



XmpIteratorPtr xmp_iterator_new(XmpPtr xmp, const char * schema,
																const char * propName, uint32_t options)
{
	return (XmpIteratorPtr)new SXMPIterator(*(SXMPMeta*)xmp, schema, propName, options);
}


void xmp_iterator_free(XmpIteratorPtr iter)
{
	SXMPIterator *titer = (SXMPIterator*)iter;
	delete titer;
}

bool xmp_iterator_next(XmpIteratorPtr iter, XmpStringPtr schema,
											 XmpStringPtr propName, XmpStringPtr propValue,
											 uint32_t *options)
{
	SXMPIterator *titer = (SXMPIterator*)iter;
	return titer->Next(reinterpret_cast<std::string*>(schema),
										 reinterpret_cast<std::string*>(propName),
										 reinterpret_cast<std::string*>(propValue),
										 options);
}

void xmp_iterator_skip(XmpIteratorPtr iter, uint32_t options)
{
	SXMPIterator *titer = (SXMPIterator*)iter;
	titer->Skip(options);
}


#ifdef __cplusplus
}
#endif
