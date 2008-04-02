AC_PREREQ(2.50)
AC_INIT(exempi/xmp.h)
AM_INIT_AUTOMAKE(exempi, 2.0.1)
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
dnl 1.99.0 is the revision 2 0 0
dnl 1.99.1 is the revision 3 1 0
dnl 1.99.2 is the revision 4 2 0
dnl 1.99.3 is the revision 5 3 0
dnl 1.99.4 is the revision 6 4 0
dnl ABI breakage reset major to 3
dnl 1.99.5 is the revision 3.0.0
dnl 1.99.6 is the revision 3.1.0 WHICH IS WRONG!!!
dnl 1.99.7 is the revision 4.1.0
dnl 1.99.8 is the revision 4.1.1
dnl 1.99.9 is the revision 4.1.1 (no library change have been made)
dnl 2.0.0 is the revision 4.1.2
dnl 2.0.1 is the revision 4.1.3
AC_SUBST([EXEMPI_REVISION], [3])
AC_SUBST([EXEMPI_AGE],      [1])
AC_SUBST([EXEMPI_CURRENT],  [4])
AC_SUBST([EXEMPI_CURRENT_MIN],
         [`expr $EXEMPI_CURRENT - $EXEMPI_AGE`])
AC_SUBST([EXEMPI_VERSION_INFO],
         [$EXEMPI_CURRENT:$EXEMPI_REVISION:$EXEMPI_AGE])

dnl the major version. used for package names, etc
EXEMPI_MAJOR_VERSION=2.0
AC_SUBST(EXEMPI_MAJOR_VERSION)

dnl the path where includes are installed
EXEMPI_INCLUDE_BASE=exempi-$EXEMPI_MAJOR_VERSION
AC_SUBST(EXEMPI_INCLUDE_BASE)

dnl --------------------------------------------------------------------
dnl guess directory to install *.pc into
dnl --------------------------------------------------------------------
pkgconfigdir='${libdir}/pkgconfig'
AC_SUBST(pkgconfigdir)

AC_CONFIG_MACRO_DIR([m4])

AC_PROG_CC
AC_PROG_CXX
AC_PROG_LIBTOOL

AC_C_BIGENDIAN
AC_DEFINE(CHECKED_ENDIANNESS)

AC_ARG_WITH(darwinports,[  --with-darwinports     add /opt/local/... to CPP/LDFLAGS (Mac OSX)],[
        if test "x$withval" != "xno"; then
                CPPFLAGS="$CPPFLAGS -I/opt/local/include"
                LDFLAGS="$LDFLAGS -L/opt/local/lib"
		CC=g++
        fi
])
AC_ARG_WITH(fink,[  --with-fink          add /sw/... to CPP/LDFLAGS (Mac OSX)],[
        if test "x$withval" != "xno"; then
                CPPFLAGS="$CPPFLAGS -I/sw/include"
                LDFLAGS="$LDFLAGS -L/sw/lib"
		CC=g++
        fi
])


AC_CHECK_HEADER(expat.h, ,
	 AC_MSG_ERROR([expat headers missing]))
AC_CHECK_HEADER(iconv.h, ,
	 AC_MSG_ERROR([iconv headers missing]))

AC_ARG_ENABLE(unittest,
              AC_HELP_STRING([--enable-unittest=yes|no],
                            [enable the unittest. requires boost (default is yes)]),
              ENABLE_UNITTEST=$enableval,
              ENABLE_UNITTEST=yes)
if test x$ENABLE_UNITTEST = xyes ; then
   BOOST_REQUIRE([1.33.0])
   BOOST_TEST([mt-s])
else
   ENABLE_UNITTEST=no
fi


case $build_vendor in
	apple)
		EXEMPI_PLATFORM_DEF=MAC_ENV
		CPPFLAGS="$CPPFLAGS -I/Developer/Headers/FlatCarbon"
		LDFLAGS="$LDFLAGS -framework CoreServices"
		;;
	*)
		EXEMPI_PLATFORM_DEF=UNIX_ENV
		;;
esac

AC_SUBST(EXEMPI_PLATFORM_DEF)

AM_CONDITIONAL(WITH_UNIT_TEST, test x$ENABLE_UNITTEST = xyes)

dnl Check for valgrind (optional) for make check.
AC_CHECK_PROG(VALGRIND, valgrind, valgrind)
AC_CHECK_HEADERS(valgrind/memcheck.h)
if test "$VALGRIND" = ""; then
        AC_MSG_WARN([Valgrind is missing. checks will run without])
fi
if test "$VALGRIND" != ""; then
dnl due to stupid m4 I had to double the [ and ] in the regexp
        vg_ver=`valgrind --version | sed -e  's/.*-\([[0-9]]\)\.[[0-9]].*/\1/'`
        VALGRIND="$VALGRIND --tool=memcheck --leak-check=yes --num-callers=10"
        if test "$vg_ver" != "3" ; then
           VALGRIND="$VALGRIND --logfile-fd=-1"
        fi
fi
AC_SUBST(VALGRIND)


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
samples/BlueSquares/Makefile
build/Makefile
public/Makefile
public/include/Makefile
public/include/client-glue/Makefile
exempi/Makefile
exempi/doc/Makefile
exempi/doc/Doxyfile
exempi/tests/Makefile
exempi/exempi-2.0.pc
])


echo "
Defines:

   EXEMPI_PLATFORM_DEF = $EXEMPI_PLATFORM_DEF
   CPPFLAGS            = $CPPFLAGS
   LDFLAGS             = $LDFLAGS
   Run unit test       = $ENABLE_UNITTEST
"
