// Copyright (c) 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_CHROME_CLEANER_STRINGS_STRING_TEST_HELPERS_H_
#define CHROME_CHROME_CLEANER_STRINGS_STRING_TEST_HELPERS_H_

#include <vector>

#include "base/strings/string16.h"

namespace chrome_cleaner {

// Turns a string constant into a vector containing embedded nulls by
// converting every '0' to null.
std::vector<wchar_t> CreateVectorWithNulls(const base::string16& str);

// Returns a string16 obtained from |v| by replacing null characters with "\\0".
base::string16 FormatVectorWithNulls(const std::vector<wchar_t>& v);

}  // namespace chrome_cleaner

#endif  // CHROME_CHROME_CLEANER_STRINGS_STRING_TEST_HELPERS_H_
