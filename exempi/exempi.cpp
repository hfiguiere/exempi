/*
 * exempi - exempi.cpp
 *
 * Copyright (C) 2007-2009 Hubert Figuiere
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
#define TXMP_STRING_TYPE std::string
#include "XMP.hpp"
#include "XMP.incl_cpp"


#if HAVE_NATIVE_TLS

static TLS int g_error = 0;

static void set_error(int err)
{
    g_error = err;
}

#else

#include <pthread.h>

/* Portable thread local storage using pthreads */
static pthread_key_t key = NULL;
static pthread_once_t key_once = PTHREAD_ONCE_INIT;

/* Destructor called when a thread exits - ensure to delete allocated int pointer */ 
static void destroy_tls_key( void * ptr )
{
	int* err_ptr = static_cast<int*>(ptr);
	delete err_ptr;
}

/* Create a key for use with pthreads local storage */ 
static void create_tls_key()
{
	(void) pthread_key_create(&key, destroy_tls_key);
}

/* Obtain the latest xmp error for this specific thread - defaults to 0 */
static int get_error_for_thread() 
{
	int * err_ptr;
	
	pthread_once(&key_once, create_tls_key);
	err_ptr = (int *) pthread_getspecific(key);
	
	if(err_ptr == NULL) {
		return 0;
	}
	 
	return *err_ptr;
}

/* set the current xmp error for this specific thread */
static void set_error(int err)
{
	int * err_ptr;
	
	// Ensure that create_thread_local_storage_key is only called
	// once, by the first thread, to create the key.
	pthread_once(&key_once, create_tls_key);
	 
	// Retrieve pointer to int for this thread.
	err_ptr = (int *) pthread_getspecific(key);
	
	// Allocate it, if it does not exists.
	if( err_ptr == NULL ) {
		err_ptr = new int;
		pthread_setspecific(key, err_ptr);
	}
	 
	// Save the error for this thread.
	*err_ptr = err;
}

#endif

static void set_error(const XMP_Error & e)
{
	set_error(-e.GetID());
	std::cerr << e.GetErrMsg() << std::endl;
}

#define RESET_ERROR set_error(0)

#define ASSIGN(dst, src) \
	dst.year = src.year; \
	dst.month = src.month;\
	dst.day = src.day; \
	dst.hour = src.hour; \
	dst.minute = src.minute; \
	dst.second = src.second; \
	dst.tzSign = src.tzSign; \
	dst.tzHour = src.tzHour; \
	dst.tzMinute = src.tzMinute; \
	dst.nanoSecond = src.nanoSecond;



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
const char NS_CAMERA_RAW_SETTINGS[] = kXMP_NS_CameraRaw;
const char NS_CAMERA_RAW_SAVED_SETTINGS[] = "http://ns.adobe.com/camera-raw-saved-settings/1.0/";
const char NS_PHOTOSHOP[] = kXMP_NS_Photoshop;
const char NS_IPTC4XMP[] = kXMP_NS_IPTCCore;
const char NS_TPG[] = kXMP_NS_XMP_PagedFile;
const char NS_DIMENSIONS_TYPE[] = kXMP_NS_XMP_Dimensions;
const char NS_CC[] = "http://creativecommons.org/ns#";
const char NS_PDF[] = kXMP_NS_PDF;

#define STRING(x) reinterpret_cast<std::string*>(x)

#define CHECK_PTR(p,r) \
	if(p == NULL) { set_error(XMPErr_BadObject); return r; }


int xmp_get_error()
{
#if HAVE_NATIVE_TLS
    return g_error;
#else
	return get_error_for_thread();
#endif
}

bool xmp_init()
{
	// no need to initialize anything else.
	return SXMPFiles::Initialize();
}


void xmp_terminate()
{
	SXMPFiles::Terminate();
}


bool xmp_register_namespace(const char *namespaceURI, 
							const char *suggestedPrefix,
							XmpStringPtr registeredPrefix)
{
	RESET_ERROR;
	try {
		return SXMPMeta::RegisterNamespace(namespaceURI, 
										   suggestedPrefix,
										   STRING(registeredPrefix));
	}
	catch(const XMP_Error & e) {
		set_error(e);
	}
	return false;
}


