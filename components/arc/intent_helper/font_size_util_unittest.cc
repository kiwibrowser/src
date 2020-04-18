// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/arc/intent_helper/font_size_util.h"

#include "testing/gtest/include/gtest/gtest.h"

namespace arc {

TEST(ArcSettingsServiceTest, FontSizeConvertChromeToAndroid) {
  // Does not return a value smaller than Small.
  EXPECT_DOUBLE_EQ(kAndroidFontScaleSmall,
                   ConvertFontSizeChromeToAndroid(0, 0, 0));

  // Does not return a value larger than Huge
  EXPECT_DOUBLE_EQ(kAndroidFontScaleHuge,
                   ConvertFontSizeChromeToAndroid(100, 100, 100));

  // The max of any Chrome values is what determines the Android value.
  EXPECT_DOUBLE_EQ(kAndroidFontScaleLarge,
                   ConvertFontSizeChromeToAndroid(20, 0, 0));
  EXPECT_DOUBLE_EQ(kAndroidFontScaleLarge,
                   ConvertFontSizeChromeToAndroid(0, 20, 0));
  EXPECT_DOUBLE_EQ(kAndroidFontScaleLarge,
                   ConvertFontSizeChromeToAndroid(0, 0, 20));

  // default fixed font size is adjusted up three pixels
  EXPECT_DOUBLE_EQ(kAndroidFontScaleLarge,
                   ConvertFontSizeChromeToAndroid(0, 17, 0));

  // Small converts properly.
  EXPECT_DOUBLE_EQ(kAndroidFontScaleSmall,
                   ConvertFontSizeChromeToAndroid(12, 0, 0));

  // Normal converts properly.
  EXPECT_DOUBLE_EQ(kAndroidFontScaleNormal,
                   ConvertFontSizeChromeToAndroid(16, 0, 0));

  // Large converts properly.
  EXPECT_DOUBLE_EQ(kAndroidFontScaleLarge,
                   ConvertFontSizeChromeToAndroid(20, 0, 0));

  // Very large converts properly.
  EXPECT_DOUBLE_EQ(kAndroidFontScaleHuge,
                   ConvertFontSizeChromeToAndroid(24, 0, 0));
}

}  // namespace arc
