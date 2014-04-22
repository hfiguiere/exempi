#!/bin/sh
#
# Test script.
# Currently only make sure dumpmainxmp does not crash
# Written by Hubert Figuiere <hub@figuiere.net>

if [ -z $srcdir ] ; then
    echo "$srcdir srcdir not defined."
    exit 255
fi

SAMPLES="BlueSquare.ai  BlueSquare.eps BlueSquare.gif  BlueSquare.jpg  BlueSquare.mp3  BlueSquare.png  BlueSquare.tif BlueSquare.avi  BlueSquare.indd BlueSquare.pdf  BlueSquare.psd  BlueSquare.wav"
SAMPLES_DIR=$srcdir/../../samples/testfiles
DUMPMAINXMP_PROG=../../samples/source/dumpmainxmp

if [ ! -x $DUMPMAINXMP_PROG ] ; then
    echo "$DUMPMAINXMP_PROG not executable."
    exit 255
fi

for i in $SAMPLES
do
  echo "Running $DUMPMAINXMP_PROG $SAMPLES_DIR/$i"
  $VALGRIND $DUMPMAINXMP_PROG $SAMPLES_DIR/$i > /dev/null
  if [ $? -ne 0 ] ; then
      echo "Failed"
      exit 255
  fi
done
