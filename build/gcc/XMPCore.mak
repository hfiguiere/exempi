# ==================================================================================================
# Copyright 2002-2004 Adobe Systems Incorporated
# All Rights Reserved.
#
# NOTICE:  Adobe permits you to use, modify, and distribute this file in accordance with the terms
# of the Adobe license agreement accompanying it.
# ==================================================================================================

# ==================================================================================================

# Define internal use variables.

Error =
TargetOS = ${OS}

ifeq "${TargetOS}" ""
	TargetOS = ${os}
endif

ifeq "${TargetOS}" ""
	TargetOS = ${MACHTYPE}${OSTYPE}
endif

ifeq "${TargetOS}" "i386linux"	# Linux ${MACHTYPE}${OSTYPE} is i386linux.
	TargetOS = i80386linux
endif

ifeq "${TargetOS}" "linux"
	TargetOS = i80386linux
endif

ifeq "${TargetOS}" "solaris"
	TargetOS = sparcsolaris
endif

ifneq "${TargetOS}" "i80386linux"
	ifneq "${TargetOS}" "sparcsolaris"
		Error += Invalid target OS "${TargetOS}"
	endif
endif

TargetStage = ${STAGE}

ifeq "${TargetStage}" ""
	TargetStage = ${stage}
endif

ifeq "${TargetStage}" ""
	TargetStage = debug
endif

ifneq "${TargetStage}" "debug"
	ifneq "${TargetStage}" "release"
		Error += Invalid target stage "${TargetStage}"
	endif
endif

ifeq "${TargetStage}" "debug"
   LibSuffix = StaticDebug
endif

ifeq "${TargetStage}" "release"
   LibSuffix = StaticRelease
endif

BuildRoot  = ../..
TargetRoot = ${BuildRoot}/public/libraries/${TargetOS}/${TargetStage}
TempRoot   = ${BuildRoot}/intermediate/${TargetOS}/${TargetStage}

HeaderRoot = ${BuildRoot}/public/include
SourceRoot = ${BuildRoot}/source
ExpatRoot  = ${BuildRoot}/third-party/expat
MD5Root    = ${BuildRoot}/third-party/MD5

LibName    = ${TargetRoot}/libXMPCore${LibSuffix}.a

# ==================================================================================================

CC  = gcc
CPP = gcc -x c++
AR  = ar -rs

CPPFLAGS = -Wno-multichar -Wno-implicit -Wno-ctor-dtor-privacy -funsigned-char -fexceptions
CPPFLAGS += -DUNIX_ENV=1 -DXMP_IMPL=1 -DXMP_ClientBuild=0 -D_FILE_OFFSET_BITS=64 -DHAVE_EXPAT_CONFIG_H=1 -DXML_STATIC=1

ifeq "${TargetOS}" "i80386linux"
	CPPFLAGS += -mtune=i686
endif

ifeq "${TargetOS}" "sparcsolaris"
	CPPFLAGS += -mtune=ultrasparc
endif

ifeq "$(TargetStage)" "debug"
	CPPFLAGS += -DDEBUG=1 -D_DEBUG=1 -g -O0
endif

ifeq "$(TargetStage)" "release"
	CPPFLAGS += -DNDEBUG=1 -O2 -Os
endif

# ==================================================================================================

CPPObjs = $(foreach objs,${CPPSources:.cpp=.o},${TempRoot}/$(objs))
CCObjs  = $(foreach objs,${CCSources:.c=.o},${TempRoot}/$(objs))

vpath %.incl_cpp \
    ${HeaderRoot}: \
    ${HeaderRoot}/client-glue:

vpath %.cpp \
    ${SourceRoot}/XMPCore: \
    ${SourceRoot}/common: \
    ${HeaderRoot}: \
    ${HeaderRoot}/client-glue: \
    ${ExpatRoot}/lib: \
    ${MD5Root}:

vpath %.c \
    ${SourceRoot}/XMPCore: \
    ${HeaderRoot}: \
    ${HeaderRoot}/client-glue: \
    ${ExpatRoot}/lib:

CPPSources =  \
    XMPMeta.cpp \
    XMPMeta-GetSet.cpp \
    XMPMeta-Parse.cpp \
    XMPMeta-Serialize.cpp \
    XMPIterator.cpp \
    XMPUtils.cpp \
    XMPUtils-FileInfo.cpp \
    XMPCore_Impl.cpp \
    ExpatAdapter.cpp \
    ParseRDF.cpp \
    UnicodeConversions.cpp \
    MD5.cpp \
    WXMPMeta.cpp \
    WXMPIterator.cpp \
    WXMPUtils.cpp

CCSources = \
    xmlparse.c \
    xmlrole.c \
    xmltok.c

Includes = \
   -I${HeaderRoot} \
   -I${SourceRoot}/XMPCore \
   -I${SourceRoot}/common \
   -I${BuildRoot}/build \
   -I${BuildRoot}/build/gcc/${TargetOS} \
   -I${ExpatRoot}/lib \
   -I${MD5Root}

.SUFFIXES:                # Delete the default suffixes
.SUFFIXES: .o .c .cpp     # Define our suffix list

# ==================================================================================================

${TempRoot}/%.o : %.c
	@echo ""
	@echo "Compiling $<"
	${CC} ${CPPFLAGS} ${Includes} -c $< -o $@

${TempRoot}/%.o : %.cpp
	@echo ""
	@echo "Compiling $<"
	${CPP} ${CPPFLAGS} ${Includes} -c $< -o $@

# ==================================================================================================

.PHONY: all msg create_dirs

all : msg create_dirs ${LibName}

msg :
ifeq "${Error}" ""
	@echo ""
	@echo Building XMP toolkit for ${TargetOS} ${TargetStage}
else
	@echo ""
	@echo "Error: ${Error}"
	@echo ""
	@echo "# To build the XMP Toolkit :"
	@echo "#   make -f XMPCore.mak [OS=<os>] [STAGE=<stage>]"
	@echo "# where"
	@echo "#   OS    = i80386linux | sparcsolaris"
	@echo "#   STAGE = debug | release"
	@echo "#"
	@echo "# The OS and STAGE symbols can also be lowercase, os and stage."
	@echo "# This makefile is only for Linux and Solaris, AIX and HPUX do"
	@echo "# not use gcc, their makefiles are in other build folders. If" 
	@echo "# the OS is omitted it will try to default from the OSTYPE and"
	@echo "# MACHTYPE environment variables. If the stage is omitted it"
	@echo "# defaults to debug."
	@echo ""
	exit 1
endif

create_dirs :
	mkdir -p ${TempRoot}
	mkdir -p ${TargetRoot}

${LibName} : ${CCObjs} ${CPPObjs}
	@echo ""
	@echo "Linking $@"
	rm -f $@
	${AR}  $@ $?
	@echo ""

clean : msg
	rm -f ${TempRoot}/* ${TargetRoot}/*
