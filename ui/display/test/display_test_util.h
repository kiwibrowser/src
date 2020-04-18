// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UI_DISPLAY_TEST_DISPLAY_TEST_UTIL_H_
#define UI_DISPLAY_TEST_DISPLAY_TEST_UTIL_H_

#include "ui/display/display.h"

namespace display {

void PrintTo(const Display& display, ::std::ostream* os) {
  *os << display.ToString();
}

}  // namespace display

#endif  // UI_DISPLAY_TEST_DISPLAY_TEST_UTIL_H_
