#!/bin/sh
# fontconfig/test/run-test.sh
#
# Copyright © 2000 Keith Packard
#
# Permission to use, copy, modify, distribute, and sell this software and its
# documentation for any purpose is hereby granted without fee, provided that
# the above copyright notice appear in all copies and that both that
# copyright notice and this permission notice appear in supporting
# documentation, and that the name of the author(s) not be used in
# advertising or publicity pertaining to distribution of the software without
# specific, written prior permission.  The authors make no
# representations about the suitability of this software for any purpose.  It
# is provided "as is" without express or implied warranty.
#
# THE AUTHOR(S) DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE,
# INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO
# EVENT SHALL THE AUTHOR(S) BE LIABLE FOR ANY SPECIAL, INDIRECT OR
# CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE,
# DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER
# TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
# PERFORMANCE OF THIS SOFTWARE.
case "$OSTYPE" in
    msys ) MyPWD=`pwd -W` ;;  # On Msys/MinGW, returns a MS Windows style path.
    *    ) MyPWD=`pwd`    ;;  # On any other platforms, returns a Unix style path.
esac

TESTDIR=${srcdir-"$MyPWD"}

FONTDIR="$MyPWD"/fonts
CACHEDIR="$MyPWD"/cache.dir
EXPECTED=${EXPECTED-"out.expected"}

ECHO=true

FCLIST=../fc-list/fc-list$EXEEXT
FCCACHE=../fc-cache/fc-cache$EXEEXT

FONT1=$TESTDIR/4x6.pcf
FONT2=$TESTDIR/8x16.pcf

check () {
  $FCLIST - family pixelsize | sort > out
  echo "=" >> out
  $FCLIST - family pixelsize | sort >> out
  echo "=" >> out
  $FCLIST - family pixelsize | sort >> out
  tr -d '\015' <out >out.tmp; mv out.tmp out
  if cmp out $TESTDIR/$EXPECTED > /dev/null ; then : ; else
    echo "*** Test failed: $TEST"
    echo "*** output is in 'out', expected output in '$EXPECTED'"
    exit 1
  fi
  rm out
}

prep() {
  rm -rf $CACHEDIR
  rm -rf $FONTDIR
  mkdir $FONTDIR
}

dotest () {
  TEST=$1
  test x$VERBOSE = x || echo Running: $TEST
}

sed "s!@FONTDIR@!$FONTDIR!
s!@CACHEDIR@!$CACHEDIR!" < $TESTDIR/fonts.conf.in > fonts.conf

FONTCONFIG_FILE="$MyPWD"/fonts.conf
export FONTCONFIG_FILE

dotest "Basic check"
prep
cp $FONT1 $FONT2 $FONTDIR
check

dotest "With a subdir"
prep
cp $FONT1 $FONT2 $FONTDIR
$FCCACHE $FONTDIR
check

dotest "Subdir with a cache file"
prep
mkdir $FONTDIR/a
cp $FONT1 $FONT2 $FONTDIR/a
$FCCACHE $FONTDIR/a
check

dotest "Complicated directory structure"
prep
mkdir $FONTDIR/a
mkdir $FONTDIR/a/a
mkdir $FONTDIR/b
mkdir $FONTDIR/b/a
cp $FONT1 $FONTDIR/a
cp $FONT2 $FONTDIR/b/a
check

dotest "Subdir with an out-of-date cache file"
prep
mkdir $FONTDIR/a
$FCCACHE $FONTDIR/a
sleep 1
cp $FONT1 $FONT2 $FONTDIR/a
check

dotest "Dir with an out-of-date cache file"
prep
cp $FONT1 $FONTDIR
$FCCACHE $FONTDIR
sleep 1
mkdir $FONTDIR/a
cp $FONT2 $FONTDIR/a
check

rm -rf $FONTDIR $CACHEFILE $CACHEDIR $FONTCONFIG_FILE out
