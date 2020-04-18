/*
 * Copyright 2010 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>

#include <string>
#include <vector>

#include "native_client/src/trusted/debug_stub/util.h"

using std::string;

namespace gdb_rsp {

bool NibbleToInt(char ch, int *val) {
  // Check for nibble of a-f
  if ((ch >= 'a') && (ch <= 'f')) {
    if (val) *val = (ch - 'a' + 10);
    return true;
  }

  // Check for nibble of A-F
  if ((ch >= 'A') && (ch <= 'F')) {
    if (val) *val = (ch - 'A' + 10);
    return true;
  }

  // Check for nibble of 0-9
  if ((ch >= '0') && (ch <= '9')) {
    if (val) *val = (ch - '0');
    return true;
  }

  // Not a valid nibble representation
  return false;
}

bool IntToNibble(int nibble, char *ch) {
  // Verify this value fits in a nibble
  if (nibble != (nibble & 0xF)) return false;

  nibble &= 0xF;
  if (nibble < 10) {
    if (ch) *ch =  '0' + nibble;
  } else {
    // Although uppercase maybe more readible GDB
    // expects lower case values.
    if (ch) *ch = 'a' + (nibble - 10);
  }

  return true;
}

bool NibblesToByte(const char *inStr, int *outInt) {
  int o1, o2;

  if (NibbleToInt(inStr[0], &o1) && NibbleToInt(inStr[1], &o2)) {
    *outInt = (o1 << 4) + o2;
    return true;
  }

  return false;
}

stringvec StringSplit(const string &instr, const char *delim) {
  int count = 0;
  const char *in = instr.data();
  const char *start = in;

  stringvec ovec;

  // Check if we have nothing to do
  if (NULL == in) return ovec;
  if (NULL == delim) {
    ovec.push_back(string(in));
    return ovec;
  }

  while (*in) {
    int len = 0;
    // Toss all preceeding delimiters
    while (*in && strchr(delim, *in)) in++;

    // If we still have something to process
    if (*in) {
      std::string token;
      start = in;
      len   = 0;
      // Keep moving forward for all valid chars
      while (*in && (strchr(delim, *in) == NULL)) {
        len++;
        in++;
      }

      // Build this token and add it to the array.
      ovec.resize(count + 1);
      ovec[count].assign(start, len);
      count++;
    }
  }

  return ovec;
}



}  // End of namespace gdb_rsp

