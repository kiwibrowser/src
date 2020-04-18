/*
 * Copyright 2010 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <string>

#include "native_client/src/trusted/debug_stub/util.h"

using std::string;
using gdb_rsp::NibbleToInt;
using gdb_rsp::IntToNibble;
using gdb_rsp::stringvec;


int TestUtil() {
  char goodStr[] = "0123456789abcdefABCDEF";
  int good;
  int val;
  int a;
  int errCnt = 0;

  // Make sure we find expected and only expected conversions (0-9,a-f,A-F)
  good = 0;
  for (a = 0; a < 256; a++) {
    // NOTE:  Exclude 'zero' since the terminator is found by strchr
    bool found = a && (strrchr(goodStr, a) != NULL);
    bool xvert = NibbleToInt(static_cast<char>(a), &val);
    if (xvert != found) {
      printf("FAILED NibbleToInt of '%c'(#%d), convertion %s.\n",
        a, a, xvert ? "succeeded" : "failed");
      errCnt++;
    }
  }

  good = 0;
  for (a = -256; a < 256; a++) {
    char ch;
    bool xvert = IntToNibble(a, &ch);

    if (xvert) {
      good++;
      if (!NibbleToInt(ch, &val)) {
        printf("FAILED IntToNibble on good NibbleToInt of #%d\n.\n", a);
        errCnt++;
      }
      // NOTE: check if IntToNible matches NibbleToInt looking at both
      // possitive and negative values of -15 to +15
      if (val != a) {
        printf("FAILED IntToNibble != NibbleToInt of #%d\n.\n", a);
        errCnt++;
      }
    }
  }

  // Although we check -15 to +15 above, we realy only want -7 to +15
  // to verify unsiged (0-15) plus signed (-7 to -1).
  if (good != 16) {
        printf("FAILED IntToNibble range of 0 to 15.\n");
        errCnt++;
  }

  string xml = "qXfer:features:read:target.xml:0,7ca";
  string str;
  stringvec vec, vec2;

  return errCnt;
}

