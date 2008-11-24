#
# exempi - Makefile.am
#
# Copyright (C) 2007 Hubert Figuiere
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
check_PROGRAMS = test1 test2 test3 testinit testfdo18635
TESTS = testcore.sh testinit test1 test2 test3 testfdo18635
TESTS_ENVIRONMENT = TEST_DIR=$(srcdir) @VALGRIND@
endif
check_SCRIPTS = testcore.sh

check_DATA = test1.xmp fdo18635.jpg

noinst_HEADERS = utils.h

EXTRA_DIST = $(check_DATA) $(check_SCRIPTS)
CLEANFILES = test.jpg


testinit_SOURCES = testinit.cpp utils.cpp
testinit_LDADD = ../libexempi.la @BOOST_UNIT_TEST_FRAMEWORK_LIBS@
testinit_LDFLAGS = -static @BOOST_UNIT_TEST_FRAMEWORK_LDFLAGS@

test1_SOURCES = test1.cpp utils.cpp
test1_LDADD = ../libexempi.la @BOOST_UNIT_TEST_FRAMEWORK_LIBS@
test1_LDFLAGS = -static @BOOST_UNIT_TEST_FRAMEWORK_LDFLAGS@

test2_SOURCES = test2.cpp utils.cpp
test2_LDADD = ../libexempi.la @BOOST_UNIT_TEST_FRAMEWORK_LIBS@
test2_LDFLAGS = -static @BOOST_UNIT_TEST_FRAMEWORK_LDFLAGS@

test3_SOURCES = test3.cpp utils.cpp
test3_LDADD = ../libexempi.la @BOOST_UNIT_TEST_FRAMEWORK_LIBS@
test3_LDFLAGS = -static @BOOST_UNIT_TEST_FRAMEWORK_LDFLAGS@

testfdo18635_SOURCES = test-bgo.cpp utils.cpp
testfdo18635_LDADD = ../libexempi.la @BOOST_UNIT_TEST_FRAMEWORK_LIBS@
testfdo18635_LDFLAGS = -static @BOOST_UNIT_TEST_FRAMEWORK_LDFLAGS@
