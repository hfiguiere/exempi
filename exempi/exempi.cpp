/*
 * openxmp - openxmp.cpp
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

#include <string>

#define XMP_INCLUDE_XMPFILES 1
#define UNIX_ENV 1
#define TXMP_STRING_TYPE std::string
#include "XMP.hpp"


#ifdef __cplusplus
extern "C" {
#endif


XmpFilePtr xmp_files_new()
{
	SXMPFiles *txf = new SXMPFiles();
	return (XmpFilePtr)txf;
}

XmpFilePtr xmp_files_open_new(const char *path)
{
	SXMPFiles *txf = new SXMPFiles(path);

	return (XmpFilePtr)txf;
}


bool xmp_files_open(XmpFilePtr xf, const char *path)
{
	SXMPFiles *txf = (SXMPFiles*)xf;
	return txf->OpenFile(path);
}


void xmp_files_close(XmpFilePtr xf)
{
	SXMPFiles *txf = (SXMPFiles*)xf;
	txf->CloseFile();
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


void xmp_files_free(XmpFilePtr xf)
{
	SXMPFiles *txf = (SXMPFiles*)xf;
	try {
		delete txf;
	}
	catch(...) {
	}
}


XmpPtr xmp_new(const char *buffer, size_t len)
{
	SXMPMeta *txmp = new SXMPMeta(buffer, len);
	return (XmpPtr)txmp;
}


void xmp_free(XmpPtr xmp)
{
	SXMPMeta *txmp = (SXMPMeta *)xmp;
	delete txmp;
}


#ifdef __cplusplus
}
#endif
