// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// This file defines utility functions for working with strings.
//
// The original source code is from:
// http://src.chromium.org/viewvc/chrome/trunk/src/base/strings/string_util.h?revision=268754
//
// Modified to contain only DoReplaceStringPlaceholders() that works only with
// std::string.

#ifndef I18N_ADDRESSINPUT_UTIL_STRING_UTIL_H_
#define I18N_ADDRESSINPUT_UTIL_STRING_UTIL_H_

#include <string>
#include <vector>

namespace i18n {
namespace addressinput {

std::string DoReplaceStringPlaceholders(const std::string& format_string,
                                        const std::vector<std::string>& subst);

}  // addressinput
}  // i18n

#endif  // I18N_ADDRESSINPUT_UTIL_STRING_UTIL_H_
