// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_ANDROID_COLOR_HELPERS_H_
#define CHROME_BROWSER_ANDROID_COLOR_HELPERS_H_

#include <stdint.h>

#include <limits>
#include <string>

#include "base/optional.h"
#include "third_party/skia/include/core/SkColor.h"

constexpr int64_t kInvalidJavaColor =
    static_cast<int64_t>(std::numeric_limits<int32_t>::max()) + 1;

// Converts |color| to a CSS color string. If |color| is null, the empty string
// is returned.
std::string OptionalSkColorToString(const base::Optional<SkColor>& color);

// Conversions between a Java color and an Optional<SkColor>. Java colors are
// represented as 64-bit signed integers. Valid colors are in the range
// [std::numeric_limits<int32_t>::min(), std::numeric_limits<int32_t>::max()].
// while |kInvalidJavaColor| is reserved for representing a null/unset color.
int64_t OptionalSkColorToJavaColor(const base::Optional<SkColor>& skcolor);
base::Optional<SkColor> JavaColorToOptionalSkColor(int64_t java_color);

#endif  // CHROME_BROWSER_ANDROID_COLOR_HELPERS_H_
