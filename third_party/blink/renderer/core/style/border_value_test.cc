// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/core/style/border_value.h"

#include <limits.h>
#include "testing/gtest/include/gtest/gtest.h"

namespace blink {

TEST(BorderValueTest, BorderValueWidth) {
  const float kTolerance = 1.0f / kBorderWidthDenominator;
  BorderValue border;

  border.SetWidth(1.0f);
  EXPECT_FLOAT_EQ(1.0f, border.Width());
  border.SetWidth(1.25f);
  EXPECT_FLOAT_EQ(1.25f, border.Width());
  border.SetWidth(1.1f);
  EXPECT_NEAR(border.Width(), 1.1f, kTolerance);
  border.SetWidth(1.33f);
  EXPECT_NEAR(border.Width(), 1.33f, kTolerance);
  border.SetWidth(1.3333f);
  EXPECT_NEAR(border.Width(), 1.3333f, kTolerance);
  border.SetWidth(1.53434f);
  EXPECT_NEAR(border.Width(), 1.53434f, kTolerance);
  border.SetWidth(345634);
  EXPECT_NEAR(border.Width(), 345634.0f, kTolerance);
  border.SetWidth(345634.12335f);
  EXPECT_NEAR(border.Width(), 345634.12335f, kTolerance);

  border.SetWidth(0);
  EXPECT_EQ(0, border.Width());
  border.SetWidth(1);
  EXPECT_EQ(1, border.Width());
  border.SetWidth(100);
  EXPECT_EQ(100, border.Width());
  border.SetWidth(1000);
  EXPECT_EQ(1000, border.Width());
  border.SetWidth(10000);
  EXPECT_EQ(10000, border.Width());
  border.SetWidth(kMaxForBorderWidth / 2);
  EXPECT_EQ(kMaxForBorderWidth / 2, border.Width());
  border.SetWidth(kMaxForBorderWidth - 1);
  EXPECT_EQ(kMaxForBorderWidth - 1, border.Width());
  border.SetWidth(kMaxForBorderWidth);
  EXPECT_EQ(kMaxForBorderWidth, border.Width());
  border.SetWidth(kMaxForBorderWidth + 1);
  EXPECT_EQ(kMaxForBorderWidth, border.Width());
  border.SetWidth(INT_MAX / 2);
  EXPECT_EQ(kMaxForBorderWidth, border.Width());
  border.SetWidth(INT_MAX);
  EXPECT_EQ(kMaxForBorderWidth, border.Width());
}

TEST(BorderValueTest, BorderValueColor) {
  BorderValue border1 =
      BorderValue(EBorderStyle::kSolid, StyleColor::CurrentColor(), 5);
  EXPECT_EQ(border1.ColorIsCurrentColor(), true);

  BorderValue border2 =
      BorderValue(EBorderStyle::kSolid, StyleColor(Color(128, 0, 0)), 5);
  EXPECT_EQ(border2.ColorIsCurrentColor(), false);
}

}  // namespace blink
