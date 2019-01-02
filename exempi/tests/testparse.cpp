/*
 * exempi - test3.cpp
 *
 * Copyright (C) 2007-2017 Hubert Figuiere
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
#include <iostream>

#include <boost/test/minimal.hpp>
#include <boost/format.hpp>

#include "utils.h"
#include "xmp.h"
#include "xmpconsts.h"

using boost::unit_test::test_suite;

// void test_exempi_iterate()
int test_main(int, char *[])
{
  char *buffer;

  buffer = strdup("This is not even XML");
  size_t len = strlen(buffer);

  BOOST_CHECK(len != 0);
  BOOST_CHECK(xmp_init());

  XmpPtr xmp = xmp_new_empty();

  BOOST_CHECK(!xmp_parse(xmp, buffer, len));

  BOOST_CHECK(xmp != NULL);

  BOOST_CHECK(xmp_free(xmp));

  free(buffer);
  xmp_terminate();

  BOOST_CHECK(!g_lt->check_leaks());
  BOOST_CHECK(!g_lt->check_errors());
  return 0;
}
