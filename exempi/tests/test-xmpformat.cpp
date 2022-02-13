/*
 * exempi - test-xmpformat.cpp
 *
 * Copyright (C) 2020-2022 Hubert Figuière
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

#include <map>
#include <string>

#include <boost/test/included/unit_test.hpp>

#include "xmp.h"

const std::map<const char*, XmpFileType> TEST_CASES = {
  { "avi", XMP_FT_AVI },
  { "eps", XMP_FT_EPS },
  { "gif", XMP_FT_GIF },
  { "indd", XMP_FT_INDESIGN },
  { "jpg", XMP_FT_JPEG },
  { "mov", XMP_FT_MOV },
  { "mp3", XMP_FT_MP3 },
  { "png", XMP_FT_PNG },
  { "psd", XMP_FT_PHOTOSHOP },
  { "tif", XMP_FT_TIFF },
  { "webp", XMP_FT_WEBP },
  { "wav", XMP_FT_WAV }
};

const std::map<const char*, XmpFileType> FAILING_CASES = {
  // This is a GIF87a. Not supported.
  { "gif", XMP_FT_UNKNOWN }
};

// Test that the sample files match the file type.
//
// This test would have caught:
// https://gitlab.freedesktop.org/libopenraw/exempi/-/issues/20
//
boost::unit_test::test_suite* init_unit_test_suite(int, char **)
{
  return nullptr;
}

BOOST_AUTO_TEST_CASE(test_xmpformat)
{
  const char* srcdir = getenv("srcdir");
  if (!srcdir) {
    srcdir = ".";
  }

  BOOST_CHECK(xmp_init());

  for (auto testcase : TEST_CASES) {
    std::string imagepath(srcdir);
    imagepath += "/../samples/testfiles/BlueSquare.";
    imagepath += testcase.first;

    printf("%s\n", imagepath.c_str());
    auto xft = xmp_files_check_file_format(imagepath.c_str());
    BOOST_CHECK(xft == testcase.second);
  }

  for (auto testcase : FAILING_CASES) {
    std::string imagepath(srcdir);
    imagepath += "/../samples/testfiles/BlueSquare-FAIL.";
    imagepath += testcase.first;

    printf("%s\n", imagepath.c_str());
    auto xft = xmp_files_check_file_format(imagepath.c_str());
    BOOST_CHECK(xft == testcase.second);
  }
}
