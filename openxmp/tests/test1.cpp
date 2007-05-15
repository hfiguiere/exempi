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

#include <exempi/xmp.h>
#include <exempi/xmpconsts.h>

#include <boost/static_assert.hpp>
#include <boost/test/auto_unit_test.hpp>


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
	/*size_t rlen =*/ fread(buffer, 1, len, f);

	XmpPtr xmp = xmp_new(buffer, len);

	BOOST_CHECK(xmp != NULL);

	XmpTreePtr tree = xmp_get_tree(xmp);
	
	BOOST_CHECK(tree != NULL); 

	XmpSchemaIterator iter = xmp_tree_get_schema_iter(tree);
	BOOST_CHECK(iter != NULL);
	int i = 0;
	while (xmp_schema_iter_is_valid(iter)) {
		i++;
		xmp_schema_iter_next(iter);
	}
	BOOST_CHECK_EQUAL(i, 10);
	xmp_schema_iter_free(iter);

	iter = xmp_tree_get_schema_iter(tree);
	while(xmp_schema_iter_is_valid(iter)) {
		if (strcmp(NS_TIFF, xmp_schema_iter_name(iter)) == 0) {
			break;
		}
		xmp_schema_iter_next(iter);		
	}
	BOOST_CHECK_EQUAL(xmp_schema_iter_is_valid(iter), true);
	XmpSchemaPtr schema = xmp_schema_iter_value(iter);
	BOOST_CHECK(schema != NULL);

	XmpPropertyIterator iter2 = xmp_schema_get_property_iter(schema);
	i = 0;
	while(xmp_property_iter_is_valid(iter2)) {
		i++;
		xmp_property_iter_next(iter2);
	}
	BOOST_CHECK_EQUAL(i, 5);
	xmp_property_iter_free(iter2);
	
	iter2 = xmp_schema_get_property_iter(schema);
	while(xmp_property_iter_is_valid(iter2)) {
		if (strcmp(xmp_property_iter_name(iter2), "Make") == 0) {
			break;
		}
		xmp_property_iter_next(iter2);
	}
	
	XmpValuePtr value = xmp_property_iter_value(iter2);
	const char* make = xmp_value_get_string(value);
	BOOST_CHECK(make != NULL);
	BOOST_CHECK_EQUAL(strcmp(make, "Canon"), 0);

	BOOST_CHECK_EQUAL(strcmp(make, xmp_get_property(xmp, NS_TIFF, "Make")), 
							0); 

	xmp_property_iter_free(iter2);
	
	xmp_schema_iter_free(iter);

	iter = xmp_tree_get_schema_iter(tree);
	while(xmp_schema_iter_is_valid(iter)) {
		if (strcmp(NS_TPG, xmp_schema_iter_name(iter)) == 0) {
			break;
		}
		xmp_schema_iter_next(iter);		
	}

	BOOST_CHECK_EQUAL(xmp_schema_iter_is_valid(iter), true);
	schema = xmp_schema_iter_value(iter);
	iter2 = xmp_schema_get_property_iter(schema);
	while(xmp_property_iter_is_valid(iter2)) {
		if (strcmp(xmp_property_iter_name(iter2), "MaxPageSize") == 0) {
			break;
		}
		xmp_property_iter_next(iter2);
	}	
	BOOST_CHECK_EQUAL(xmp_property_iter_is_valid(iter2), true);
	if (xmp_property_iter_is_valid(iter2)) {
		value = xmp_property_iter_value(iter2);
		const char *field = xmp_value_get_field(value, "unit");
		BOOST_CHECK(field != NULL);
		BOOST_CHECK_EQUAL(strcmp(field, "inches"), 0);
	}
	xmp_property_iter_free(iter2);
	
	xmp_schema_iter_free(iter);


	// lets pretend the fantastic shot taken with a Canon has been 
	// with a Nikon
	BOOST_CHECK_EQUAL(xmp_set_property(xmp, NS_TIFF, "Make", "Nikon"), 0);
	// and make sure it is really done
	BOOST_CHECK_EQUAL(strcmp("Nikon", xmp_get_property(xmp, NS_TIFF, "Make")), 
										0); 

	
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

