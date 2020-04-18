// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/base/resource/scale_factor.h"

#include "testing/gtest/include/gtest/gtest.h"

namespace ui {

TEST(ScaleFactorTest, GetScaleFactorScale) {
  EXPECT_FLOAT_EQ(1.0f, GetScaleForScaleFactor(SCALE_FACTOR_100P));
  EXPECT_FLOAT_EQ(1.25f, GetScaleForScaleFactor(SCALE_FACTOR_125P));
  EXPECT_FLOAT_EQ(1.33f, GetScaleForScaleFactor(SCALE_FACTOR_133P));
  EXPECT_FLOAT_EQ(1.4f, GetScaleForScaleFactor(SCALE_FACTOR_140P));
  EXPECT_FLOAT_EQ(1.5f, GetScaleForScaleFactor(SCALE_FACTOR_150P));
  EXPECT_FLOAT_EQ(1.8f, GetScaleForScaleFactor(SCALE_FACTOR_180P));
  EXPECT_FLOAT_EQ(2.0f, GetScaleForScaleFactor(SCALE_FACTOR_200P));
  EXPECT_FLOAT_EQ(2.5f, GetScaleForScaleFactor(SCALE_FACTOR_250P));
  EXPECT_FLOAT_EQ(3.0f, GetScaleForScaleFactor(SCALE_FACTOR_300P));
}

}  // namespace ui
