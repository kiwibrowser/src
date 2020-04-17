// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef OSP_BASE_STRINGPRINTF_H_
#define OSP_BASE_STRINGPRINTF_H_

#include <ostream>

namespace openscreen {

template <typename It>
void PrettyPrintAsciiHex(std::ostream& os, It first, It last) {
  auto it = first;
  while (it != last) {
    uint8_t c = *it++;
    if (c >= ' ' && c <= '~') {
      os.put(c);
    } else {
      // Output a hex escape sequence for non-printable values.
      os.put('\\');
      os.put('x');
      char digit = (c >> 4) & 0xf;
      if (digit >= 0 && digit <= 9) {
        os.put(digit + '0');
      } else {
        os.put(digit - 10 + 'a');
      }
      digit = c & 0xf;
      if (digit >= 0 && digit <= 9) {
        os.put(digit + '0');
      } else {
        os.put(digit - 10 + 'a');
      }
    }
  }
}

}  // namespace openscreen

#endif  // OSP_BASE_STRINGPRINTF_H_
