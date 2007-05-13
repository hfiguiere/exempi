AC_PREREQ(2.50)
AC_INIT(source/XMPCore/WXMPMeta.cpp)
AM_INIT_AUTOMAKE(openxmp, 4.1.1)
AM_MAINTAINER_MODE


dnl ---------------------------------------------------------------------------
dnl Advanced information about versioning:
dnl   * "Writing shared libraries" by Mike Hearn
dnl         http://navi.cx/~mike/writing-shared-libraries.html
dnl   * libtool.info chapter "Versioning"
dnl   * libtool.info chapter "Updating library version information"
dnl ---------------------------------------------------------------------------
dnl Versioning:
dnl  - CURRENT (Major):  Increment if the interface has changes. AGE is always
dnl                      *changed* at the same time.
dnl  - AGE (Micro):      Increment if any interfaces have been added; set to 0
dnl                      if any interfaces have been removed. Removal has
dnl                      precedence over adding, so set to 0 if both happened.
dnl                      It denotes upward compatibility.
dnl  - REVISION (Minor): Increment any time the source changes; set to
dnl                      0 if you incremented CURRENT.
dnl
dnl  To summarize. Any interface *change* increment CURRENT. If that interface
dnl  change does not break upward compatibility (ie it is an addition),
dnl  increment AGE, Otherwise AGE is reset to 0. If CURRENT has changed,
dnl  REVISION is set to 0, otherwise REVISION is incremented.
dnl ---------------------------------------------------------------------------
dnl 4.1.1 is the first revision 0 0 0
AC_SUBST([LIBXMPCORE_REVISION], [0])
AC_SUBST([LIBXMPCORE_AGE],      [0])
AC_SUBST([LIBXMPCORE_CURRENT],  [0])
AC_SUBST([LIBXMPCORE_CURRENT_MIN],
         [`expr $LIBXMPCORE_CURRENT - $LIBXMPCORE_AGE`])
AC_SUBST([LIBXMPCORE_VERSION_INFO],
         [$LIBXMPCORE_CURRENT:$LIBXMPCORE_REVISION:$LIBXMPCORE_AGE])

AC_PROG_CC
AC_PROG_CXX
AC_PROG_LIBTOOL

AC_CHECK_HEADER(expat.h, ,
	 AC_MSG_ERROR([expat headers missing]))
AC_CHECK_HEADER(iconv.h, ,
	 AC_MSG_ERROR([iconv headers missing]))


AC_OUTPUT([
Makefile
third-party/Makefile
third-party/MD5/Makefile
source/Makefile
source/common/Makefile
source/XMPCore/Makefile
source/XMPFiles/Makefile
source/XMPFiles/FileHandlers/Makefile
source/XMPFiles/FormatSupport/Makefile
samples/Makefile
samples/source/Makefile
])
