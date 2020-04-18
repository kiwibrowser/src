// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_ARC_INTENT_HELPER_FONT_SIZE_UTIL_H_
#define COMPONENTS_ARC_INTENT_HELPER_FONT_SIZE_UTIL_H_

namespace arc {

// The following values were obtained from chrome://settings and Android's
// Display settings on Nov 2015. They are expected to remain stable.
const float kAndroidFontScaleSmall = 0.85;
const float kAndroidFontScaleNormal = 1;
const float kAndroidFontScaleLarge = 1.15;
const float kAndroidFontScaleHuge = 1.3;
const int kChromeFontSizeNormal = 16;
const int kChromeFontSizeLarge = 20;
const int kChromeFontSizeVeryLarge = 24;

// Android has only a single float value for system-wide font size
// (font_scale).  Chrome has three main int pixel values that affect
// system-wide font size.  We will take the largest font value of the three
// main font values on Chrome and convert to an Android size.
double ConvertFontSizeChromeToAndroid(int default_size,
                                      int default_fixed_size,
                                      int minimum_size);

}  // namespace arc

#endif  // COMPONENTS_ARC_INTENT_HELPER_FONT_SIZE_UTIL_H_
