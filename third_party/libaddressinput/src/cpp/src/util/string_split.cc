// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// The original source code is from:
// http://src.chromium.org/viewvc/chrome/trunk/src/base/strings/string_split.cc?revision=216633

#include "string_split.h"

#include <cassert>
#include <cstddef>
#include <string>
#include <vector>

namespace i18n {
namespace addressinput {

void SplitString(const std::string& str, char s, std::vector<std::string>* r) {
  assert(r != nullptr);
  r->clear();
  size_t last = 0;
  size_t c = str.size();
  for (size_t i = 0; i <= c; ++i) {
    if (i == c || str[i] == s) {
      std::string tmp(str, last, i - last);
      // Avoid converting an empty or all-whitespace source string into a vector
      // of one empty string.
      if (i != c || !r->empty() || !tmp.empty()) {
        r->push_back(tmp);
      }
      last = i + 1;
    }
  }
}

}  // namespace addressinput
}  // namespace i18n
