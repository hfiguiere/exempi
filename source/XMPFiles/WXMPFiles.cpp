// =================================================================================================
// ADOBE SYSTEMS INCORPORATED
// Copyright 2002-2007 Adobe Systems Incorporated
// All Rights Reserved
//
// NOTICE: Adobe permits you to use, modify, and distribute this file in accordance with the terms
// of the Adobe license agreement accompanying it.
// =================================================================================================

#include "XMP_Environment.h"
#include "XMP_Const.h"

#include "client-glue/WXMPFiles.hpp"

#include "XMPFiles_Impl.hpp"
#include "XMPFiles.hpp"

#if XMP_WinBuild
	#if XMP_DebugBuild
		#pragma warning ( disable : 4297 ) // function assumed not to throw an exception but does
	#endif
#endif

#if __cplusplus
extern "C" {
#endif

// =================================================================================================

static WXMP_Result voidResult;	// Used for functions that  don't use the normal result mechanism.

// =================================================================================================

void WXMPFiles_GetVersionInfo_1 ( XMP_VersionInfo * versionInfo )
{
	WXMP_Result * wResult = &voidResult;	// ! Needed to "fool" the EnterWrapper macro.
	XMP_ENTER_WRAPPER_NO_LOCK ( "WXMPFiles_GetVersionInfo_1" )
	
		XMPFiles::GetVersionInfo ( versionInfo );
	
	XMP_EXIT_WRAPPER_NO_THROW
}
    
// -------------------------------------------------------------------------------------------------
    
void WXMPFiles_Initialize_1 ( WXMP_Result * wResult )
{
	XMP_ENTER_WRAPPER_NO_LOCK ( "WXMPFiles_Initialize_1" )
	
		wResult->int32Result = XMPFiles::Initialize ( 0 );
	
	XMP_EXIT_WRAPPER
}
    
// -------------------------------------------------------------------------------------------------
    
void WXMPFiles_Initialize_2 ( XMP_OptionBits options, WXMP_Result * wResult )
{
	XMP_ENTER_WRAPPER_NO_LOCK ( "WXMPFiles_Initialize_1" )
	
		wResult->int32Result = XMPFiles::Initialize ( options );
	
	XMP_EXIT_WRAPPER
}

// -------------------------------------------------------------------------------------------------
    
void WXMPFiles_Terminate_1()
{
	WXMP_Result * wResult = &voidResult;	// ! Needed to "fool" the EnterWrapper macro.
	XMP_ENTER_WRAPPER_NO_LOCK ( "WXMPFiles_Terminate_1" )
	
		XMPFiles::Terminate();
	
	XMP_EXIT_WRAPPER_NO_THROW
}

// =================================================================================================
    
void WXMPFiles_CTor_1 ( WXMP_Result * wResult )
{
	XMP_ENTER_WRAPPER ( "WXMPFiles_CTor_1" )
	
		XMPFiles * newObj = new XMPFiles();
		++newObj->clientRefs;
		XMP_Assert ( newObj->clientRefs == 1 );
		wResult->ptrResult = newObj;
	
	XMP_EXIT_WRAPPER
}

// -------------------------------------------------------------------------------------------------

void WXMPFiles_UnlockLib_1()
{
	WXMP_Result * wResult = &voidResult;	// ! Needed to fool the EnterWrapper macro.
	XMP_ENTER_WRAPPER_NO_LOCK ( "WXMPFiles_UnlockLib_1" )
	
		XMPFiles::UnlockLib();
	
	XMP_EXIT_WRAPPER_NO_THROW
}

// -------------------------------------------------------------------------------------------------

void WXMPFiles_UnlockObj_1 ( XMPFilesRef xmpFilesRef )
{
	WXMP_Result * wResult = &voidResult;	// ! Needed to fool the EnterWrapper macro.
	XMP_ENTER_WRAPPER_NO_LOCK ( "WXMPFiles_UnlockObj_1" )
	
		XMPFiles * thiz = (XMPFiles*)xmpFilesRef;
		thiz->UnlockObj();
	
	XMP_EXIT_WRAPPER_NO_THROW
}

// -------------------------------------------------------------------------------------------------

void WXMPFiles_IncrementRefCount_1 ( XMPFilesRef xmpFilesRef )
{
	WXMP_Result * wResult = &voidResult;	// ! Needed to "fool" the EnterWrapper macro.
	XMP_ENTER_WRAPPER ( "WXMPFiles_IncrementRefCount_1" )
	
		XMPFiles * thiz = (XMPFiles*)xmpFilesRef;
		++thiz->clientRefs;
		XMP_Assert ( thiz->clientRefs > 0 );
	
	XMP_EXIT_WRAPPER_NO_THROW
}

// -------------------------------------------------------------------------------------------------

void WXMPFiles_DecrementRefCount_1 ( XMPFilesRef xmpFilesRef )
{
	WXMP_Result * wResult = &voidResult;	// ! Needed to "fool" the EnterWrapper macro.
	XMP_ENTER_WRAPPER ( "WXMPFiles_DecrementRefCount_1" )
	
		XMPFiles * thiz = (XMPFiles*)xmpFilesRef;
		XMP_Assert ( thiz->clientRefs > 0 );
		--thiz->clientRefs;
		if ( thiz->clientRefs <= 0 ) delete ( thiz );
	
	XMP_EXIT_WRAPPER_NO_THROW
}

// =================================================================================================

void WXMPFiles_GetFormatInfo_1 ( XMP_FileFormat   format,
                                 XMP_OptionBits * flags,
                                 WXMP_Result *    wResult )
{
	XMP_ENTER_WRAPPER ( "WXMPFiles_GetFormatInfo_1" )
	
		wResult->int32Result = XMPFiles::GetFormatInfo ( format, flags );
	
	XMP_EXIT_WRAPPER
}

// =================================================================================================

void WXMPFiles_OpenFile_1 ( XMPFilesRef    xmpFilesRef,
                            XMP_StringPtr  filePath,
			                XMP_FileFormat format,
			                XMP_OptionBits openFlags,
                            WXMP_Result *  wResult )
{
	XMP_ENTER_WRAPPER ( "WXMPFiles_OpenFile_1" )
		StartPerfCheck ( kAPIPerf_OpenFile, filePath );
	
		XMPFiles * thiz = (XMPFiles*)xmpFilesRef;
		bool ok = thiz->OpenFile ( filePath, format, openFlags );
		wResult->int32Result = ok;
	
		EndPerfCheck ( kAPIPerf_OpenFile );
	XMP_EXIT_WRAPPER
}
    
// -------------------------------------------------------------------------------------------------

void WXMPFiles_CloseFile_1 ( XMPFilesRef    xmpFilesRef,
                             XMP_OptionBits closeFlags,
                             WXMP_Result *  wResult )
{
	XMP_ENTER_WRAPPER ( "WXMPFiles_CloseFile_1" )
		StartPerfCheck ( kAPIPerf_CloseFile, "" );
	
		XMPFiles * thiz = (XMPFiles*)xmpFilesRef;
		thiz->CloseFile ( closeFlags );
	
		EndPerfCheck ( kAPIPerf_CloseFile );
	XMP_EXIT_WRAPPER
}
	
// -------------------------------------------------------------------------------------------------

void WXMPFiles_GetFileInfo_1 ( XMPFilesRef      xmpFilesRef,
                               XMP_StringPtr *  filePath,
                               XMP_StringLen *  filePathLen,
			                   XMP_OptionBits * openFlags,
			                   XMP_FileFormat * format,
			                   XMP_OptionBits * handlerFlags,
                               WXMP_Result *    wResult )
{
	bool isOpen = false;
	XMP_ENTER_WRAPPER ( "WXMPFiles_GetFileInfo_1" )
	
		XMPFiles * thiz = (XMPFiles*)xmpFilesRef;
		isOpen = thiz->GetFileInfo ( filePath, filePathLen, openFlags, format, handlerFlags );
		wResult->int32Result = isOpen;
	
	XMP_EXIT_WRAPPER_KEEP_LOCK ( isOpen )
}
    
// -------------------------------------------------------------------------------------------------

void WXMPFiles_SetAbortProc_1 ( XMPFilesRef   xmpFilesRef,
                         	    XMP_AbortProc abortProc,
							    void *        abortArg,
							    WXMP_Result * wResult )
{
	XMP_ENTER_WRAPPER ( "WXMPFiles_SetAbortProc_1" )
	
		XMPFiles * thiz = (XMPFiles*)xmpFilesRef;
		thiz->SetAbortProc ( abortProc, abortArg );
	
	XMP_EXIT_WRAPPER
}
    
// -------------------------------------------------------------------------------------------------

void WXMPFiles_GetXMP_1 ( XMPFilesRef      xmpFilesRef,
                          XMPMetaRef       xmpRef,
		                  XMP_StringPtr *  xmpPacket,
		                  XMP_StringLen *  xmpPacketLen,
		                  XMP_PacketInfo * packetInfo,
                          WXMP_Result *    wResult )
{
	bool hasXMP = false;
	XMP_ENTER_WRAPPER ( "WXMPFiles_GetXMP_1" )
		StartPerfCheck ( kAPIPerf_GetXMP, "" );

		XMPFiles * thiz = (XMPFiles*)xmpFilesRef;
		if ( xmpRef == 0 ) {
			hasXMP = thiz->GetXMP ( 0, xmpPacket, xmpPacketLen, packetInfo );
		} else {
			SXMPMeta xmpObj ( xmpRef );
			hasXMP = thiz->GetXMP ( &xmpObj, xmpPacket, xmpPacketLen, packetInfo );
		}
		wResult->int32Result = hasXMP;
	
		EndPerfCheck ( kAPIPerf_GetXMP );
	XMP_EXIT_WRAPPER_KEEP_LOCK ( hasXMP )
}
    
// -------------------------------------------------------------------------------------------------

void WXMPFiles_GetThumbnail_1 ( XMPFilesRef         xmpFilesRef,
    			                XMP_ThumbnailInfo * tnailInfo,	// ! Can be null.
                                WXMP_Result *       wResult )
{
	XMP_ENTER_WRAPPER ( "WXMPFiles_GetThumbnail_1" )
		StartPerfCheck ( kAPIPerf_GetThumbnail, "" );

		XMPFiles * thiz = (XMPFiles*)xmpFilesRef;
		bool hasTNail = thiz->GetThumbnail ( tnailInfo );
		wResult->int32Result = hasTNail;
	
		EndPerfCheck ( kAPIPerf_GetThumbnail );
	XMP_EXIT_WRAPPER	// ! No need to keep the lock, the tnail info won't change.
}
    
// -------------------------------------------------------------------------------------------------

void WXMPFiles_PutXMP_1 ( XMPFilesRef   xmpFilesRef,
                          XMPMetaRef    xmpRef,	// ! Only one of the XMP object or packet are passed.
                          XMP_StringPtr xmpPacket,
                          XMP_StringLen xmpPacketLen,
                          WXMP_Result * wResult )
{
	XMP_ENTER_WRAPPER ( "WXMPFiles_PutXMP_1" )
		StartPerfCheck ( kAPIPerf_PutXMP, "" );
	
		XMPFiles * thiz = (XMPFiles*)xmpFilesRef;
		if ( xmpRef != 0 ) {
			thiz->PutXMP ( xmpRef );
		} else {
			thiz->PutXMP ( xmpPacket, xmpPacketLen );
		}
	
		EndPerfCheck ( kAPIPerf_PutXMP );
	XMP_EXIT_WRAPPER
}
    
// -------------------------------------------------------------------------------------------------

void WXMPFiles_CanPutXMP_1 ( XMPFilesRef   xmpFilesRef,
                             XMPMetaRef    xmpRef,	// ! Only one of the XMP object or packet are passed.
                             XMP_StringPtr xmpPacket,
                             XMP_StringLen xmpPacketLen,
                             WXMP_Result * wResult )
{
	XMP_ENTER_WRAPPER ( "WXMPFiles_CanPutXMP_1" )
		StartPerfCheck ( kAPIPerf_CanPutXMP, "" );
	
		XMPFiles * thiz = (XMPFiles*)xmpFilesRef;
		if ( xmpRef != 0 ) {
			wResult->int32Result = thiz->CanPutXMP ( xmpRef );
		} else {
			wResult->int32Result = thiz->CanPutXMP ( xmpPacket, xmpPacketLen );
		}
	
		EndPerfCheck ( kAPIPerf_CanPutXMP );
	XMP_EXIT_WRAPPER
}

// =================================================================================================

#if __cplusplus
}
#endif
