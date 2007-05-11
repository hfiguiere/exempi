#ifndef __QuickTime_Support_hpp__
#define __QuickTime_Support_hpp__ 1

// =================================================================================================
// ADOBE SYSTEMS INCORPORATED
// Copyright 2002-2007 Adobe Systems Incorporated
// All Rights Reserved
//
// NOTICE: Adobe permits you to use, modify, and distribute this file in accordance with the terms
// of the Adobe license agreement accompanying it.
// =================================================================================================

#include "XMP_Environment.h"	// ! This must be the first include.

namespace QuickTime_Support 
{
	extern bool sMainInitOK;
	
	bool MainInitialize();	// For the main thread.
	void MainTerminate();
	
	bool ThreadInitialize();	// For background threads.
	void ThreadTerminate();

} // namespace QuickTime_Support

#endif	// __QuickTime_Support_hpp__