bool xmp_namespace_prefix(const char *ns, XmpStringPtr prefix)
{
	try {
		return SXMPMeta::GetNamespacePrefix(ns,
											STRING(prefix));
	}
	catch(const XMP_Error & e) {
		set_error(e);
	}
	return false;
}


bool xmp_prefix_namespace_uri(const char *prefix, XmpStringPtr ns)
{
	try {
		return SXMPMeta::GetNamespaceURI(prefix, STRING(ns));
	}
	catch(const XMP_Error & e) {
		set_error(e);
	}
	return false;
}


XmpFilePtr xmp_files_new()
{
	RESET_ERROR;
	SXMPFiles *txf = NULL;
	try {
		txf = new SXMPFiles();
	}
	catch(const XMP_Error & e) {
		set_error(e);
	}
	return (XmpFilePtr)txf;
}

XmpFilePtr xmp_files_open_new(const char *path, XmpOpenFileOptions options)
{
	CHECK_PTR(path, NULL);
	RESET_ERROR;
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
	CHECK_PTR(xf, false);
	RESET_ERROR;
	SXMPFiles *txf = (SXMPFiles*)xf;
	try {
		return txf->OpenFile(path, XMP_FT_UNKNOWN, options);
	}
	catch(const XMP_Error & e) {
		set_error(e);
	}
	return false;
}


bool xmp_files_close(XmpFilePtr xf, XmpCloseFileOptions options)
{
	CHECK_PTR(xf, false);
	RESET_ERROR;
	try {
		SXMPFiles *txf = (SXMPFiles*)xf;
		txf->CloseFile(options);
	}
	catch(const XMP_Error & e) {
		set_error(e);
		return false;
	}
	return true;
}


XmpPtr xmp_files_get_new_xmp(XmpFilePtr xf)
{
	CHECK_PTR(xf, NULL);
	RESET_ERROR;
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
	CHECK_PTR(xf, false);
	CHECK_PTR(xmp, false);
	RESET_ERROR;
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
	CHECK_PTR(xf, false);
	RESET_ERROR;
	SXMPFiles *txf = (SXMPFiles*)xf;
  bool result = false;

  try {
    result = txf->CanPutXMP(*(SXMPMeta*)xmp);
  }
	catch(const XMP_Error & e) {
		set_error(e);
		return false;
	}
  return result;
}


bool xmp_files_put_xmp(XmpFilePtr xf, XmpPtr xmp)
{
	CHECK_PTR(xf, false);
	CHECK_PTR(xmp, false);
	RESET_ERROR;
	SXMPFiles *txf = (SXMPFiles*)xf;
	
	try {
		txf->PutXMP(*(SXMPMeta*)xmp);
	}
	catch(const XMP_Error & e) {
		set_error(e);
		return false;
	}
	return true;
}


bool xmp_files_free(XmpFilePtr xf)
{
	CHECK_PTR(xf, false);
	RESET_ERROR;
	SXMPFiles *txf = (SXMPFiles*)xf;
	try {
		delete txf;
	}
	catch(const XMP_Error & e) {
		set_error(e);
		return false;
	}
	return true;
}


XmpPtr xmp_new_empty()
{
	RESET_ERROR;
	SXMPMeta *txmp = new SXMPMeta;
	return (XmpPtr)txmp;
}


XmpPtr xmp_new(const char *buffer, size_t len)
{
	CHECK_PTR(buffer, NULL);
	RESET_ERROR;

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
	CHECK_PTR(xmp, NULL);
	RESET_ERROR;

	SXMPMeta *txmp = new SXMPMeta(*(SXMPMeta*)xmp);
	return (XmpPtr)txmp;
}

bool xmp_parse(XmpPtr xmp, const char *buffer, size_t len)
{
	CHECK_PTR(xmp, false);
	CHECK_PTR(buffer, false);

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
	RESET_ERROR;
	return xmp_serialize_and_format(xmp, buffer, options, padding, 
									"\n", " ", 0);
}


