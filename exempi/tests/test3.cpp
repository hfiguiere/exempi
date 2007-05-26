/*
 * exempi - test3.cpp
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

#include <string>
#include <iostream>

#include <boost/static_assert.hpp>
#include <boost/test/auto_unit_test.hpp>

#include "xmp.h"
#include "xmpconsts.h"

using boost::unit_test::test_suite;

std::string g_testfile;


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


	XmpIteratorPtr iter = xmp_iterator_new(xmp, NULL, NULL, kXMP_IterOmitQualifiers);

	XmpStringPtr the_schema = xmp_string_new();
	XmpStringPtr the_path = xmp_string_new();
	XmpStringPtr the_prop = xmp_string_new();
	uint32_t options;

	while( xmp_iterator_next(iter, the_schema, the_path, the_prop, &options) )
	{
		std::cout << xmp_string_cstr(the_schema) << " / "
							<< xmp_string_cstr(the_path) << " = "
							<< xmp_string_cstr(the_prop) << std::endl;
	}



	xmp_string_free(the_prop);
	xmp_string_free(the_path);
	xmp_string_free(the_schema);
	xmp_iterator_free(iter);
	xmp_free(xmp);

	free(buffer);
	fclose(f);
}



test_suite*
init_unit_test_suite( int argc, char * argv[] ) 
{
    test_suite* test = BOOST_TEST_SUITE("test exempi");

		if (argc == 1) {
			// no argument, lets run like we are in "check"
			const char * srcdir = getenv("srcdir");
			
			BOOST_ASSERT(srcdir != NULL);
			g_testfile = std::string(srcdir);
			g_testfile += "/test1.xmp";
		}
		else {
			g_testfile = argv[1];
		}
		
		test->add(BOOST_TEST_CASE(&test_exempi));

    return test;
}

