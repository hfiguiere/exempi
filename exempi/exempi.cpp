/*
 * exempi - exempi.cpp
 *
 * Copyright (C) 2007-2016 Hubert Figuière
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
#include <memory>

#define XMP_INCLUDE_XMPFILES 1
#define TXMP_STRING_TYPE std::string
#include "XMP.hpp"
#include "XMP.incl_cpp"
#include "XMPUtils.hpp"

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

/* Destructor called when a thread exits - ensure to delete allocated int
 * pointer */
static void destroy_tls_key(void *ptr)
{
    int *err_ptr = static_cast<int *>(ptr);
    delete err_ptr;
}

/* Create a key for use with pthreads local storage */
static void create_tls_key()
{
    (void)pthread_key_create(&key, destroy_tls_key);
}

/* Obtain the latest xmp error for this specific thread - defaults to 0 */
static int get_error_for_thread()
{
    int *err_ptr;

    pthread_once(&key_once, create_tls_key);
    err_ptr = (int *)pthread_getspecific(key);

    if (err_ptr == NULL) {
        return 0;
    }

    return *err_ptr;
}

/* set the current xmp error for this specific thread */
static void set_error(int err)
{
    int *err_ptr;

    // Ensure that create_thread_local_storage_key is only called
    // once, by the first thread, to create the key.
    pthread_once(&key_once, create_tls_key);

    // Retrieve pointer to int for this thread.
    err_ptr = (int *)pthread_getspecific(key);

    // Allocate it, if it does not exists.
    if (err_ptr == NULL) {
        err_ptr = new int;
        pthread_setspecific(key, err_ptr);
    }

    // Save the error for this thread.
    *err_ptr = err;
}

#endif

static void set_error(const XMP_Error &e)
{
    set_error(-e.GetID());
    std::cerr << e.GetErrMsg() << std::endl;
}

#define RESET_ERROR set_error(0)

// Hack: assign between XMP_DateTime and XmpDateTime
#define ASSIGN(dst, src)                                                       \
    (dst).year = (src).year;                                                   \
    (dst).month = (src).month;                                                 \
    (dst).day = (src).day;                                                     \
    (dst).hour = (src).hour;                                                   \
    (dst).minute = (src).minute;                                               \
    (dst).second = (src).second;                                               \
    (dst).tzSign = (src).tzSign;                                               \
    (dst).tzHour = (src).tzHour;                                               \
    (dst).tzMinute = (src).tzMinute;                                           \
    (dst).nanoSecond = (src).nanoSecond;

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
const char NS_CAMERA_RAW_SAVED_SETTINGS[] =
    "http://ns.adobe.com/camera-raw-saved-settings/1.0/";
const char NS_PHOTOSHOP[] = kXMP_NS_Photoshop;
const char NS_IPTC4XMP[] = kXMP_NS_IPTCCore;
const char NS_TPG[] = kXMP_NS_XMP_PagedFile;
const char NS_DIMENSIONS_TYPE[] = kXMP_NS_XMP_Dimensions;
const char NS_CC[] = "http://creativecommons.org/ns#";
const char NS_PDF[] = kXMP_NS_PDF;

#define STRING(x) reinterpret_cast<std::string *>(x)

#define CHECK_PTR(p, r)                                                        \
    if (p == NULL) {                                                           \
        set_error(XMPErr_BadObject);                                           \
        return r;                                                              \
    }

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
    RESET_ERROR;
    try {
        // no need to initialize anything else.
        // XMP SDK 5.1.2 needs this because it has been lobotomized of local
        // text
        // conversion
        // the one that was done in Exempi with libiconv.
        return SXMPFiles::Initialize(kXMPFiles_IgnoreLocalText);
    }
    catch (const XMP_Error &e) {
        set_error(e);
    }
    return false;
}

void xmp_terminate()
{
    RESET_ERROR;
    SXMPFiles::Terminate();
}

bool xmp_register_namespace(const char *namespaceURI,
                            const char *suggestedPrefix,
                            XmpStringPtr registeredPrefix)
{
    RESET_ERROR;
    try {
        return SXMPMeta::RegisterNamespace(namespaceURI, suggestedPrefix,
                                           STRING(registeredPrefix));
    }
    catch (const XMP_Error &e) {
        set_error(e);
    }
    return false;
}

