// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// The original source code is from:
// http://src.chromium.org/viewvc/chrome/trunk/src/base/strings/string_split.h?revision=236210
//
// Modifications from original:
//   1) Supports only std::string type.
//   2) Does not trim whitespace.

#ifndef I18N_ADDRESSINPUT_UTIL_STRING_SPLIT_H_
#define I18N_ADDRESSINPUT_UTIL_STRING_SPLIT_H_

#include <string>
#include <vector>

namespace i18n {
namespace addressinput {

// Splits |str| into a vector of strings delimited by |c|, placing the results
// in |r|. If several instances of |c| are contiguous, or if |str| begins with
// or ends with |c|, then an empty string is inserted.
//
// |str| should not be in a multi-byte encoding like Shift-JIS or GBK in which
// the trailing byte of a multi-byte character can be in the ASCII range.
// UTF-8, and other single/multi-byte ASCII-compatible encodings are OK.
// Note: |c| must be in the ASCII range.
void SplitString(const std::string& str, char s, std::vector<std::string>* r);

}  // namespace addressinput
}  // namespace i18n

#endif  // I18N_ADDRESSINPUT_UTIL_STRING_SPLIT_H_
