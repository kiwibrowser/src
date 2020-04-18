// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/platform/geometry/layout_size.h"

#include "testing/gtest/include/gtest/gtest.h"
#include "third_party/blink/renderer/platform/wtf/text/wtf_string.h"

namespace blink {

TEST(LayoutSizeTest, FitToAspectRatioDoesntOverflow) {
  // FitToAspectRatio() shouldn't overflow due to intermediate calculations,
  // for both the "constrained by width" and "constrained by height" cases.
  LayoutSize aspect_ratio(50000, 50000);
  EXPECT_EQ("1000x1000",
            LayoutSize(2000, 1000)
                .FitToAspectRatio(aspect_ratio, kAspectRatioFitShrink)
                .ToString());

  LayoutSize size(1000, 2000);
  EXPECT_EQ("1000x1000",
            LayoutSize(1000, 2000)
                .FitToAspectRatio(aspect_ratio, kAspectRatioFitShrink)
                .ToString());
}

}  // namespace blink
