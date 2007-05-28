/*
 * exempi - xmp.h
 *
 * Copyright (C) 2007 Hubert Figuiere
 * Copyright 2002-2007 Adobe Systems Incorporated
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


#ifndef __EXEMPI_XMP_H_
#define __EXEMPI_XMP_H_

#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* enums grafted from XMP_Const.h */
	

/** Option bits for xmp_files_open() */
typedef enum {
	XMP_OPEN_NOOPTION       = 0x00000000, /**< No open option */
	XMP_OPEN_READ           = 0x00000001, /**< Open for read-only access. */
	XMP_OPEN_FORUPDATE      = 0x00000002, /**< Open for reading and writing. */
	XMP_OPEN_OPNLYXMP       = 0x00000004, /**< Only the XMP is wanted, allows space/time optimizations. */
	XMP_OPEN_CACHETNAIL     = 0x00000008, /**< Cache thumbnail if possible, GetThumbnail will be called. */
	XMP_OPEN_STRICTLY       = 0x00000010, /**< Be strict about locating XMP and reconciling with other forms. */
	XMP_OPEN_USESMARTHANDLER= 0x00000020, /**< Require the use of a smart handler. */
	XMP_OPEN_USEPACKETSCANNING = 0x00000040, /**< Force packet scanning, don't use a smart handler. */
	XMP_OPEN_LIMITSCANNING  = 0x00000080, /**< Only packet scan files "known" to need scanning. */
	XMP_OPEN_INBACKGROUND   = 0x10000000  /**< Set if calling from background thread. */
} XmpOpenFileOptions;


/** Option bits for xmp_files_close() */
typedef enum {
	XMP_CLOSE_NOOPTION      = 0x0000, /**< No close option */
	XMP_CLOSE_SAFEUPDATE    = 0x0001  /**< Write into a temporary file and swap for crash safety. */
} XmpCloseFileOptions;



typedef enum {

	/* Public file formats. Hex used to avoid gcc warnings. */
	/* ! Leave them as big endian. There seems to be no decent way on UNIX to determine the target */
	/* ! endianness at compile time. Forcing it on the client isn't acceptable. */
	
	XMP_FT_PDF      = 0x50444620UL,  /* 'PDF ' */
	XMP_FT_PS       = 0x50532020UL,  /* 'PS  ', general PostScript following DSC conventions. */
	XMP_FT_EPS      = 0x45505320UL,  /* 'EPS ', encapsulated PostScript. */

	XMP_FT_JPEG     = 0x4A504547UL,  /* 'JPEG' */
	XMP_FT_JPEG2K   = 0x4A505820UL,  /* 'JPX ', ISO 15444-1 */
	XMP_FT_TIFF     = 0x54494646UL,  /* 'TIFF' */
	XMP_FT_GIF      = 0x47494620UL,  /* 'GIF ' */
	XMP_FT_PNG      = 0x504E4720UL,  /* 'PNG ' */
    
	XMP_FT_SWF      = 0x53574620UL,  /* 'SWF ' */
	XMP_FT_FLA      = 0x464C4120UL,  /* 'FLA ' */
	XMP_FT_FLV      = 0x464C5620UL,  /* 'FLV ' */
	
  XMP_FT_MOV      = 0x4D4F5620UL,  /* 'MOV ', Quicktime */
  XMP_FT_AVI      = 0x41564920UL,  /* 'AVI ' */
  XMP_FT_CIN      = 0x43494E20UL,  /* 'CIN ', Cineon */
  XMP_FT_WAV      = 0x57415620UL,  /* 'WAV ' */
	XMP_FT_MP3      = 0x4D503320UL,  /* 'MP3 ' */
	XMP_FT_SES      = 0x53455320UL,  /* 'SES ', Audition session */
	XMP_FT_CEL      = 0x43454C20UL,  /* 'CEL ', Audition loop */
	XMP_FT_MPEG     = 0x4D504547UL,  /* 'MPEG' */
	XMP_FT_MPEG2    = 0x4D503220UL,  /* 'MP2 ' */
	XMP_FT_MPEG4    = 0x4D503420UL,  /* 'MP4 ', ISO 14494-12 and -14 */
	XMP_FT_WMAV     = 0x574D4156UL,  /* 'WMAV', Windows Media Audio and Video */
	XMP_FT_AIFF     = 0x41494646UL,  /* 'AIFF' */

	XMP_FT_HTML     = 0x48544D4CUL,  /* 'HTML' */
	XMP_FT_XML      = 0x584D4C20UL,  /* 'XML ' */
	XMP_FT_TEXT     = 0x74657874UL,  /* 'text' */

	/* Adobe application file formats. */
	XMP_FT_PHOTOSHOP       = 0x50534420UL,  /* 'PSD ' */
	XMP_FT_ILLUSTRATOR     = 0x41492020UL,  /* 'AI  ' */
	XMP_FT_INDESIGN        = 0x494E4444UL,  /* 'INDD' */
	XMP_FT_AEPROJECT       = 0x41455020UL,  /* 'AEP ' */
	XMP_FT_AEPROJTEMPLATE  = 0x41455420UL,  /* 'AET ', After Effects Project Template */
	XMP_FT_AEFILTERPRESET  = 0x46465820UL,  /* 'FFX ' */
	XMP_FT_ENCOREPROJECT   = 0x4E434F52UL,  /* 'NCOR' */
	XMP_FT_PREMIEREPROJECT = 0x5052504AUL,  /* 'PRPJ' */
	XMP_FT_PREMIERETITLE   = 0x5052544CUL,  /* 'PRTL' */

	/* Catch all. */
	XMP_FT_UNKNOWN  = 0x20202020UL   /* '    ' */
} XmpFileType;




