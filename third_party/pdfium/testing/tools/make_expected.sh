#!/bin/bash
#
# Copyright 2015 PDFium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.
#
# Script to generate expected result files.

# Do this before "set -e" so "which" failing is not fatal.
PNGOPTIMIZER="$(which optipng)"

set -e
while (( "$#" )); do
  INFILE="$1"
  echo $INFILE | grep -qs ' ' && echo space in filename detected && exit 1
  out/Debug/pdfium_test --png $INFILE
  RESULTS="$INFILE.*.png"
  for RESULT in $RESULTS ; do
      EXPECTED=`echo -n $RESULT | sed 's/[.]pdf[.]/_expected.pdf./'`
      mv $RESULT $EXPECTED
      if [ -n "$PNGOPTIMIZER" ]; then
        "$PNGOPTIMIZER" $EXPECTED
      fi
  done
  shift
done
