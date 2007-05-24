/*
 * openxmp - xmp.h
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


#ifndef __EXEMPI_XMP_H_
#define __EXEMPI_XMP_H_

#include <stdlib.h>

#ifdef __cplusplus
extern "C" {
#endif

/** pointer to XMP packet. Opaque. */
typedef struct _Xmp *XmpPtr;
typedef struct _XmpFile *XmpFilePtr;
typedef struct _XmpString *XmpStringPtr;


bool xmp_init();

XmpFilePtr xmp_files_new();
XmpFilePtr xmp_files_open_new(const char *);

bool xmp_files_open(XmpFilePtr xf, const char *);
void xmp_files_close(XmpFilePtr xf);

XmpPtr xmp_files_get_new_xmp(XmpFilePtr xf);
bool xmp_files_get_xmp(XmpFilePtr xf, XmpPtr xmp);
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


#if 0
//////////////////////////////////////////////////////

typedef struct _XmpTree *XmpTreePtr;
typedef struct _XmpSchema *XmpSchemaPtr;
typedef struct _XmpValue *XmpValuePtr;
typedef struct _XmpSchemaIterator *XmpSchemaIterator;
typedef struct _XmpPropertyIterator *XmpPropertyIterator;







/** Return an xmp tree
 * @param xmp the xmp packet
 * @return a pointer to the xmp tree
 * The returned point belong to the xmp packet.
 */
XmpTreePtr xmp_get_tree(XmpPtr xmp);

/** Get the iterator to walk through schemas
 * @param tree the XMP tree to walk in
 * @return an iterator positionned to the first item.
 * The iterator must be freed by the caller using 
 * xmp_schema_iter_free()
 */
XmpSchemaIterator xmp_tree_get_schema_iter(XmpTreePtr tree);
/** Move to the next element
 * @return 1 if still valid, 0 if reached the end.
 */
int xmp_schema_iter_next(XmpSchemaIterator iter);
/** Test if the schema iterator is valid 
 */
int xmp_schema_iter_is_valid(XmpSchemaIterator iter);
/** Get the schema name at the current iterator position
 * @return the name of the schema. The pointer belongs 
 * to the schema and can be invalidated if the XMP data
 * change.
 */
const char* xmp_schema_iter_name(XmpSchemaIterator iter);
/** Get the schema out of the iterator position
 * @return the schema at the iterator position. The pointer
 * belongs to the tree and can be invalidated if the XMP data
 * change.
 */
XmpSchemaPtr xmp_schema_iter_value(XmpSchemaIterator iter);
/** Free a schema iterator.
 * @param iter the iterator to free
 */
void xmp_schema_iter_free(XmpSchemaIterator iter);


/** Get the iterator to go through properties
 * @param schema the schema to walk through
 * @return the allocated iterator, positionned to the first item. 
 * The iteraotor must be freed by xmp_property_iter_free()
 */
XmpPropertyIterator xmp_schema_get_property_iter(XmpSchemaPtr schema);
/** Move the iterator to the next element
 * @return 1 if the iterator is still valid. 0 if the end
 * is reached.
 */
int xmp_property_iter_next(XmpPropertyIterator iter);
/** Test if the iterator is valid.
 */
int xmp_property_iter_is_valid(XmpPropertyIterator iter);
/** Get the property name at the iterator location 
 */
const char* xmp_property_iter_name(XmpPropertyIterator iter);
/** Get the property value at the iterator location 
 */
XmpValuePtr xmp_property_iter_value(XmpPropertyIterator iter);
/** Free the iterator 
 */
void xmp_property_iter_free(XmpPropertyIterator iter);


/** Get the string value. If it is not a string, return NULL
 * @param value the value to retrieve the string from
 * @return a zero terminated C string, or NULL. String belong
 * to the XMP value
 */
const char* xmp_value_get_string(XmpValuePtr value);

/** Get a struct field.
 * @param value the value to retrieve the string from
 * @param field the field from the structure
 * @return the field value, or NULL if the value is not struct or 
 * the field does not exist
 */
const char* xmp_value_get_field(XmpValuePtr value, const char *field);
#endif

#ifdef __cplusplus
}
#endif

#endif
