// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_AUTOFILL_CORE_COMMON_AUTOFILL_REGEXES_H_
#define COMPONENTS_AUTOFILL_CORE_COMMON_AUTOFILL_REGEXES_H_

#include "base/strings/string16.h"

// Parsing utilities.
namespace autofill {

// Case-insensitive regular expression matching.
// Returns true if |pattern| is found in |input|.
bool MatchesPattern(const base::string16& input,
                    const base::string16& pattern);

}  // namespace autofill

#endif  // COMPONENTS_AUTOFILL_CORE_COMMON_AUTOFILL_REGEXES_H_
