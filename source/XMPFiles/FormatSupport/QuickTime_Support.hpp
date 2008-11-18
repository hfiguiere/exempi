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

#include "XMP_Environment.h"		// ! This must be the first include.
#if ! ( XMP_64 || XMP_UNIXBuild)	//	Closes at very bottom.

namespace QuickTime_Support 
{
	extern bool sMainInitOK;
	
	bool MainInitialize ( bool ignoreInit );	// For the main thread.
	void MainTerminate ( bool ignoreInit );
	
	bool ThreadInitialize();	// For background threads.
	void ThreadTerminate();

} // namespace QuickTime_Support

#endif	// XMP_64 || XMP_UNIXBuild
#endif	// __QuickTime_Support_hpp__
