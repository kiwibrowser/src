// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/platform/geometry/double_rect.h"

#include "testing/gtest/include/gtest/gtest.h"
#include "third_party/blink/renderer/platform/wtf/text/wtf_string.h"

namespace blink {

TEST(DoubleRectTest, ToString) {
  DoubleRect empty_rect = DoubleRect();
  EXPECT_EQ("0,0 0x0", empty_rect.ToString());

  DoubleRect rect(1, 2, 3, 4);
  EXPECT_EQ("1,2 3x4", rect.ToString());

  DoubleRect granular_rect(1.6, 2.7, 3.8, 4.9);
  EXPECT_EQ("1.6,2.7 3.8x4.9", granular_rect.ToString());
}

}  // namespace blink
