#!/bin/sh

#
# part of libopenraw
#


topsrcdir=`dirname $0`
if test x$topsrcdir = x ; then
        topsrcdir=.
fi

builddir=`pwd`

AUTOCONF=autoconf

LIBTOOL=$(command -v glibtool)
if [ -z "$LIBTOOL" ]; then
    LIBTOOL=libtool
fi

LIBTOOLIZE=$(command -v glibtoolize)
if [ -z "$LIBTOOLIZE" ]; then
    LIBTOOLIZE=libtoolize
fi


AUTOMAKE=automake
ACLOCAL=aclocal

cd $topsrcdir

rm -f autogen.err
$LIBTOOLIZE --force
$ACLOCAL -I m4 >> autogen.err 2>&1

#autoheader --force
$AUTOMAKE --add-missing --copy --foreign 
$AUTOCONF

cd $builddir


if [ "$NOCONFIGURE" = "" ]; then
    if test -z "$*"; then
        echo "I am going to run ./configure with --enable-maintainer-mode"
	echo "If you wish to pass any to it, please specify them on "
	echo "the $0 command line."
    fi
    echo "Running configure..."
    $topsrcdir/configure --enable-maintainer-mode "$@"
fi
