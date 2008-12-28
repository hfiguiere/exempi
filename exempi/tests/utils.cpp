/*
 * exempi - utils.cpp
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

#include <cstdio>

#include <boost/test/unit_test.hpp>

#include "utils.h"

std::string g_testfile;
std::string g_src_testdir;
boost::scoped_ptr<LeakTracker> g_lt(new LeakTracker) ;

void prepare_test(int argc, char * argv[], const char *filename)
{
	if (argc == 1) {
		// no argument, lets run like we are in "check"
		const char * srcdir = getenv("TEST_DIR");
		
		BOOST_ASSERT(srcdir != NULL);
		g_src_testdir = std::string(srcdir) + "/";
		g_testfile = g_src_testdir + filename;
	}
	else {
		g_src_testdir = "./";
		g_testfile = argv[1];
	}
}



#ifdef HAVE_VALGRIND_MEMCHECK_H
# include <valgrind/memcheck.h>
# include <valgrind/valgrind.h>
# define CC_EXTENSION __extension__
#else
# define VALGRIND_COUNT_ERRORS 0
# define VALGRIND_DO_LEAK_CHECK
# define VALGRIND_COUNT_LEAKS(a,b,c,d) (a=b=c=d=0)
# define CC_EXTENSION
#endif



LeakTracker::LeakTracker()
	: m_leaks(0),
	  m_dubious(0),
	  m_reachable(0),
	  m_suppressed(0),
	  m_errors(0)
{

}


LeakTracker::~LeakTracker()
{
	printf("LeakTracker: leaked = %d, errors = %d\n", m_leaks, m_errors);
}


int LeakTracker::check_leaks()
{
    int leaked = 0;
	int dubious = 0;
	int reachable = 0;
	int suppressed = 0;

    VALGRIND_DO_LEAK_CHECK;
    VALGRIND_COUNT_LEAKS(leaked, dubious, reachable, suppressed);
	printf("memleaks: sure:%d dubious:%d reachable:%d suppress:%d\n",
		   leaked, dubious, reachable, suppressed);
	bool has_leaks = (m_leaks != leaked);

	m_leaks = leaked;
	m_dubious = dubious;
	m_reachable = reachable;
	m_suppressed = suppressed;

	return has_leaks;
}

int LeakTracker::check_errors()
{
	int errors = (int)(CC_EXTENSION VALGRIND_COUNT_ERRORS);
	bool has_new_errors = (m_errors != errors);
	m_errors = errors;
    return has_new_errors;
}