bool xmp_serialize_and_format(XmpPtr xmp, XmpStringPtr buffer, 
							  uint32_t options, 
							  uint32_t padding, const char *newline, 
							  const char *tab, int32_t indent)
{
	CHECK_PTR(xmp, false);
	CHECK_PTR(buffer, false);
	RESET_ERROR;

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


bool xmp_free(XmpPtr xmp)
{
	CHECK_PTR(xmp, false);
	RESET_ERROR;
	SXMPMeta *txmp = (SXMPMeta *)xmp;
	delete txmp;
	return true;
}


bool xmp_get_property(XmpPtr xmp, const char *schema, 
					  const char *name, XmpStringPtr property,
					  uint32_t *propsBits)
{
	CHECK_PTR(xmp, false);
	RESET_ERROR;

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
	}
	return ret;
}

bool xmp_get_property_date(XmpPtr xmp, const char *schema, 
					  const char *name, XmpDateTime *property,
					  uint32_t *propsBits)
{
	CHECK_PTR(xmp, false);
	RESET_ERROR;

	bool ret = false;
	try {
		SXMPMeta *txmp = (SXMPMeta *)xmp;
		XMP_OptionBits optionBits;
		XMP_DateTime dt;
//		memset((void*)&dt, 1, sizeof(XMP_DateTime)); 
		ret = txmp->GetProperty_Date(schema, name, &dt, &optionBits);
		ASSIGN((*property), dt);
		if(propsBits) {
			*propsBits = optionBits;
		}
	}
	catch(const XMP_Error & e) {
		set_error(e);
	}
	return ret;
}

bool xmp_get_property_float(XmpPtr xmp, const char *schema, 
							const char *name, double * property,
							uint32_t *propsBits)
{
	CHECK_PTR(xmp, false);
	RESET_ERROR;

	bool ret = false;
	try {
		SXMPMeta *txmp = (SXMPMeta *)xmp;
		XMP_OptionBits optionBits;
		ret = txmp->GetProperty_Float(schema, name, property, &optionBits);
		if(propsBits) {
			*propsBits = optionBits;
		}
	}
	catch(const XMP_Error & e) {
		set_error(e);
	}
	return ret;
}

bool xmp_get_property_bool(XmpPtr xmp, const char *schema, 
							const char *name, bool * property,
							uint32_t *propsBits)
{
	CHECK_PTR(xmp, false);
	RESET_ERROR;

	bool ret = false;
	try {
		SXMPMeta *txmp = (SXMPMeta *)xmp;
		XMP_OptionBits optionBits;
		ret = txmp->GetProperty_Bool(schema, name, property, &optionBits);
		if(propsBits) {
			*propsBits = optionBits;
		}
	}
	catch(const XMP_Error & e) {
		set_error(e);
	}
	return ret;
}

bool xmp_get_property_int32(XmpPtr xmp, const char *schema, 
							const char *name, int32_t * property,
							uint32_t *propsBits)
{
	CHECK_PTR(xmp, false);
	RESET_ERROR;

	bool ret = false;
	try {
		SXMPMeta *txmp = (SXMPMeta *)xmp;
		XMP_OptionBits optionBits;
		// the long converstion is needed until XMPCore is fixed it use proper types.
		ret = txmp->GetProperty_Int(schema, name, property, &optionBits);
		if(propsBits) {
			*propsBits = optionBits;
		}
	}
	catch(const XMP_Error & e) {
		set_error(e);
	}
	return ret;
}

bool xmp_get_property_int64(XmpPtr xmp, const char *schema, 
							const char *name, int64_t * property,
							uint32_t *propsBits)
{
	CHECK_PTR(xmp, false);
	RESET_ERROR;

	bool ret = false;
	try {
		SXMPMeta *txmp = (SXMPMeta *)xmp;
		XMP_OptionBits optionBits;
		ret = txmp->GetProperty_Int64(schema, name, property, &optionBits);
		if(propsBits) {
			*propsBits = optionBits;
		}
	}
	catch(const XMP_Error & e) {
		set_error(e);
	}
	return ret;
}



