# =================================================================================================
# ADOBE SYSTEMS INCORPORATED
# Copyright 2013 Adobe Systems Incorporated
# All Rights Reserved
#
# NOTICE: Adobe permits you to use, modify, and distribute this file in accordance with the terms
# of the Adobe license agreement accompanying it.
# =================================================================================================

# define minimum cmake version
cmake_minimum_required(VERSION 2.8.6)

# ==============================================================================
# Adding Project Name
# ==============================================================================
project (XMPIterations)

# ==============================================================================
file (GLOB SOURCE_FILES ${SAMPLE_SOURCE_ROOT}/XMPIterations.cpp)


source_group(DumpFile FILES ${SOURCE_FILES})
source_group(CommonFiles FILES ${COMMON_FILES})
include_directories( ${XMP_SDK_ROOT} )
include_directories( ${PUBLIC_INCLUDE} )

add_executable(${PROJECT_NAME} ${SOURCE_FILES} )

#setting up XMP_BUILDMODE_DIR variable
SetupInternalBuildDirectory()
#addding XMP libs and setting output path
if(UNIX)
    if(APPLE) #For Mac
		target_link_libraries(${PROJECT_NAME} ${XMP_SDK_ROOT}/public/libraries/${PLATFORM_FOLDER}/${XMP_BUILDMODE_DIR}/lib${XMPCORE_LIB}${LIB_EXT} ${XMP_SDK_ROOT}/public/libraries/${PLATFORM_FOLDER}/${XMP_BUILDMODE_DIR}/lib${XMPFILES_LIB}${LIB_EXT} )
		set(OUTPUT_DIR ${SAMPLE_SOURCE_ROOT}/../target/${PLATFORM_FOLDER}/ ) 
		set(EXECUTABLE_OUTPUT_PATH ${OUTPUT_DIR})
	else(APPLE) #For Linux
        SetPlatformLinkFlags(${PROJECT_NAME} "" "")
		target_link_libraries(${PROJECT_NAME} ${XMP_SDK_ROOT}/public/libraries/${PLATFORM_FOLDER}/${XMP_BUILDMODE_DIR}/${XMPCORE_LIB}${LIB_EXT} ${XMP_SDK_ROOT}/public/libraries/${PLATFORM_FOLDER}/${XMP_BUILDMODE_DIR}/${XMPFILES_LIB}${LIB_EXT} )
		set(OUTPUT_DIR ${SAMPLE_SOURCE_ROOT}/../target/${PLATFORM_FOLDER}/${XMP_BUILDMODE_DIR} ) 
		set(EXECUTABLE_OUTPUT_PATH ${OUTPUT_DIR})		
	endif(APPLE)	
else(UNIX) #For Windows
    target_link_libraries(${PROJECT_NAME} ${XMP_SDK_ROOT}/public/libraries/${PLATFORM_FOLDER}/${XMP_BUILDMODE_DIR}/${XMPCORE_LIB}${LIB_EXT} ${XMP_SDK_ROOT}/public/libraries/${PLATFORM_FOLDER}/${XMP_BUILDMODE_DIR}/${XMPFILES_LIB}${LIB_EXT})	
	set(OUTPUT_DIR ${SAMPLE_SOURCE_ROOT}/../target/${PLATFORM_FOLDER}/ ) 
	set(EXECUTABLE_OUTPUT_PATH ${OUTPUT_DIR})
endif(UNIX)

#adding Cocoa for Mac
ADD_FRAMEWORK(Cocoa ${PROJECT_NAME})



