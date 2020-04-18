#!/bin/bash

# Copyright (c) 2011 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

# Calculates the test-coverage percentage for non-test files in the
# gestures directory. Requires a file 'app.info' to contain the
# results of running the unittests while collecting coverage data.

cat $1 | awk -F '[,:]' '

BEGIN { OFS = ":"; }

/^SF:/{ FILEN = $2; }

/^end_of_record$/{ FILEN = ""; }

/^DA:/{ print FILEN, $2, $3; }

' | sort | awk -F : '
BEGIN {
  OFS = ":";
  FILEN = "";
  LINE = "";
  HITS = 0;
}
{
  NEWFILEN = $1;
  NEWLINE = $2;
  if ((NEWFILEN == FILEN) && (NEWLINE == LINE)) {
    HITS += $3
  } else {
    if (FILEN != "") {
      print FILEN, LINE, HITS;
    }
    FILEN = NEWFILEN;
    LINE = NEWLINE;
    HITS = $3;
  }
}
' | grep '^.*\/trunk\/src\/platform\/gestures\/' | \
fgrep -v '_unittest.cc:' | \
fgrep -v '/test_main.cc' | \
fgrep -v '/mock' | \
awk -F : '

function printfile() {
  if (FNAME != "")
    printf "%-40s %4d / %4d: %5.1f%%\n", FNAME, FILE_GOOD_LINES,
        (FILE_BAD_LINES + FILE_GOOD_LINES),
        (FILE_GOOD_LINES * 100) / (FILE_BAD_LINES + FILE_GOOD_LINES);
}

BEGIN {
  FNAME = "";
  FILE_BAD_LINES = 0;
  FILE_GOOD_LINES = 0;
}
{
  // calc filename
  ARR_SIZE = split($1, PARTS, "/");
  NEWFNAME = PARTS[ARR_SIZE];
  if (NEWFNAME != FNAME) {
    printfile();
    FILE_BAD_LINES = 0;
    FILE_GOOD_LINES = 0;
    FNAME = NEWFNAME;
  }
  if ($3 == "0") {
    BAD_LINES += 1;
    FILE_BAD_LINES += 1;
  } else {
    GOOD_LINES += 1;
    FILE_GOOD_LINES += 1;
  }
}

END {
  printfile();
  print "---\nSummary: tested " GOOD_LINES " / " (BAD_LINES + GOOD_LINES);
  printf "Test coverage: %.1f%%\n", \
    ((GOOD_LINES * 100) / (BAD_LINES + GOOD_LINES));
}
'
