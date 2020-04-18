// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_COMMON_DATE_TIME_SUGGESTION_H_
#define CONTENT_COMMON_DATE_TIME_SUGGESTION_H_

#include <vector>

#include "base/strings/string16.h"
#include "content/common/content_export.h"
#include "third_party/skia/include/core/SkColor.h"

namespace content {

// Container for information about datalist suggestion for the date/time input
// control. Keep in sync with DateTimeSuggestion.java
struct CONTENT_EXPORT DateTimeSuggestion {
  DateTimeSuggestion() {}

  // The date/time value represented as a double.
  double value;
  // The localized value to be shown to the user.
  base::string16 localized_value;
  // The label for the suggestion.
  base::string16 label;
};

}  // namespace content

#endif  // CONTENT_COMMON_DATE_TIME_SUGGESTION_H_
