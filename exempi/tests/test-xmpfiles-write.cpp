/*
 * exempi - test-xmpfile-write.cpp
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
#include <sys/stat.h>

#include <string>

#include <boost/test/minimal.hpp>

#include "utils.h"
#include "xmp.h"
#include "xmpconsts.h"

using boost::unit_test::test_suite;


//void test_xmpfiles_write()
int test_main(int argc, char *argv[])
{
	prepare_test(argc, argv, "../../samples/testfiles/BlueSquare.jpg");

	BOOST_CHECK(xmp_init());

	XmpFilePtr f = xmp_files_open_new(g_testfile.c_str(), XMP_OPEN_READ);

	BOOST_CHECK(f != NULL);
	if (f == NULL) {
		return 1;
	}

	XmpPtr xmp = xmp_files_get_new_xmp(f);
	BOOST_CHECK(xmp != NULL);

	BOOST_CHECK(xmp_files_free(f));

	char buf[1024];
	snprintf(buf, 1024, "cp %s test.jpg ; chmod u+w test.jpg", g_testfile.c_str());
	BOOST_CHECK(system(buf) != -1);
	
	f = xmp_files_open_new("test.jpg", XMP_OPEN_FORUPDATE);

	BOOST_CHECK(f != NULL);
	if (f == NULL) {
		return 2;
	}

	BOOST_CHECK(xmp_set_property(xmp, NS_PHOTOSHOP, "ICCProfile", "foo", 0));

	BOOST_CHECK(xmp_files_can_put_xmp(f, xmp));
	BOOST_CHECK(xmp_files_put_xmp(f, xmp));

	BOOST_CHECK(xmp_free(xmp));
	BOOST_CHECK(xmp_files_close(f, XMP_CLOSE_SAFEUPDATE));
	BOOST_CHECK(xmp_files_free(f));

	f = xmp_files_open_new("test.jpg", XMP_OPEN_READ);

	BOOST_CHECK(f != NULL);
	if (f == NULL) {
		return 3;
	}
	xmp = xmp_files_get_new_xmp(f);
	BOOST_CHECK(xmp != NULL);

	XmpStringPtr the_prop = xmp_string_new();
	BOOST_CHECK(xmp_get_property(xmp, NS_PHOTOSHOP, "ICCProfile", the_prop, NULL));
	BOOST_CHECK(strcmp("foo", xmp_string_cstr(the_prop)) == 0); 

	xmp_string_free(the_prop);

	BOOST_CHECK(xmp_free(xmp));
	BOOST_CHECK(xmp_files_close(f, XMP_CLOSE_NOOPTION));
	BOOST_CHECK(xmp_files_free(f));

//	unlink("test.jpg");
	xmp_terminate();

	BOOST_CHECK(!g_lt->check_leaks());
	BOOST_CHECK(!g_lt->check_errors());

	return 0;
}

