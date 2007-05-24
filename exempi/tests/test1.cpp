/*
 * exempi - test1.cpp
 *
 * Copyright (C) 2007 Hubert Figuiere
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, 
 * USA
 */

#include <stdlib.h>
#include <stdio.h>

#include <string>

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

	XmpStringPtr the_prop = xmp_string_new();

	BOOST_CHECK(xmp_get_property(xmp, NS_TIFF, "Make", the_prop));
	BOOST_CHECK_EQUAL(strcmp("Canon", xmp_string_cstr(the_prop)),	0); 

	xmp_set_property(xmp, NS_TIFF, "Make", "Nikon");
	BOOST_CHECK(xmp_get_property(xmp, NS_TIFF, "Make", the_prop));
	BOOST_CHECK_EQUAL(strcmp("Nikon", xmp_string_cstr(the_prop)),	0); 

	xmp_string_free(the_prop);
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

