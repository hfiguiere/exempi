/*
 * exempi - test-exempi-core.cpp
 *
 * Copyright (C) 2007-2008 Hubert Figuiere
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

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <string>

#include <boost/test/minimal.hpp>

#include "utils.h"
#include "xmpconsts.h"
#include "xmp.h"

using boost::unit_test::test_suite;


//void test_exempi()
int test_main(int argc, char *argv[])
{
	prepare_test(argc, argv, "test1.xmp");

	size_t len;
	char * buffer;
	
	FILE * f = fopen(g_testfile.c_str(), "rb");

	BOOST_CHECK(f != NULL);
	if (f == NULL) {
		exit(128);
	}
 	fseek(f, 0, SEEK_END);
	len = ftell(f);
 	fseek(f, 0, SEEK_SET);

	buffer = (char*)malloc(len + 1);
	size_t rlen = fread(buffer, 1, len, f);

	BOOST_CHECK(rlen == len);
	BOOST_CHECK(len != 0);

	BOOST_CHECK(xmp_init());

	XmpPtr xmp = xmp_new_empty();

	BOOST_CHECK(xmp_parse(xmp, buffer, len));

	BOOST_CHECK(xmp != NULL);

	XmpStringPtr the_prop = xmp_string_new();

	BOOST_CHECK(xmp_has_property(xmp, NS_TIFF, "Make"));
	BOOST_CHECK(!xmp_has_property(xmp, NS_TIFF, "Foo"));

	BOOST_CHECK(xmp_get_property(xmp, NS_TIFF, "Make", the_prop, NULL));
	BOOST_CHECK(strcmp("Canon", xmp_string_cstr(the_prop)) == 0); 

	BOOST_CHECK(xmp_set_property(xmp, NS_TIFF, "Make", "Leica", 0));
	BOOST_CHECK(xmp_get_property(xmp, NS_TIFF, "Make", the_prop, NULL));
	BOOST_CHECK(strcmp("Leica", xmp_string_cstr(the_prop)) == 0); 

	uint32_t bits;
	BOOST_CHECK(xmp_get_property(xmp, NS_DC, "rights[1]/?xml:lang",
								 the_prop, &bits));
	BOOST_CHECK(bits == 0x20);
	BOOST_CHECK(XMP_IS_PROP_QUALIFIER(bits));

	BOOST_CHECK(xmp_get_property(xmp, NS_DC, "rights[1]",
								 the_prop, &bits));
	BOOST_CHECK(bits == 0x50);
	BOOST_CHECK(XMP_HAS_PROP_QUALIFIERS(bits));

	XmpStringPtr the_lang = xmp_string_new();
	BOOST_CHECK(xmp_get_localized_text(xmp, NS_DC, "rights",
									   NULL, "x-default", 
									   the_lang, the_prop, &bits));
	BOOST_CHECK(strcmp("x-default", xmp_string_cstr(the_lang)) == 0); 
	BOOST_CHECK(xmp_set_localized_text(xmp, NS_DC, "rights",
									   "en", "en-CA", 
									   xmp_string_cstr(the_prop), 0));	
	BOOST_CHECK(xmp_get_localized_text(xmp, NS_DC, "rights",
									   "en", "en-US", 
									   the_lang, the_prop, &bits));
	BOOST_CHECK(strcmp("en-US", xmp_string_cstr(the_lang)) != 0); 
	BOOST_CHECK(strcmp("en-CA", xmp_string_cstr(the_lang)) == 0); 

	BOOST_CHECK(xmp_delete_localized_text(xmp, NS_DC, "rights",
										  "en", "en-CA"));
	BOOST_CHECK(xmp_has_property(xmp, NS_DC, "rights[1]"));
	BOOST_CHECK(!xmp_has_property(xmp, NS_DC, "rights[2]"));
	

	xmp_string_free(the_lang);

	BOOST_CHECK(xmp_set_array_item(xmp, NS_DC, "creator", 2,
								   "foo", 0));
	BOOST_CHECK(xmp_get_array_item(xmp, NS_DC, "creator", 2,
								   the_prop, &bits));
	BOOST_CHECK(XMP_IS_PROP_SIMPLE(bits));
	BOOST_CHECK(strcmp("foo", xmp_string_cstr(the_prop)) == 0); 
	BOOST_CHECK(xmp_append_array_item(xmp, NS_DC, "creator", 0, "bar", 0));
	
	BOOST_CHECK(xmp_get_array_item(xmp, NS_DC, "creator", 3,
								   the_prop, &bits));
	BOOST_CHECK(XMP_IS_PROP_SIMPLE(bits));
	BOOST_CHECK(strcmp("bar", xmp_string_cstr(the_prop)) == 0); 
	
	BOOST_CHECK(xmp_delete_property(xmp, NS_DC, "creator[3]"));
	BOOST_CHECK(!xmp_has_property(xmp, NS_DC, "creator[3]"));


	BOOST_CHECK(xmp_get_property(xmp, NS_EXIF, "DateTimeOriginal", 
											the_prop, NULL));
	BOOST_CHECK(strcmp("2006-12-07T23:20:43-05:00", 
							 xmp_string_cstr(the_prop)) == 0); 

	BOOST_CHECK(xmp_get_property(xmp, NS_XAP, "Rating",
				the_prop, NULL));
	BOOST_CHECK(strcmp("3", xmp_string_cstr(the_prop)) == 0); 
	
	xmp_string_free(the_prop);

	// testing date time get
	XmpDateTime the_dt;
	bool ret;
	BOOST_CHECK(ret = xmp_get_property_date(xmp, NS_EXIF, "DateTimeOriginal", 
											&the_dt, NULL));
	BOOST_CHECK(the_dt.year == 2006);
	BOOST_CHECK(the_dt.minute == 20);
	BOOST_CHECK(the_dt.tzSign == XMP_TZ_WEST);


	// testing float get set
	double float_value = 0.0;
	BOOST_CHECK(xmp_get_property_float(xmp, NS_CAMERA_RAW_SETTINGS, 
									   "SharpenRadius", &float_value, NULL));
	BOOST_CHECK(float_value == 1.0);
	BOOST_CHECK(xmp_set_property_float(xmp, NS_CAMERA_RAW_SETTINGS, 
									   "SharpenRadius", 2.5, 0));
	BOOST_CHECK(xmp_get_property_float(xmp, NS_CAMERA_RAW_SETTINGS, 
									   "SharpenRadius", &float_value, NULL));
	BOOST_CHECK(float_value == 2.5);
	

	// testing bool get set
	bool bool_value = true;
	BOOST_CHECK(xmp_get_property_bool(xmp, NS_CAMERA_RAW_SETTINGS, 
									   "AlreadyApplied", &bool_value, NULL));
	BOOST_CHECK(!bool_value);
	BOOST_CHECK(xmp_set_property_bool(xmp, NS_CAMERA_RAW_SETTINGS, 
									   "AlreadyApplied", true, 0));
	BOOST_CHECK(xmp_get_property_bool(xmp, NS_CAMERA_RAW_SETTINGS, 
									   "AlreadyApplied", &bool_value, NULL));
	BOOST_CHECK(bool_value);	

	// testing int get set
	int32_t value = 0;
	BOOST_CHECK(xmp_get_property_int32(xmp, NS_EXIF, "MeteringMode",
									   &value, NULL));
	BOOST_CHECK(value == 5);
	BOOST_CHECK(xmp_set_property_int32(xmp, NS_EXIF, "MeteringMode",
									   10, 0));
	int64_t valuelarge = 0;
	BOOST_CHECK(xmp_get_property_int64(xmp, NS_EXIF, "MeteringMode",
									   &valuelarge, NULL));
	BOOST_CHECK(valuelarge == 10);
	BOOST_CHECK(xmp_set_property_int64(xmp, NS_EXIF, "MeteringMode",
									   32, 0));

	BOOST_CHECK(xmp_get_property_int32(xmp, NS_EXIF, "MeteringMode",
									   &value, NULL));
	BOOST_CHECK(value == 32);


	BOOST_CHECK(xmp_free(xmp));


	free(buffer);
	fclose(f);

	xmp_terminate();

	BOOST_CHECK(!g_lt->check_leaks());
	BOOST_CHECK(!g_lt->check_errors());
	return 0;
}


