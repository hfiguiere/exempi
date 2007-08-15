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

#include "xmpconsts.h"
#include "xmp.h"
#include "xmperrors.h"

#include <string>
#include <iostream>

#define XMP_INCLUDE_XMPFILES 1
#define UNIX_ENV 1
#define TXMP_STRING_TYPE std::string
#include "XMP.hpp"
#include "XMP.incl_cpp"

int static g_error = 0;

static void set_error(int err)
{
	g_error = err;
}

static void set_error(const XMP_Error & e)
{
	set_error(-e.GetID());
	std::cerr << e.GetErrMsg() << std::endl;
}

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
const char NS_CC[] = "http://creativecommons.org/ns#";

#define STRING(x) reinterpret_cast<std::string*>(x)



int xmp_get_error()
{
	return g_error;
}

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


bool xmp_register_namespace(const char *namespaceURI, 
														const char *suggestedPrefix,
														XmpStringPtr registeredPrefix)
{
    return SXMPMeta::RegisterNamespace(namespaceURI, 
																			 suggestedPrefix,
																			 STRING(registeredPrefix));
}


XmpFilePtr xmp_files_new()
{
	SXMPFiles *txf = new SXMPFiles();
	return (XmpFilePtr)txf;
}

XmpFilePtr xmp_files_open_new(const char *path, XmpOpenFileOptions options)
{
	SXMPFiles *txf = NULL;
	try {
		txf = new SXMPFiles(path, XMP_FT_UNKNOWN, options);
	}
	catch(const XMP_Error & e) {
		set_error(e);
	}

	return (XmpFilePtr)txf;
}


bool xmp_files_open(XmpFilePtr xf, const char *path, XmpOpenFileOptions options)
{
	SXMPFiles *txf = (SXMPFiles*)xf;
	try {
		return txf->OpenFile(path, XMP_FT_UNKNOWN, options);
	}
	catch(const XMP_Error & e) {
		set_error(e);
	}
	return false;
}


void xmp_files_close(XmpFilePtr xf, XmpCloseFileOptions options)
{
	SXMPFiles *txf = (SXMPFiles*)xf;
	txf->CloseFile(options);
}


XmpPtr xmp_files_get_new_xmp(XmpFilePtr xf)
{
	SXMPMeta *xmp = new SXMPMeta();
	SXMPFiles *txf = (SXMPFiles*)xf;

	bool result = false;
	try {
		result = txf->GetXMP(xmp);
		if(!result) {
			delete xmp;
			return NULL;
		}
	}
	catch(const XMP_Error & e) {
		set_error(e);
	}
	return (XmpPtr)xmp;
}


bool xmp_files_get_xmp(XmpFilePtr xf, XmpPtr xmp)
{
	bool result = false;
	try {
		SXMPFiles *txf = (SXMPFiles*)xf;
		
		result = txf->GetXMP((SXMPMeta*)xmp);
	} 
	catch(const XMP_Error & e) {
		set_error(e);
	}
	return result;
}


bool xmp_files_can_put_xmp(XmpFilePtr xf, XmpPtr xmp)
{
	SXMPFiles *txf = (SXMPFiles*)xf;

	return txf->CanPutXMP(*(SXMPMeta*)xmp);
}


void xmp_files_put_xmp(XmpFilePtr xf, XmpPtr xmp)
{
	SXMPFiles *txf = (SXMPFiles*)xf;
	
	try {
		txf->PutXMP(*(SXMPMeta*)xmp);
	}
	catch(const XMP_Error & e) {
		set_error(e);
	}
}


void xmp_files_free(XmpFilePtr xf)
{
	SXMPFiles *txf = (SXMPFiles*)xf;
	try {
		delete txf;
	}
	catch(const XMP_Error & e) {
		set_error(e);
	}
}


XmpPtr xmp_new_empty()
{
	SXMPMeta *txmp = new SXMPMeta;
	return (XmpPtr)txmp;
}