bool xmp_namespace_prefix(const char *ns, XmpStringPtr prefix)
{
    CHECK_PTR(ns, false);
    RESET_ERROR;
    try {
        return SXMPMeta::GetNamespacePrefix(ns, STRING(prefix));
    }
    catch (const XMP_Error &e) {
        set_error(e);
    }
    return false;
}

bool xmp_prefix_namespace_uri(const char *prefix, XmpStringPtr ns)
{
    CHECK_PTR(prefix, false);
    RESET_ERROR;
    try {
        return SXMPMeta::GetNamespaceURI(prefix, STRING(ns));
    }
    catch (const XMP_Error &e) {
        set_error(e);
    }
    return false;
}

XmpFilePtr xmp_files_new()
{
    RESET_ERROR;

    try {
        auto txf = std::unique_ptr<SXMPFiles>(new SXMPFiles());

        return reinterpret_cast<XmpFilePtr>(txf.release());
    }
    catch (const XMP_Error &e) {
        set_error(e);
    }
    return NULL;
}

XmpFilePtr xmp_files_open_new(const char *path, XmpOpenFileOptions options)
{
    CHECK_PTR(path, NULL);
    RESET_ERROR;

    try {
        auto txf = std::unique_ptr<SXMPFiles>(new SXMPFiles);

        txf->OpenFile(path, XMP_FT_UNKNOWN, options);

        return reinterpret_cast<XmpFilePtr>(txf.release());
    }
    catch (const XMP_Error &e) {
        set_error(e);
    }

    return NULL;
}

bool xmp_files_open(XmpFilePtr xf, const char *path, XmpOpenFileOptions options)
{
    CHECK_PTR(xf, false);
    RESET_ERROR;
    auto txf = reinterpret_cast<SXMPFiles *>(xf);
    try {
        return txf->OpenFile(path, XMP_FT_UNKNOWN, options);
    }
    catch (const XMP_Error &e) {
        set_error(e);
    }
    return false;
}

bool xmp_files_close(XmpFilePtr xf, XmpCloseFileOptions options)
{
    CHECK_PTR(xf, false);
    RESET_ERROR;
    try {
        auto txf = reinterpret_cast<SXMPFiles *>(xf);
        txf->CloseFile(options);
    }
    catch (const XMP_Error &e) {
        set_error(e);
        return false;
    }
    return true;
}

XmpPtr xmp_files_get_new_xmp(XmpFilePtr xf)
{
    CHECK_PTR(xf, NULL);
    RESET_ERROR;
    auto txf = reinterpret_cast<SXMPFiles *>(xf);

    bool result = false;
    try {
        auto xmp = std::unique_ptr<SXMPMeta>(new SXMPMeta);
        result = txf->GetXMP(xmp.get());
        if (result) {
            return reinterpret_cast<XmpPtr>(xmp.release());
        }
    }
    catch (const XMP_Error &e) {
        set_error(e);
    }
    return NULL;
}

bool xmp_files_get_xmp(XmpFilePtr xf, XmpPtr xmp)
{
    CHECK_PTR(xf, false);
    CHECK_PTR(xmp, false);
    RESET_ERROR;
    bool result = false;
    try {
        auto txf = reinterpret_cast<SXMPFiles *>(xf);

        result = txf->GetXMP(reinterpret_cast<SXMPMeta *>(xmp));
    }
    catch (const XMP_Error &e) {
        set_error(e);
    }
    return result;
}

