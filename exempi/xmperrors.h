/*
 * exempi - xmperror.h
 *
 * Copyright (C) 2007 Hubert Figuiere
 * Copyright 2002-2007 Adobe Systems Incorporated
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



#ifndef __EXEMPI_XMPERROR_H_
#define __EXEMPI_XMPERROR_H_


enum {
	/* More or less generic error codes. */
	XMPErr_Unknown          =   0,
	XMPErr_TBD              =   -1,
	XMPErr_Unavailable      =   -2,
	XMPErr_BadObject        =   -3,
	XMPErr_BadParam         =   -4,
	XMPErr_BadValue         =   -5,
	XMPErr_AssertFailure    =   -6,
	XMPErr_EnforceFailure   =   -7,
	XMPErr_Unimplemented    =   -8,
	XMPErr_InternalFailure  =   -9,
	XMPErr_Deprecated       =  -10,
	XMPErr_ExternalFailure  =  -11,
	XMPErr_UserAbort        =  -12,
	XMPErr_StdException     =  -13,
	XMPErr_UnknownException =  -14,
	XMPErr_NoMemory         =  -15,
	
	/* More specific parameter error codes.  */
	XMPErr_BadSchema        = -101,
	XMPErr_BadXPath         = -102,
	XMPErr_BadOptions       = -103,
	XMPErr_BadIndex         = -104,
	XMPErr_BadIterPosition  = -105,
	XMPErr_BadParse         = -106,
	XMPErr_BadSerialize     = -107,
	XMPErr_BadFileFormat    = -108,
	XMPErr_NoFileHandler    = -109,
	XMPErr_TooLargeForJPEG  = -110,
	
	/* File format and internal structure error codes. */
	XMPErr_BadXML           = -201,
	XMPErr_BadRDF           = -202,
	XMPErr_BadXMP           = -203,
	XMPErr_EmptyIterator    = -204,
	XMPErr_BadUnicode       = -205,
	XMPErr_BadTIFF          = -206,
	XMPErr_BadJPEG          = -207,
	XMPErr_BadPSD           = -208,
	XMPErr_BadPSIR          = -209,
	XMPErr_BadIPTC          = -210,
	XMPErr_BadMPEG          = -211
};

#endif