typedef enum {
	XMP_ITER_CLASSMASK      = 0x00FFUL,  /* The low 8 bits are an enum of what data structure to iterate. */
	XMP_ITER_PROPERTIES     = 0x0000UL,  /* Iterate the property tree of a TXMPMeta object. */
	XMP_ITER_ALIASES        = 0x0001UL,  /* Iterate the global alias table. */
	XMP_ITER_NAMESPACES     = 0x0002UL,  /* Iterate the global namespace table. */
	XMP_ITER_JUSTCHILDREN   = 0x0100UL,  /* Just do the immediate children of the root, default is subtree. */
	XMP_ITER_JUSTLEAFNODES  = 0x0200UL,  /* Just do the leaf nodes, default is all nodes in the subtree. */
	XMP_ITER_JUSTLEAFNAME   = 0x0400UL,  /* Return just the leaf part of the path, default is the full path. */
	XMP_ITER_INCLUDEALIASES = 0x0800UL,  /* Include aliases, default is just actual properties. */
	
	XMP_ITER_OMITQUALIFIERS = 0x1000UL   /* Omit all qualifiers. */
} XmpIterOptions;

typedef enum {
	XMP_ITER_SKIPSUBTREE   = 0x0001UL,  /* Skip the subtree below the current node. */
	XMP_ITER_SKIPSIBLINGS  = 0x0002UL   /* Skip the subtree below and remaining siblings of the current node. */
} XmpIterSkipOptions;


/** pointer to XMP packet. Opaque. */
typedef struct _Xmp *XmpPtr;
typedef struct _XmpFile *XmpFilePtr;
typedef struct _XmpString *XmpStringPtr;
typedef struct _XmpIterator *XmpIteratorPtr;

/** Init the library. Must be called before anything else */
bool xmp_init();
void xmp_terminate();

XmpFilePtr xmp_files_new();
XmpFilePtr xmp_files_open_new(const char *, XmpOpenFileOptions options);

bool xmp_files_open(XmpFilePtr xf, const char *, XmpOpenFileOptions options);
/** Close an XMP file. Will flush the changes
 * @param xf the file object
 * @param optiosn the options to close.
 */
void xmp_files_close(XmpFilePtr xf, XmpCloseFileOptions options);

XmpPtr xmp_files_get_new_xmp(XmpFilePtr xf);
bool xmp_files_get_xmp(XmpFilePtr xf, XmpPtr xmp);

bool xmp_files_can_put_xmp(XmpFilePtr xf, XmpPtr xmp);
void xmp_files_put_xmp(XmpFilePtr xf, XmpPtr xmp);

void xmp_files_free(XmpFilePtr xf);


XmpPtr xmp_new_empty();

/** Create a new XMP packet
 * @param buffer the buffer to load data from. UTF-8 encoded.
 * @param len the buffer length in byte
 * @return the packet point. Must be free with xmp_free()
 */
XmpPtr xmp_new(const char *buffer, size_t len);

/** Free the xmp packet
 * @param xmp the xmp packet to free
 */
void xmp_free(XmpPtr xmp);

/** Parse the XML passed through the buffer and load
 * it.
 * @param xmp the XMP packet.
 * @param buffer the buffer.
 * @param len the length of the buffer.
 */
bool xmp_parse(XmpPtr xmp, const char *buffer, size_t len);


/** Get an XMP property from the XMP packet
 * @param xmp the XMP packet
 * @param schema
 * @param name
 * @param property the allocated XmpStrinPtr
 * @return true if found
 */
bool xmp_get_property(XmpPtr xmp, const char *schema, 
															const char *name, XmpStringPtr property);


/** Set an XMP property from the XMP packet
 * @param xmp the XMP packet
 * @param schema
 * @param name
 * @param value 0 terminated string
 */
void xmp_set_property(XmpPtr xmp, const char *schema, 
											const char *name, const char *value);


/** Instanciate a new string 
 * @return the new instance. Must be freed with 
 * xmp_string_free()
 */
XmpStringPtr xmp_string_new();

/** Free a XmpStringPtr
 * @param s the string to free
 */
void xmp_string_free(XmpStringPtr s);

/** Get the C string from the XmpStringPtr
 * @param s the string object
 * @return the const char * for the XmpStringPtr. It 
 * belong to the object.
 */
const char * xmp_string_cstr(XmpStringPtr s);


/**
 */
XmpIteratorPtr xmp_iterator_new(XmpPtr xmp, const char * schema,
																const char * propName, XmpIterOptions options);

/**
 */
void xmp_iterator_free(XmpIteratorPtr iter);

/** Iterate to the next value
 * @param iter the iterator
 * @param schema the schema name. Pass NULL if not wanted
 * @param propName the property path. Pass NULL if not wanted
 * @param propValue the value of the property. Pass NULL if not wanted.
 * @param options the options for the property. Pass NULL if not wanted.
 * @return true if still something, false if none
 */
bool xmp_iterator_next(XmpIteratorPtr iter, XmpStringPtr schema,
											 XmpStringPtr propName, XmpStringPtr propValue,
											 uint32_t *options);


/**
 */
void xmp_iterator_skip(XmpIteratorPtr iter, XmpIterSkipOptions options);


#ifdef __cplusplus
}
#endif

#endif