bool xmp_files_get_xmp_xmpstring(XmpFilePtr xf, XmpStringPtr xmp_packet,
                                 XmpPacketInfo* packet_info)
{
  CHECK_PTR(xf, false);
  CHECK_PTR(xmp_packet, false);
  RESET_ERROR;
  bool result = false;
  try {
    SXMPFiles *txf = reinterpret_cast<SXMPFiles*>(xf);

    XMP_PacketInfo xmp_packet_info;
    result = txf->GetXMP(NULL,
                         reinterpret_cast<std::string*>(xmp_packet),
                         &xmp_packet_info);
    if (packet_info) {
      packet_info->offset = xmp_packet_info.offset;
      packet_info->length = xmp_packet_info.length;
      packet_info->padSize = xmp_packet_info.padSize;
      packet_info->charForm = xmp_packet_info.charForm;
      packet_info->writeable = xmp_packet_info.writeable;
      packet_info->hasWrapper = xmp_packet_info.hasWrapper;
      packet_info->pad = xmp_packet_info.pad;
    }
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
    auto txf = reinterpret_cast<SXMPFiles *>(xf);
    bool result = false;

    try {
        result = txf->CanPutXMP(*reinterpret_cast<const SXMPMeta *>(xmp));
    }
    catch (const XMP_Error &e) {
        set_error(e);
        return false;
    }
    return result;
}

bool xmp_files_can_put_xmp_xmpstring(XmpFilePtr xf, XmpStringPtr xmp_packet)
{
  CHECK_PTR(xf, false);
  RESET_ERROR;
  SXMPFiles *txf = reinterpret_cast<SXMPFiles*>(xf);
  bool result = false;

  try {
    result = txf->CanPutXMP(*reinterpret_cast<const std::string*>(xmp_packet));
  }
  catch(const XMP_Error & e) {
    set_error(e);
    return false;
  }
  return result;
}

bool xmp_files_can_put_xmp_cstr(XmpFilePtr xf, const char* xmp_packet, size_t len)
{
  CHECK_PTR(xf, false);
  RESET_ERROR;
  SXMPFiles *txf = reinterpret_cast<SXMPFiles*>(xf);
  bool result = false;

  try {
    result = txf->CanPutXMP(xmp_packet, len);
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
    auto txf = reinterpret_cast<SXMPFiles *>(xf);

    try {
        txf->PutXMP(*reinterpret_cast<const SXMPMeta *>(xmp));
    }
    catch (const XMP_Error &e) {
        set_error(e);
        return false;
    }
    return true;
}

bool xmp_files_put_xmp_xmpstring(XmpFilePtr xf, XmpStringPtr xmp_packet)
{
  CHECK_PTR(xf, false);
  CHECK_PTR(xmp_packet, false);
  RESET_ERROR;
  SXMPFiles *txf = reinterpret_cast<SXMPFiles*>(xf);

  try {
    txf->PutXMP(*reinterpret_cast<const std::string*>(xmp_packet));
  }
  catch(const XMP_Error & e) {
    set_error(e);
    return false;
  }
  return true;
}

bool xmp_files_put_xmp_cstr(XmpFilePtr xf, const char* xmp_packet, size_t len)
{
  CHECK_PTR(xf, false);
  CHECK_PTR(xmp_packet, false);
  RESET_ERROR;
  SXMPFiles *txf = reinterpret_cast<SXMPFiles*>(xf);

  try {
    txf->PutXMP(xmp_packet, len);
  }
  catch(const XMP_Error & e) {
    set_error(e);
    return false;
  }
  return true;
}

bool xmp_files_get_file_info(XmpFilePtr xf, XmpStringPtr filePath,
                             XmpOpenFileOptions *options,
                             XmpFileType *file_format,
                             XmpFileFormatOptions *handler_flags)
{
    CHECK_PTR(xf, false);
    RESET_ERROR;

    bool result = false;
    auto txf = reinterpret_cast<SXMPFiles *>(xf);
    try {
        result = txf->GetFileInfo(STRING(filePath), (XMP_OptionBits *)options,
                                  (XMP_FileFormat *)file_format,
                                  (XMP_OptionBits *)handler_flags);
    }
    catch (const XMP_Error &e) {
        set_error(e);
        return false;
    }

    return result;
}

bool xmp_files_free(XmpFilePtr xf)
{
    CHECK_PTR(xf, false);
    RESET_ERROR;
    auto txf = reinterpret_cast<SXMPFiles *>(xf);
    try {
        delete txf;
    }
    catch (const XMP_Error &e) {
        set_error(e);
        return false;
    }
    return true;
}

bool xmp_files_get_format_info(XmpFileType format,
                               XmpFileFormatOptions *options)
{
    RESET_ERROR;

    bool result = false;
    try {
        result = SXMPFiles::GetFormatInfo(format, (XMP_OptionBits *)options);
    }
    catch (const XMP_Error &e) {
        set_error(e);
        return false;
    }
    return result;
}

XmpFileType xmp_files_check_file_format(const char *filePath)
{
    CHECK_PTR(filePath, XMP_FT_UNKNOWN);
    RESET_ERROR;

    XmpFileType file_type = XMP_FT_UNKNOWN;
    try {
        file_type = (XmpFileType)SXMPFiles::CheckFileFormat(filePath);
    }
    catch (const XMP_Error &e) {
        set_error(e);
        return XMP_FT_UNKNOWN;
    }
    return file_type;
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

    try {
        auto txmp = std::unique_ptr<SXMPMeta>(new SXMPMeta(buffer, len));
        return reinterpret_cast<XmpPtr>(txmp.release());
    }
    catch (const XMP_Error &e) {
        set_error(e);
    }
    return NULL;
}

XmpPtr xmp_copy(XmpPtr xmp)
{
    CHECK_PTR(xmp, NULL);
    RESET_ERROR;

    try {
        auto txmp = std::unique_ptr<SXMPMeta>(new SXMPMeta(*(SXMPMeta *)xmp));
        return reinterpret_cast<XmpPtr>(txmp.release());
    }
    catch (const XMP_Error &e) {
        set_error(e);
    }
    return NULL;
}

bool xmp_parse(XmpPtr xmp, const char *buffer, size_t len)
{
    CHECK_PTR(xmp, false);
    CHECK_PTR(buffer, false);

    SXMPMeta *txmp = (SXMPMeta *)xmp;
    try {
        txmp->ParseFromBuffer(buffer, len, kXMP_RequireXMPMeta);
    }
    catch (const XMP_Error &e) {
        set_error(e);
        return false;
    }
    return true;
}

bool xmp_serialize(XmpPtr xmp, XmpStringPtr buffer, uint32_t options,
                   uint32_t padding)
{
    RESET_ERROR;
    return xmp_serialize_and_format(xmp, buffer, options, padding, "\n", " ",
                                    0);
}

bool xmp_serialize_and_format(XmpPtr xmp, XmpStringPtr buffer, uint32_t options,
                              uint32_t padding, const char *newline,
                              const char *tab, int32_t indent)
{
    CHECK_PTR(xmp, false);
    CHECK_PTR(buffer, false);
    RESET_ERROR;

    SXMPMeta *txmp = (SXMPMeta *)xmp;
    try {
        txmp->SerializeToBuffer(STRING(buffer), options, padding, newline, tab,
                                indent);
    }
    catch (const XMP_Error &e) {
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

bool xmp_get_property(XmpPtr xmp, const char *schema, const char *name,
                      XmpStringPtr property, uint32_t *propsBits)
{
    CHECK_PTR(xmp, false);
    RESET_ERROR;

    bool ret = false;
    try {
        auto txmp = reinterpret_cast<const SXMPMeta *>(xmp);
        XMP_OptionBits optionBits;
        ret = txmp->GetProperty(schema, name, STRING(property), &optionBits);
        if (propsBits) {
            *propsBits = optionBits;
        }
    }
    catch (const XMP_Error &e) {
        set_error(e);
    }
    return ret;
}

bool xmp_get_property_date(XmpPtr xmp, const char *schema, const char *name,
                           XmpDateTime *property, uint32_t *propsBits)
{
    CHECK_PTR(xmp, false);
    RESET_ERROR;

    bool ret = false;
    try {
        auto txmp = reinterpret_cast<const SXMPMeta *>(xmp);
        XMP_OptionBits optionBits;
        XMP_DateTime dt;
        //		memset((void*)&dt, 1, sizeof(XMP_DateTime));
        ret = txmp->GetProperty_Date(schema, name, &dt, &optionBits);
        ASSIGN((*property), dt);
        if (propsBits) {
            *propsBits = optionBits;
        }
    }
    catch (const XMP_Error &e) {
        set_error(e);
    }
    return ret;
}

bool xmp_get_property_float(XmpPtr xmp, const char *schema, const char *name,
                            double *property, uint32_t *propsBits)
{
    CHECK_PTR(xmp, false);
    RESET_ERROR;

    bool ret = false;
    try {
        auto txmp = reinterpret_cast<const SXMPMeta *>(xmp);
        XMP_OptionBits optionBits;
        ret = txmp->GetProperty_Float(schema, name, property, &optionBits);
        if (propsBits) {
            *propsBits = optionBits;
        }
    }
    catch (const XMP_Error &e) {
        set_error(e);
    }
    return ret;
}

bool xmp_get_property_bool(XmpPtr xmp, const char *schema, const char *name,
                           bool *property, uint32_t *propsBits)
{
    CHECK_PTR(xmp, false);
    RESET_ERROR;

    bool ret = false;
    try {
        auto txmp = reinterpret_cast<const SXMPMeta *>(xmp);
        XMP_OptionBits optionBits;
        ret = txmp->GetProperty_Bool(schema, name, property, &optionBits);
        if (propsBits) {
            *propsBits = optionBits;
        }
    }
    catch (const XMP_Error &e) {
        set_error(e);
    }
    return ret;
}

bool xmp_get_property_int32(XmpPtr xmp, const char *schema, const char *name,
                            int32_t *property, uint32_t *propsBits)
{
    CHECK_PTR(xmp, false);
    RESET_ERROR;

    bool ret = false;
    try {
        auto txmp = reinterpret_cast<const SXMPMeta *>(xmp);
        XMP_OptionBits optionBits;
        // the long converstion is needed until XMPCore is fixed it use proper
        // types.
        ret = txmp->GetProperty_Int(schema, name, property, &optionBits);
        if (propsBits) {
            *propsBits = optionBits;
        }
    }
    catch (const XMP_Error &e) {
        set_error(e);
    }
    return ret;
}

bool xmp_get_property_int64(XmpPtr xmp, const char *schema, const char *name,
                            int64_t *property, uint32_t *propsBits)
{
    CHECK_PTR(xmp, false);
    RESET_ERROR;

    bool ret = false;
    try {
        auto txmp = reinterpret_cast<const SXMPMeta *>(xmp);
        XMP_OptionBits optionBits;
        ret = txmp->GetProperty_Int64(schema, name, property, &optionBits);
        if (propsBits) {
            *propsBits = optionBits;
        }
    }
    catch (const XMP_Error &e) {
        set_error(e);
    }
    return ret;
}

bool xmp_get_array_item(XmpPtr xmp, const char *schema, const char *name,
                        int32_t index, XmpStringPtr property,
                        uint32_t *propsBits)
{
    CHECK_PTR(xmp, false);
    RESET_ERROR;

    bool ret = false;
    try {
        auto txmp = reinterpret_cast<const SXMPMeta *>(xmp);
        XMP_OptionBits optionBits;
        ret = txmp->GetArrayItem(schema, name, index, STRING(property),
                                 &optionBits);
        if (propsBits) {
            *propsBits = optionBits;
        }
    }
    catch (const XMP_Error &e) {
        set_error(e);
    }
    return ret;
}

bool xmp_set_property(XmpPtr xmp, const char *schema, const char *name,
                      const char *value, uint32_t optionBits)
{
    CHECK_PTR(xmp, false);
    RESET_ERROR;

    bool ret = false;
    auto txmp = reinterpret_cast<SXMPMeta *>(xmp);
    // see bug #16030
    // when it is a struct or an array, get prop return an empty string
    // but it fail if passed an empty string
    if ((optionBits & (XMP_PROP_VALUE_IS_STRUCT | XMP_PROP_VALUE_IS_ARRAY)) &&
        (*value == 0)) {
        value = NULL;
    }
    try {
        txmp->SetProperty(schema, name, value, optionBits);
        ret = true;
    }
    catch (const XMP_Error &e) {
        set_error(e);
    }
    catch (...) {
    }
    return ret;
}

bool xmp_set_property_date(XmpPtr xmp, const char *schema, const char *name,
                           const XmpDateTime *value, uint32_t optionBits)
{
    CHECK_PTR(xmp, false);
    RESET_ERROR;

    bool ret = false;
    auto txmp = reinterpret_cast<SXMPMeta *>(xmp);
    try {
        XMP_DateTime dt;
        ASSIGN(dt, (*value));
        txmp->SetProperty_Date(schema, name, dt, optionBits);
        ret = true;
    }
    catch (const XMP_Error &e) {
        set_error(e);
    }
    catch (...) {
    }
    return ret;
}

bool xmp_set_property_float(XmpPtr xmp, const char *schema, const char *name,
                            double value, uint32_t optionBits)
{
    CHECK_PTR(xmp, false);
    RESET_ERROR;

    bool ret = false;
    auto txmp = reinterpret_cast<SXMPMeta *>(xmp);
    try {
        txmp->SetProperty_Float(schema, name, value, optionBits);
        ret = true;
    }
    catch (const XMP_Error &e) {
        set_error(e);
    }
    catch (...) {
    }
    return ret;
}

bool xmp_set_property_bool(XmpPtr xmp, const char *schema, const char *name,
                           bool value, uint32_t optionBits)
{
    CHECK_PTR(xmp, false);
    RESET_ERROR;

    bool ret = false;
    auto txmp = reinterpret_cast<SXMPMeta *>(xmp);
    try {
        txmp->SetProperty_Bool(schema, name, value, optionBits);
        ret = true;
    }
    catch (const XMP_Error &e) {
        set_error(e);
    }
    catch (...) {
    }
    return ret;
}

bool xmp_set_property_int32(XmpPtr xmp, const char *schema, const char *name,
                            int32_t value, uint32_t optionBits)
{
    CHECK_PTR(xmp, false);
    RESET_ERROR;

    bool ret = false;
    auto txmp = reinterpret_cast<SXMPMeta *>(xmp);
    try {
        txmp->SetProperty_Int(schema, name, value, optionBits);
        ret = true;
    }
    catch (const XMP_Error &e) {
        set_error(e);
    }
    catch (...) {
    }
    return ret;
}

bool xmp_set_property_int64(XmpPtr xmp, const char *schema, const char *name,
                            int64_t value, uint32_t optionBits)
{
    CHECK_PTR(xmp, false);
    RESET_ERROR;

    bool ret = false;
    auto txmp = reinterpret_cast<SXMPMeta *>(xmp);
    try {
        txmp->SetProperty_Int64(schema, name, value, optionBits);
        ret = true;
    }
    catch (const XMP_Error &e) {
        set_error(e);
    }
    catch (...) {
    }
    return ret;
}

bool xmp_set_array_item(XmpPtr xmp, const char *schema, const char *name,
                        int32_t index, const char *value, uint32_t optionBits)
{
    CHECK_PTR(xmp, false);
    RESET_ERROR;

    bool ret = false;
    auto txmp = reinterpret_cast<SXMPMeta *>(xmp);
    try {
        txmp->SetArrayItem(schema, name, index, value, optionBits);
        ret = true;
    }
    catch (const XMP_Error &e) {
        set_error(e);
    }
    catch (...) {
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
    auto txmp = reinterpret_cast<SXMPMeta *>(xmp);
    try {
        txmp->AppendArrayItem(schema, name, arrayOptions, value, optionBits);
        ret = true;
    }
    catch (const XMP_Error &e) {
        set_error(e);
    }
    catch (...) {
    }
    return ret;
}

bool xmp_delete_property(XmpPtr xmp, const char *schema, const char *name)
{
    CHECK_PTR(xmp, false);
    RESET_ERROR;

    bool ret = true;
    auto txmp = reinterpret_cast<SXMPMeta *>(xmp);
    try {
        txmp->DeleteProperty(schema, name);
    }
    catch (const XMP_Error &e) {
        set_error(e);
        ret = false;
    }
    catch (...) {
        ret = false;
    }
    return ret;
}

bool xmp_has_property(XmpPtr xmp, const char *schema, const char *name)
{
    CHECK_PTR(xmp, false);
    RESET_ERROR;

    bool ret = true;
    auto txmp = reinterpret_cast<const SXMPMeta *>(xmp);
    try {
        ret = txmp->DoesPropertyExist(schema, name);
    }
    catch (const XMP_Error &e) {
        set_error(e);
        ret = false;
    }
    catch (...) {
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
        auto txmp = reinterpret_cast<const SXMPMeta *>(xmp);
        XMP_OptionBits optionBits;
        ret = txmp->GetLocalizedText(schema, name, genericLang, specificLang,
                                     STRING(actualLang), STRING(itemValue),
                                     &optionBits);
        if (propsBits) {
            *propsBits = optionBits;
        }
    }
    catch (const XMP_Error &e) {
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
    auto txmp = reinterpret_cast<SXMPMeta *>(xmp);
    try {
        txmp->SetLocalizedText(schema, name, genericLang, specificLang, value,
                               optionBits);
    }
    catch (const XMP_Error &e) {
        set_error(e);
        ret = false;
    }
    catch (...) {
        ret = false;
    }
    return ret;
}

bool xmp_delete_localized_text(XmpPtr xmp, const char *schema, const char *name,
                               const char *genericLang,
                               const char *specificLang)
{
    CHECK_PTR(xmp, false);
    RESET_ERROR;

    bool ret = true;
    auto txmp = reinterpret_cast<SXMPMeta *>(xmp);
    try {
        txmp->DeleteLocalizedText(schema, name, genericLang, specificLang);
    }
    catch (const XMP_Error &e) {
        set_error(e);
        ret = false;
    }
    catch (...) {
        ret = false;
    }
    return ret;
}

XmpStringPtr xmp_string_new()
{
    return (XmpStringPtr) new std::string;
}

void xmp_string_free(XmpStringPtr s)
{
    auto str = reinterpret_cast<std::string *>(s);
    delete str;
}

const char *xmp_string_cstr(XmpStringPtr s)
{
    CHECK_PTR(s, NULL);
    return reinterpret_cast<const std::string *>(s)->c_str();
}

size_t xmp_string_len(XmpStringPtr s)
{
    CHECK_PTR(s, 0);
    return reinterpret_cast<const std::string *>(s)->size();
}

XmpIteratorPtr xmp_iterator_new(XmpPtr xmp, const char *schema,
                                const char *propName, XmpIterOptions options)
{
    CHECK_PTR(xmp, NULL);
    RESET_ERROR;

    try {
        auto xiter = std::unique_ptr<SXMPIterator>(
            new SXMPIterator(*(SXMPMeta *)xmp, schema, propName, options));

        return reinterpret_cast<XmpIteratorPtr>(xiter.release());
    }
    catch (const XMP_Error &e) {
        set_error(e);
    }

    return NULL;
}

bool xmp_iterator_free(XmpIteratorPtr iter)
{
    CHECK_PTR(iter, false);
    RESET_ERROR;
    auto titer = reinterpret_cast<SXMPIterator *>(iter);
    delete titer;
    return true;
}

bool xmp_iterator_next(XmpIteratorPtr iter, XmpStringPtr schema,
                       XmpStringPtr propName, XmpStringPtr propValue,
                       uint32_t *options)
{
    CHECK_PTR(iter, false);
    RESET_ERROR;
    auto titer = reinterpret_cast<SXMPIterator *>(iter);
    return titer->Next(reinterpret_cast<std::string *>(schema),
                       reinterpret_cast<std::string *>(propName),
                       reinterpret_cast<std::string *>(propValue), options);
}

bool xmp_iterator_skip(XmpIteratorPtr iter, XmpIterSkipOptions options)
{
    CHECK_PTR(iter, false);
    RESET_ERROR;
    auto titer = reinterpret_cast<SXMPIterator *>(iter);
    titer->Skip(options);
    return true;
}

int xmp_datetime_compare(XmpDateTime *left, XmpDateTime *right)
{
    if (!left && !right) {
        return 0;
    }
    if (!left) {
        return -1;
    }
    if (!right) {
        return 1;
    }
    XMP_DateTime _left;
    ASSIGN(_left, *left);
    XMP_DateTime _right;
    ASSIGN(_right, *right);
    return XMPUtils::CompareDateTime(_left, _right);
}

#ifdef __cplusplus
}
#endif