bool xmp_get_array_item(XmpPtr xmp, const char *schema, 
												const char *name, int32_t index, XmpStringPtr property,
												uint32_t *propsBits)
{
	CHECK_PTR(xmp, false);
	RESET_ERROR;

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
	}
	return ret;
}

bool xmp_set_property(XmpPtr xmp, const char *schema, 
											const char *name, const char *value,
											uint32_t optionBits)
{
	CHECK_PTR(xmp, false);
	RESET_ERROR;

	bool ret = false;
	SXMPMeta *txmp = (SXMPMeta *)xmp;
	// see bug #16030
	// when it is a struct or an array, get prop return an empty string
	// but it fail if passed an empty string
	if ((optionBits & (XMP_PROP_VALUE_IS_STRUCT | XMP_PROP_VALUE_IS_ARRAY)) 
			&& (*value == 0)) {
		value = NULL;
	}
	try {
		txmp->SetProperty(schema, name, value, optionBits);
		ret = true;
	}
	catch(const XMP_Error & e) {
		set_error(e);
	}
	catch(...) {
	}
	return ret;
}


bool xmp_set_property_date(XmpPtr xmp, const char *schema, 
						   const char *name, const XmpDateTime *value,
						   uint32_t optionBits)
{
	CHECK_PTR(xmp, false);
	RESET_ERROR;

	bool ret = false;
	SXMPMeta *txmp = (SXMPMeta *)xmp;
	try {
		XMP_DateTime dt;
		ASSIGN(dt, (*value));
		txmp->SetProperty_Date(schema, name, dt, optionBits);
		ret = true;
	}
	catch(const XMP_Error & e) {
		set_error(e);
	}
	catch(...) {
	}
	return ret;
}

bool xmp_set_property_float(XmpPtr xmp, const char *schema, 
							const char *name, double value,
							uint32_t optionBits)
{
	CHECK_PTR(xmp, false);
	RESET_ERROR;

	bool ret = false;
	SXMPMeta *txmp = (SXMPMeta *)xmp;
	try {
		txmp->SetProperty_Float(schema, name, value, optionBits);
		ret = true;
	}
	catch(const XMP_Error & e) {
		set_error(e);
	}
	catch(...) {
	}
	return ret;
}


bool xmp_set_property_bool(XmpPtr xmp, const char *schema, 
						   const char *name, bool value,
						   uint32_t optionBits)
{
	CHECK_PTR(xmp, false);
	RESET_ERROR;

	bool ret = false;
	SXMPMeta *txmp = (SXMPMeta *)xmp;
	try {
		txmp->SetProperty_Bool(schema, name, value, optionBits);
		ret = true;
	}
	catch(const XMP_Error & e) {
		set_error(e);
	}
	catch(...) {
	}
	return ret;
}


bool xmp_set_property_int32(XmpPtr xmp, const char *schema, 
							const char *name, int32_t value,
							uint32_t optionBits)
{
	CHECK_PTR(xmp, false);
	RESET_ERROR;

	bool ret = false;
	SXMPMeta *txmp = (SXMPMeta *)xmp;
	try {
		txmp->SetProperty_Int(schema, name, value, optionBits);
		ret = true;
	}
	catch(const XMP_Error & e) {
		set_error(e);
	}
	catch(...) {
	}
	return ret;
}

bool xmp_set_property_int64(XmpPtr xmp, const char *schema, 
							const char *name, int64_t value,
							uint32_t optionBits)
{
	CHECK_PTR(xmp, false);
	RESET_ERROR;

	bool ret = false;
	SXMPMeta *txmp = (SXMPMeta *)xmp;
	try {
		txmp->SetProperty_Int64(schema, name, value, optionBits);
		ret = true;
	}
	catch(const XMP_Error & e) {
		set_error(e);
	}
	catch(...) {
	}
	return ret;
}


