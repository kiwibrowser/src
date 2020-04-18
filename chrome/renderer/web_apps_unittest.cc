// Copyright (c) 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/renderer/web_apps.h"

#include <stddef.h>

#include "base/macros.h"
#include "base/strings/utf_string_conversions.h"
#include "testing/gtest/include/gtest/gtest.h"

// Tests ParseIconSizes with various input.
TEST(WebAppInfo, ParseIconSizes) {
  struct TestData {
    const char* input;
    const bool expected_result;
    const bool is_any;
    const size_t expected_size_count;
    const int width1;
    const int height1;
    const int width2;
    const int height2;
  } data[] = {
    // Bogus input cases.
    { "10",         false, false, 0, 0, 0, 0, 0 },
    { "10 10",      false, false, 0, 0, 0, 0, 0 },
    { "010",        false, false, 0, 0, 0, 0, 0 },
    { " 010 ",      false, false, 0, 0, 0, 0, 0 },
    { " 10x ",      false, false, 0, 0, 0, 0, 0 },
    { " x10 ",      false, false, 0, 0, 0, 0, 0 },
    { "any 10x10",  false, false, 0, 0, 0, 0, 0 },
    { "",           false, false, 0, 0, 0, 0, 0 },
    { "10ax11",     false, false, 0, 0, 0, 0, 0 },

    // Any.
    { "any",        true, true, 0, 0, 0, 0, 0 },
    { " any",       true, true, 0, 0, 0, 0, 0 },
    { " any ",      true, true, 0, 0, 0, 0, 0 },

    // Sizes.
    { "10x11",      true, false, 1, 10, 11, 0, 0 },
    { " 10x11 ",    true, false, 1, 10, 11, 0, 0 },
    { " 10x11 1x2", true, false, 2, 10, 11, 1, 2 },
  };
  for (size_t i = 0; i < arraysize(data); ++i) {
    bool is_any;
    std::vector<gfx::Size> sizes;
    bool result = web_apps::ParseIconSizes(
        base::ASCIIToUTF16(data[i].input), &sizes, &is_any);
    ASSERT_EQ(result, data[i].expected_result);
    if (result) {
      ASSERT_EQ(data[i].is_any, is_any);
      ASSERT_EQ(data[i].expected_size_count, sizes.size());
      if (!sizes.empty()) {
        ASSERT_EQ(data[i].width1, sizes[0].width());
        ASSERT_EQ(data[i].height1, sizes[0].height());
      }
      if (sizes.size() > 1) {
        ASSERT_EQ(data[i].width2, sizes[1].width());
        ASSERT_EQ(data[i].height2, sizes[1].height());
      }
    }
  }
}
