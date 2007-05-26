

INCLUDES = -I$(srcdir)/.. 

check_PROGRAMS = test1 test2

check_SCRIPTS = testcore.sh

check_DATA = test1.xmp

EXTRA_DIST = $(check_DATA)

TESTS = testcore.sh test1 test2

test1_SOURCES = test1.cpp
test1_LDADD = ../libexempi.la @BOOST_UNIT_TEST_FRAMEWORK_LIB@
test1_LDFLAGS = -static

test2_SOURCES = test2.cpp
test2_LDADD = ../libexempi.la @BOOST_UNIT_TEST_FRAMEWORK_LIB@
test2_LDFLAGS = -static