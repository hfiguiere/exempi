#
# exempi - Makefile.am
#
# Copyright (C) 2007-2013 Hubert Figuiere
# All rights reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions
# are met:
#
# 1 Redistributions of source code must retain the above copyright
# notice, this list of conditions and the following disclaimer.
# 
# 2 Redistributions in binary form must reproduce the above copyright
# notice, this list of conditions and the following disclaimer in the
# documentation and/or other materials provided with the
# distribution.
#
# 3 Neither the name of the Authors, nor the names of its
# contributors may be used to endorse or promote products derived
# from this software wit hout specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
# "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
# LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
# FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
# COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
# INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
# (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
# SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
# HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
# STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
# ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
# OF THE POSSIBILITY OF SUCH DAMAGE.
#

SUBDIRS =
DIST_SUBDIRS = doc

exempidir = $(includedir)/@EXEMPI_INCLUDE_BASE@/exempi
exempi_HEADERS = xmp.h xmpconsts.h xmperrors.h xmp++.hpp

bin_PROGRAMS = exempi

exempi_SOURCES = main.cpp
exempi_CPPFLAGS = -I$(srcdir)/..
exempi_LDADD = libexempi.la

AM_CPPFLAGS = -I$(top_srcdir)/public/include -I$(top_srcdir)/XMPCore/source \
	-I$(top_srcdir)/build/ -I$(top_srcdir) \
	-I$(top_srcdir)/source/common/ \
	-Wall -D@EXEMPI_PLATFORM_DEF@=1

lib_LTLIBRARIES = libexempi.la

pkgconfig_DATA = exempi-@EXEMPI_MAJOR_VERSION@.pc

libexempi_la_SOURCES = exempi.cpp

libexempi_la_LIBADD = $(top_builddir)/source/libxmpcommon.la \
	$(top_builddir)/XMPCore/source/libXMPCore.la \
	$(top_builddir)/XMPFiles/source/libXMPFiles.la \
	$(top_builddir)/third-party/zuid/interfaces/libmd5.la \
	-lexpat -lz -ldl

if UNIX_ENV
libexempi_la_LIBADD += -lrt
endif

libexempi_la_LDFLAGS = -version-info @EXEMPI_VERSION_INFO@

dist_man1_MANS = exempi.1

### tests

AUTOMAKE_OPTIONS = parallel-tests
TEST_EXTENSIONS = .sh

if WITH_UNIT_TEST
check_PROGRAMS = testexempicore testserialise \
	testwritenewprop \
	testtiffleak testxmpfiles testxmpfileswrite \
	testparse testiterator testinit testfdo18635\
	testfdo83313 testcpp testwebp \
	testadobesdk testxmpformat \
	$(NULL)
TESTS = tests/testcore.sh testinit testexempicore testserialise \
	testwritenewprop \
	testtiffleak testxmpfiles testxmpfileswrite \
	testparse testiterator testfdo18635 testfdo83313 testcpp testwebp \
	testadobesdk testxmpformat \
	$(NULL)
TESTS_ENVIRONMENT = TEST_DIR=$(srcdir)/tests BOOST_TEST_CATCH_SYSTEM_ERRORS=no VALGRIND="$(VALGRIND)"
LOG_COMPILER = $(VALGRIND)
endif
check_SCRIPTS = tests/testcore.sh

check_DATA = tests/test1.xmp tests/fdo18635.jpg tests/fdo83313.jpg

noinst_HEADERS = tests/utils.h

EXTRA_DIST += $(check_DATA) $(check_SCRIPTS)
CLEANFILES = test.jpg test.webp

AM_CXXFLAGS = @BOOST_CPPFLAGS@

testinit_SOURCES = tests/testinit.cpp tests/utils.cpp
testinit_LDADD = libexempi.la @BOOST_UNIT_TEST_FRAMEWORK_LIBS@
testinit_LDFLAGS = -static @BOOST_UNIT_TEST_FRAMEWORK_LDFLAGS@

testexempicore_SOURCES = tests/test-exempi-core.cpp tests/utils.cpp
testexempicore_LDADD = libexempi.la @BOOST_UNIT_TEST_FRAMEWORK_LIBS@
testexempicore_LDFLAGS = -static @BOOST_UNIT_TEST_FRAMEWORK_LDFLAGS@