XmpPtr xmp_new(const char *buffer, size_t len)
{
	SXMPMeta *txmp;
	try {
		txmp = new SXMPMeta(buffer, len);
	}
	catch(const XMP_Error & e) {
		set_error(e);
		txmp = 0;
	}
	return (XmpPtr)txmp;
}


XmpPtr xmp_copy(XmpPtr xmp)
{
	if(xmp == 0) {
		return 0;
	}
	SXMPMeta *txmp = new SXMPMeta(*(SXMPMeta*)xmp);
	return (XmpPtr)txmp;
}

bool xmp_parse(XmpPtr xmp, const char *buffer, size_t len)
{
	SXMPMeta *txmp = (SXMPMeta *)xmp;
	try {
		txmp->ParseFromBuffer(buffer, len, kXMP_RequireXMPMeta );
	}
	catch(const XMP_Error & e)
	{
		set_error(e);
		return false;
	}
	return true;
}



bool xmp_serialize(XmpPtr xmp, XmpStringPtr buffer, uint32_t options, 
									 uint32_t padding)
{
	return xmp_serialize_and_format(xmp, buffer, options, padding, 
																	"\n", " ", 0);
}


bool xmp_serialize_and_format(XmpPtr xmp, XmpStringPtr buffer, 
															uint32_t options, 
															uint32_t padding, const char *newline, 
															const char *tab, int32_t indent)
{
	SXMPMeta *txmp = (SXMPMeta *)xmp;
	try {
		txmp->SerializeToBuffer(STRING(buffer), options, padding,
														newline, tab, indent);
	}
	catch(const XMP_Error & e)
	{
		set_error(e);
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
	return xmp_get_property_and_bits(xmp, schema, name, property, NULL);
}


bool xmp_get_property_and_bits(XmpPtr xmp, const char *schema, 
															 const char *name, XmpStringPtr property,
															 uint32_t *propsBits)
{
	bool ret = false;
	try {
		SXMPMeta *txmp = (SXMPMeta *)xmp;
		XMP_OptionBits optionBits;
		ret = txmp->GetProperty(schema, name, STRING(property), 
																 &optionBits);
		if(propsBits) {
			*propsBits = optionBits;
		}
	}
	catch(const XMP_Error & e) {
		set_error(e);
		ret = false;
	}
	return ret;
}


bool xmp_get_array_item(XmpPtr xmp, const char *schema, 
												const char *name, int32_t index, XmpStringPtr property,
												uint32_t *propsBits)
{
	bool ret = false;
	try {
		SXMPMeta *txmp = (SXMPMeta *)xmp;
		XMP_OptionBits optionBits;
		ret = txmp->GetArrayItem(schema, name, index, STRING(property), 
														 &optionBits);
		if(propsBits) {
			*propsBits = optionBits;
		}
	}
	catch(const XMP_Error & e) {
		set_error(e);
		ret = false;
	}
	return ret;
}

void xmp_set_property(XmpPtr xmp, const char *schema, 
											const char *name, const char *value)
{
	xmp_set_property2(xmp, schema, name, value, 0);
}



bool xmp_set_property2(XmpPtr xmp, const char *schema, 
											const char *name, const char *value,
											uint32_t optionBits)
{
	bool ret = true;
	SXMPMeta *txmp = (SXMPMeta *)xmp;
	try {
		txmp->SetProperty(schema, name, value, optionBits);
	}
	catch(const XMP_Error & e) {
		set_error(-e.GetID());
		ret = false;
		std::cerr << e.GetErrMsg() << std::endl;
	}
	catch(...) {
		ret = false;		
	}
	return ret;
}


bool xmp_set_array_item(XmpPtr xmp, const char *schema, 
											 const char *name, int32_t index, const char *value,
											 uint32_t optionBits)
{
	bool ret = true;
	SXMPMeta *txmp = (SXMPMeta *)xmp;
	try {
		txmp->SetArrayItem(schema, name, index, value, optionBits);
	}
	catch(const XMP_Error & e) {
		set_error(-e.GetID());
		ret = false;
		std::cerr << e.GetErrMsg() << std::endl;
	}
	catch(...) {
		ret = false;		
	}
	return ret;
}

bool xmp_append_array_item(XmpPtr xmp, const char *schema, const char *name,
			   uint32_t arrayOptions, const char *value,
			   uint32_t optionBits)
{
	bool ret = true;
	SXMPMeta *txmp = (SXMPMeta *)xmp;
	try {
		txmp->AppendArrayItem(schema, name, arrayOptions, value,
				      optionBits);
	}
	catch(const XMP_Error & e) {
		set_error(-e.GetID());
		ret = false;
		std::cerr << e.GetErrMsg() << std::endl;
	}
	catch(...) {
		ret = false;
	}
	return ret;
}

bool xmp_delete_property(XmpPtr xmp, const char *schema, const char *name)
{
	bool ret = true;
	SXMPMeta *txmp = (SXMPMeta *)xmp;
	try {
		txmp->DeleteProperty(schema, name);
	}
	catch(const XMP_Error & e) {
		set_error(-e.GetID());
		ret = false;
		std::cerr << e.GetErrMsg() << std::endl;
	}
	catch(...) {
		ret = false;
	}
	return ret;
}

bool xmp_has_property(XmpPtr xmp, const char *schema, const char *name)
{
	bool ret = true;
	SXMPMeta *txmp = (SXMPMeta *)xmp;
	try {
		ret = txmp->DoesPropertyExist(schema, name);
	}
	catch(const XMP_Error & e) {
		set_error(-e.GetID());
		ret = false;
		std::cerr << e.GetErrMsg() << std::endl;
	}
	catch(...) {
		ret = false;
	}
	return ret;
}

bool xmp_get_localized_text(XmpPtr xmp, const char *schema, const char *name,
			    const char *genericLang, const char *specificLang,
			    XmpStringPtr actualLang, XmpStringPtr itemValue,
			    uint32_t *propsBits)
{
	bool ret = false;
	try {
		SXMPMeta *txmp = (SXMPMeta *)xmp;
		XMP_OptionBits optionBits;
		ret = txmp->GetLocalizedText(schema, name, genericLang,
					     specificLang, STRING(actualLang), 
					     STRING(itemValue), &optionBits);
		if(propsBits) {
			*propsBits = optionBits;
		}
	}
	catch(const XMP_Error & e) {
		set_error(e);
		ret = false;
	}
	return ret;
}

bool xmp_set_localized_text(XmpPtr xmp, const char *schema, const char *name,
			    const char *genericLang, const char *specificLang,
			    const char *value, uint32_t optionBits)
{
	bool ret = true;
	SXMPMeta *txmp = (SXMPMeta *)xmp;
	try {
		txmp->SetLocalizedText(schema, name, genericLang, specificLang,
				       value, optionBits);
	}
	catch(const XMP_Error & e) {
		set_error(-e.GetID());
		ret = false;
		std::cerr << e.GetErrMsg() << std::endl;
	}
	catch(...) {
		ret = false;		
	}
	return ret;
}


bool xmp_delete_localized_text(XmpPtr xmp, const char *schema,
															 const char *name, const char *genericLang,
															 const char *specificLang)
{
	bool ret = true;
	SXMPMeta *txmp = (SXMPMeta *)xmp;
	try {
		txmp->DeleteLocalizedText(schema, name, genericLang,
															specificLang);
	}
	catch(const XMP_Error & e) {
		set_error(-e.GetID());
		ret = false;
		std::cerr << e.GetErrMsg() << std::endl;
	}
	catch(...) {
		ret = false;
	}
	return ret;
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
																const char * propName, XmpIterOptions options)
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

void xmp_iterator_skip(XmpIteratorPtr iter, XmpIterSkipOptions options)
{
	SXMPIterator *titer = (SXMPIterator*)iter;
	titer->Skip(options);
}


#ifdef __cplusplus
}
#endif
