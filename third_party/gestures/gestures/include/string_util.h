// Copyright (c) 2014 The Chromium OS Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef GESTURES_STRING_UTIL_H_
#define GESTURES_STRING_UTIL_H_

#include <string>
#include <vector>

#include "gestures/include/compiler_specific.h"

namespace gestures {

// Return a C++ string given printf-like input.
std::string StringPrintf(const char* format, ...)
      PRINTF_FORMAT(1, 2);

// Lower-level routine that takes a va_list and appends to a specified
// string.  All other routines are just convenience wrappers around it.
void StringAppendV(std::string* dst, const char* format, va_list ap)
    PRINTF_FORMAT(2, 0);

// Trims any whitespace from either end of the input string.  Returns where
// whitespace was found.
// The non-wide version has two functions:
// * TrimWhitespaceASCII()
//   This function is for ASCII strings and only looks for ASCII whitespace;
// Please choose the best one according to your usage.
// NOTE: Safe to use the same variable for both input and output.
enum TrimPositions {
  TRIM_NONE     = 0,
  TRIM_LEADING  = 1 << 0,
  TRIM_TRAILING = 1 << 1,
  TRIM_ALL      = TRIM_LEADING | TRIM_TRAILING,
};
TrimPositions TrimWhitespaceASCII(const std::string& input,
                                              TrimPositions positions,
                                              std::string* output);

// Returns true if str starts with search, or false otherwise.
bool StartsWithASCII(const std::string& str,
                     const std::string& search,
                     bool case_sensitive);

// |str| should not be in a multi-byte encoding like Shift-JIS or GBK in which
// the trailing byte of a multi-byte character can be in the ASCII range.
// UTF-8, and other single/multi-byte ASCII-compatible encodings are OK.
// Note: |c| must be in the ASCII range.
void SplitString(const std::string& str,
                 char c,
                 std::vector<std::string>* r);


}  // namespace gestures

#endif  // GESTURES_STRING_UTIL_H_