testserialise_SOURCES = tests/test-serialise.cpp tests/utils.cpp
testserialise_LDADD = libexempi.la @BOOST_UNIT_TEST_FRAMEWORK_LIBS@
testserialise_LDFLAGS = -static @BOOST_UNIT_TEST_FRAMEWORK_LDFLAGS@

testwritenewprop_SOURCES = tests/test-write-new-prop.cpp tests/utils.cpp
testwritenewprop_LDADD = libexempi.la @BOOST_UNIT_TEST_FRAMEWORK_LIBS@
testwritenewprop_LDFLAGS = -static @BOOST_UNIT_TEST_FRAMEWORK_LDFLAGS@

testtiffleak_SOURCES = tests/test-tiff-leak.cpp tests/utils.cpp
testtiffleak_LDADD = libexempi.la @BOOST_UNIT_TEST_FRAMEWORK_LIBS@
testtiffleak_LDFLAGS = -static @BOOST_UNIT_TEST_FRAMEWORK_LDFLAGS@

testxmpfiles_SOURCES = tests/test-xmpfiles.cpp tests/utils.cpp
testxmpfiles_LDADD = libexempi.la @BOOST_UNIT_TEST_FRAMEWORK_LIBS@
testxmpfiles_LDFLAGS = -static @BOOST_UNIT_TEST_FRAMEWORK_LDFLAGS@

testxmpfileswrite_SOURCES = tests/test-xmpfiles-write.cpp tests/utils.cpp
testxmpfileswrite_LDADD = libexempi.la @BOOST_UNIT_TEST_FRAMEWORK_LIBS@
testxmpfileswrite_LDFLAGS = -static @BOOST_UNIT_TEST_FRAMEWORK_LDFLAGS@

testparse_SOURCES = tests/testparse.cpp tests/utils.cpp
testparse_LDADD = libexempi.la @BOOST_UNIT_TEST_FRAMEWORK_LIBS@
testparse_LDFLAGS = -static @BOOST_UNIT_TEST_FRAMEWORK_LDFLAGS@

testiterator_SOURCES = tests/test-iterator.cpp tests/utils.cpp
testiterator_LDADD = libexempi.la @BOOST_UNIT_TEST_FRAMEWORK_LIBS@
testiterator_LDFLAGS = -static @BOOST_UNIT_TEST_FRAMEWORK_LDFLAGS@

testfdo18635_SOURCES = tests/test-bgo.cpp tests/utils.cpp
testfdo18635_LDADD = libexempi.la @BOOST_UNIT_TEST_FRAMEWORK_LIBS@
testfdo18635_LDFLAGS = -static @BOOST_UNIT_TEST_FRAMEWORK_LDFLAGS@

testfdo83313_SOURCES = tests/test-fdo83313.cpp tests/utils.cpp
testfdo83313_LDADD = libexempi.la @BOOST_UNIT_TEST_FRAMEWORK_LIBS@
testfdo83313_LDFLAGS = -static @BOOST_UNIT_TEST_FRAMEWORK_LDFLAGS@

testcpp_SOURCES = tests/testcpp.cpp tests/utils.cpp
testcpp_LDADD = libexempi.la @BOOST_UNIT_TEST_FRAMEWORK_LIBS@
testcpp_LDFLAGS = -static @BOOST_UNIT_TEST_FRAMEWORK_LDFLAGS@

testwebp_SOURCES = tests/test-webp.cpp tests/utils.cpp
testwebp_LDADD = libexempi.la @BOOST_UNIT_TEST_FRAMEWORK_LIBS@
testwebp_LDFLAGS = -static @BOOST_UNIT_TEST_FRAMEWORK_LDFLAGS@

testadobesdk_SOURCES = tests/test-adobesdk.cpp
testadobesdk_LDADD = libexempi.la @BOOST_UNIT_TEST_FRAMEWORK_LIBS@
testadobesdk_LDFLAGS = -static @BOOST_UNIT_TEST_FRAMEWORK_LDFLAGS@

testxmpformat_SOURCES = tests/test-xmpformat.cpp
testxmpformat_LDADD = libexempi.la @BOOST_UNIT_TEST_FRAMEWORK_LIBS@
testxmpformat_LDFLAGS = -static @BOOST_UNIT_TEST_FRAMEWORK_LDFLAGS@