bool xmp_set_array_item(XmpPtr xmp, const char *schema, 
											 const char *name, int32_t index, const char *value,
											 uint32_t optionBits)
{
	CHECK_PTR(xmp, false);
	RESET_ERROR;

	bool ret = false;
	SXMPMeta *txmp = (SXMPMeta *)xmp;
	try {
		txmp->SetArrayItem(schema, name, index, value, optionBits);
		ret = true;
	}
	catch(const XMP_Error & e) {
		set_error(e);
	}
	catch(...) {
	}
	return ret;
}

bool xmp_append_array_item(XmpPtr xmp, const char *schema, const char *name,
			   uint32_t arrayOptions, const char *value,
			   uint32_t optionBits)
{
	CHECK_PTR(xmp, false);
	RESET_ERROR;

	bool ret = false;
	SXMPMeta *txmp = (SXMPMeta *)xmp;
	try {
		txmp->AppendArrayItem(schema, name, arrayOptions, value,
				      optionBits);
		ret = true;
	}
	catch(const XMP_Error & e) {
		set_error(e);
	}
	catch(...) {
	}
	return ret;
}

bool xmp_delete_property(XmpPtr xmp, const char *schema, const char *name)
{
	CHECK_PTR(xmp, false);
	RESET_ERROR;

	bool ret = true;
	SXMPMeta *txmp = (SXMPMeta *)xmp;
	try {
		txmp->DeleteProperty(schema, name);
	}
	catch(const XMP_Error & e) {
		set_error(e);
		ret = false;
	}
	catch(...) {
		ret = false;
	}
	return ret;
}

bool xmp_has_property(XmpPtr xmp, const char *schema, const char *name)
{
	CHECK_PTR(xmp, false);
	RESET_ERROR;

	bool ret = true;
	SXMPMeta *txmp = (SXMPMeta *)xmp;
	try {
		ret = txmp->DoesPropertyExist(schema, name);
	}
	catch(const XMP_Error & e) {
		set_error(e);
		ret = false;
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
	CHECK_PTR(xmp, false);
	RESET_ERROR;

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
	CHECK_PTR(xmp, false);
	RESET_ERROR;

	bool ret = true;
	SXMPMeta *txmp = (SXMPMeta *)xmp;
	try {
		txmp->SetLocalizedText(schema, name, genericLang, specificLang,
				       value, optionBits);
	}
	catch(const XMP_Error & e) {
		set_error(e);
		ret = false;
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
	CHECK_PTR(xmp, false);
	RESET_ERROR;

	bool ret = true;
	SXMPMeta *txmp = (SXMPMeta *)xmp;
	try {
		txmp->DeleteLocalizedText(schema, name, genericLang,
															specificLang);
	}
	catch(const XMP_Error & e) {
		set_error(e);
		ret = false;
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
	CHECK_PTR(xmp, NULL);
	RESET_ERROR;
	return (XmpIteratorPtr)new SXMPIterator(*(SXMPMeta*)xmp, schema, propName, options);
}


bool xmp_iterator_free(XmpIteratorPtr iter)
{
	CHECK_PTR(iter, false);
	RESET_ERROR;
	SXMPIterator *titer = (SXMPIterator*)iter;
	delete titer;
	return true;
}

bool xmp_iterator_next(XmpIteratorPtr iter, XmpStringPtr schema,
					   XmpStringPtr propName, XmpStringPtr propValue,
					   uint32_t *options)
{
	CHECK_PTR(iter, false);
	RESET_ERROR;
	SXMPIterator *titer = (SXMPIterator*)iter;
	return titer->Next(reinterpret_cast<std::string*>(schema),
										 reinterpret_cast<std::string*>(propName),
										 reinterpret_cast<std::string*>(propValue),
										 options);
}

bool xmp_iterator_skip(XmpIteratorPtr iter, XmpIterSkipOptions options)
{
	CHECK_PTR(iter, false);
	RESET_ERROR;
	SXMPIterator *titer = (SXMPIterator*)iter;
	titer->Skip(options);
	return true;
}


#ifdef __cplusplus
}
#endif
