#
# exempi - Makefile.am
#
# Copyright (C) 2007-2008 Hubert Figuiere
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


INCLUDES = -I$(srcdir)/.. -I$(top_srcdir)/public/include
if WITH_UNIT_TEST
check_PROGRAMS = testexempicore testserialise testwritenewprop \
	testtiffleak testxmpfiles testxmpfileswrite \
	test3 testinit testfdo18635
TESTS = testcore.sh testinit testexempicore testserialise testwritenewprop \
	testtiffleak testxmpfiles testxmpfileswrite \
	test3 testfdo18635
TESTS_ENVIRONMENT = TEST_DIR=$(srcdir) BOOST_TEST_CATCH_SYSTEM_ERRORS=no @VALGRIND@
endif
check_SCRIPTS = testcore.sh

check_DATA = test1.xmp fdo18635.jpg

noinst_HEADERS = utils.h

EXTRA_DIST = $(check_DATA) $(check_SCRIPTS)
CLEANFILES = test.jpg


testinit_SOURCES = testinit.cpp utils.cpp
testinit_LDADD = ../libexempi.la @BOOST_UNIT_TEST_FRAMEWORK_LIBS@
testinit_LDFLAGS = -static @BOOST_UNIT_TEST_FRAMEWORK_LDFLAGS@

testexempicore_SOURCES = test-exempi-core.cpp utils.cpp
testexempicore_LDADD = ../libexempi.la @BOOST_UNIT_TEST_FRAMEWORK_LIBS@
testexempicore_LDFLAGS = -static @BOOST_UNIT_TEST_FRAMEWORK_LDFLAGS@

testserialise_SOURCES = test-serialise.cpp utils.cpp
testserialise_LDADD = ../libexempi.la @BOOST_UNIT_TEST_FRAMEWORK_LIBS@
testserialise_LDFLAGS = -static @BOOST_UNIT_TEST_FRAMEWORK_LDFLAGS@

testwritenewprop_SOURCES = test-write-new-prop.cpp utils.cpp
testwritenewprop_LDADD = ../libexempi.la @BOOST_UNIT_TEST_FRAMEWORK_LIBS@
testwritenewprop_LDFLAGS = -static @BOOST_UNIT_TEST_FRAMEWORK_LDFLAGS@

testtiffleak_SOURCES = test-tiff-leak.cpp utils.cpp
testtiffleak_LDADD = ../libexempi.la @BOOST_UNIT_TEST_FRAMEWORK_LIBS@
testtiffleak_LDFLAGS = -static @BOOST_UNIT_TEST_FRAMEWORK_LDFLAGS@

testxmpfiles_SOURCES = test-xmpfiles.cpp utils.cpp
testxmpfiles_LDADD = ../libexempi.la @BOOST_UNIT_TEST_FRAMEWORK_LIBS@
testxmpfiles_LDFLAGS = -static @BOOST_UNIT_TEST_FRAMEWORK_LDFLAGS@

testxmpfileswrite_SOURCES = test-xmpfiles-write.cpp utils.cpp
testxmpfileswrite_LDADD = ../libexempi.la @BOOST_UNIT_TEST_FRAMEWORK_LIBS@
testxmpfileswrite_LDFLAGS = -static @BOOST_UNIT_TEST_FRAMEWORK_LDFLAGS@

test3_SOURCES = test3.cpp utils.cpp
test3_LDADD = ../libexempi.la @BOOST_UNIT_TEST_FRAMEWORK_LIBS@
test3_LDFLAGS = -static @BOOST_UNIT_TEST_FRAMEWORK_LDFLAGS@

testfdo18635_SOURCES = test-bgo.cpp utils.cpp
testfdo18635_LDADD = ../libexempi.la @BOOST_UNIT_TEST_FRAMEWORK_LIBS@
testfdo18635_LDFLAGS = -static @BOOST_UNIT_TEST_FRAMEWORK_LDFLAGS@
