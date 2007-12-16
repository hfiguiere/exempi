/*
 * exempi - test1.cpp
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

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <string>

#include <boost/static_assert.hpp>
#include <boost/test/auto_unit_test.hpp>

#include "utils.h"
#include "xmpconsts.h"
#include "xmp.h"

using boost::unit_test::test_suite;


void test_write_new_property()
{
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

	XmpStringPtr reg_prefix = xmp_string_new();
	BOOST_CHECK(xmp_register_namespace(NS_CC, "cc", reg_prefix));
	BOOST_CHECK_EQUAL(strcmp("cc:", xmp_string_cstr(reg_prefix)),	0); 

	xmp_string_free(reg_prefix);

	BOOST_CHECK(xmp_set_property(xmp, NS_CC, "License", "Foo", 0));

	XmpStringPtr the_prop = xmp_string_new();
	BOOST_CHECK(xmp_get_property(xmp, NS_CC, "License", the_prop, NULL));
	BOOST_CHECK_EQUAL(strcmp("Foo", xmp_string_cstr(the_prop)),	0); 
	xmp_string_free(the_prop);

	BOOST_CHECK(xmp_free(xmp));

	free(buffer);
	fclose(f);

	xmp_terminate();
}


void test_serialize()
{
	size_t len;
	char * buffer;
	FILE * f = fopen(g_testfile.c_str(), "rb");
	
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

	std::string b1(buffer);
	std::string b2;
	XmpStringPtr output = xmp_string_new();

	BOOST_CHECK(xmp_serialize_and_format(xmp, output, 
										 XMP_SERIAL_OMITPACKETWRAPPER, 
										 0, "\n", " ", 0));
	b2 = xmp_string_cstr(output);
	// find a way to compare that.
//	BOOST_CHECK_EQUAL(b1, b2);
	
	xmp_string_free(output);
	BOOST_CHECK(xmp_free(xmp));

	free(buffer);
	fclose(f);

	xmp_terminate();
}


void test_exempi()
{
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
	BOOST_CHECK_EQUAL(xmp_has_property(xmp, NS_TIFF, "Foo"), false);

	BOOST_CHECK(xmp_get_property(xmp, NS_TIFF, "Make", the_prop, NULL));
	BOOST_CHECK_EQUAL(strcmp("Canon", xmp_string_cstr(the_prop)),	0); 

	BOOST_CHECK(xmp_set_property(xmp, NS_TIFF, "Make", "Leica", 0));
	BOOST_CHECK(xmp_get_property(xmp, NS_TIFF, "Make", the_prop, NULL));
	BOOST_CHECK_EQUAL(strcmp("Leica", xmp_string_cstr(the_prop)),	0); 

	uint32_t bits;
	BOOST_CHECK(xmp_get_property(xmp, NS_DC, "rights[1]/?xml:lang",
								 the_prop, &bits));
	BOOST_CHECK_EQUAL(bits, 0x20);
	BOOST_CHECK(XMP_IS_PROP_QUALIFIER(bits));

	BOOST_CHECK(xmp_get_property(xmp, NS_DC, "rights[1]",
								 the_prop, &bits));
	BOOST_CHECK_EQUAL(bits, 0x50);
	BOOST_CHECK(XMP_HAS_PROP_QUALIFIERS(bits));

	XmpStringPtr the_lang = xmp_string_new();
	BOOST_CHECK(xmp_get_localized_text(xmp, NS_DC, "rights",
									   NULL, "x-default", 
									   the_lang, the_prop, &bits));
	BOOST_CHECK_EQUAL(strcmp("x-default", xmp_string_cstr(the_lang)),	0); 
	BOOST_CHECK(xmp_set_localized_text(xmp, NS_DC, "rights",
									   "en", "en-CA", 
									   xmp_string_cstr(the_prop), 0));	
	BOOST_CHECK(xmp_get_localized_text(xmp, NS_DC, "rights",
									   "en", "en-US", 
									   the_lang, the_prop, &bits));
	BOOST_CHECK(strcmp("en-US", xmp_string_cstr(the_lang)) != 0); 
	BOOST_CHECK_EQUAL(strcmp("en-CA", xmp_string_cstr(the_lang)),	0); 

	BOOST_CHECK(xmp_delete_localized_text(xmp, NS_DC, "rights",
										  "en", "en-CA"));
	BOOST_CHECK(xmp_has_property(xmp, NS_DC, "rights[1]"));
	BOOST_CHECK_EQUAL(xmp_has_property(xmp, NS_DC, "rights[2]"), false);
	

	xmp_string_free(the_lang);

	BOOST_CHECK(xmp_set_array_item(xmp, NS_DC, "creator", 2,
								   "foo", 0));
	BOOST_CHECK(xmp_get_array_item(xmp, NS_DC, "creator", 2,
								   the_prop, &bits));
	BOOST_CHECK(XMP_IS_PROP_SIMPLE(bits));
	BOOST_CHECK_EQUAL(strcmp("foo", xmp_string_cstr(the_prop)),	0); 
	BOOST_CHECK(xmp_append_array_item(xmp, NS_DC, "creator", 0, "bar", 0));
	
	BOOST_CHECK(xmp_get_array_item(xmp, NS_DC, "creator", 3,
								   the_prop, &bits));
	BOOST_CHECK(XMP_IS_PROP_SIMPLE(bits));
	BOOST_CHECK_EQUAL(strcmp("bar", xmp_string_cstr(the_prop)),	0); 
	
	BOOST_CHECK(xmp_delete_property(xmp, NS_DC, "creator[3]"));
	BOOST_CHECK_EQUAL(xmp_has_property(xmp, NS_DC, "creator[3]"), false);
	
	xmp_string_free(the_prop);
	BOOST_CHECK(xmp_free(xmp));

	free(buffer);
	fclose(f);

	xmp_terminate();
}



test_suite*
init_unit_test_suite( int argc, char * argv[] ) 
{
    test_suite* test = BOOST_TEST_SUITE("test exempi");

	prepare_test(argc, argv, "test1.xmp");

	test->add(BOOST_TEST_CASE(&test_exempi));
	test->add(BOOST_TEST_CASE(&test_serialize));
	test->add(BOOST_TEST_CASE(&test_write_new_property));
	
    return test;
}

