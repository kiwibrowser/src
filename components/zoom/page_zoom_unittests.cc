// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/zoom/page_zoom.h"
#include "content/public/common/page_zoom.h"
#include "testing/gtest/include/gtest/gtest.h"

TEST(PageTestZoom, PresetZoomFactors) {
  // Fetch a vector of preset zoom factors, including a custom value that we
  // already know is not going to be in the list.
  double custom_value = 1.05;  // 105%
  std::vector<double> factors = zoom::PageZoom::PresetZoomFactors(custom_value);

  // Expect at least 10 zoom factors.
  EXPECT_GE(factors.size(), 10U);

  // Expect the first and last items to match the minimum and maximum values.
  EXPECT_DOUBLE_EQ(factors.front(), content::kMinimumZoomFactor);
  EXPECT_DOUBLE_EQ(factors.back(), content::kMaximumZoomFactor);

  // Iterate through the vector, with the following checks:
  // 1. The values are in sorted order.
  // 2. The custom value is exists.
  // 3. The 100% value exists.
  bool found_custom_value = false;
  bool found_100_percent = false;
  double last_value = 0;

  std::vector<double>::const_iterator i;
  for (i = factors.begin(); i != factors.end(); ++i) {
    double factor = *i;
    EXPECT_GT(factor, last_value);
    if (content::ZoomValuesEqual(factor, custom_value))
      found_custom_value = true;
    if (content::ZoomValuesEqual(factor, 1.0))
      found_100_percent = true;
    last_value = factor;
  }

  EXPECT_TRUE(found_custom_value);
  EXPECT_TRUE(found_100_percent);
}

TEST(PageTestZoom, PresetZoomLevels) {
  // Fetch a vector of preset zoom levels, including a custom value that we
  // already know is not going to be in the list.
  double custom_value = 0.1;
  std::vector<double> levels = zoom::PageZoom::PresetZoomLevels(custom_value);

  // Expect at least 10 zoom levels.
  EXPECT_GE(levels.size(), 10U);

  // Iterate through the vector, with the following checks:
  // 1. The values are in sorted order.
  // 2. The custom value is exists.
  // 3. The 100% value exists.
  bool found_custom_value = false;
  bool found_100_percent = false;
  double last_value = -99;

  std::vector<double>::const_iterator i;
  for (i = levels.begin(); i != levels.end(); ++i) {
    double level = *i;
    EXPECT_GT(level, last_value);
    if (content::ZoomValuesEqual(level, custom_value))
      found_custom_value = true;
    if (content::ZoomValuesEqual(level, 0))
      found_100_percent = true;
    last_value = level;
  }

  EXPECT_TRUE(found_custom_value);
  EXPECT_TRUE(found_100_percent);
}

TEST(PageTestZoom, InvalidCustomFactor) {
  double too_low = 0.01;
  std::vector<double> factors = zoom::PageZoom::PresetZoomFactors(too_low);
  EXPECT_FALSE(content::ZoomValuesEqual(factors.front(), too_low));

  double too_high = 99.0;
  factors = zoom::PageZoom::PresetZoomFactors(too_high);
  EXPECT_FALSE(content::ZoomValuesEqual(factors.back(), too_high));
}

TEST(PageTestZoom, InvalidCustomLevel) {
  double too_low = -99.0;
  std::vector<double> levels = zoom::PageZoom::PresetZoomLevels(too_low);
  EXPECT_FALSE(content::ZoomValuesEqual(levels.front(), too_low));

  double too_high = 99.0;
  levels = zoom::PageZoom::PresetZoomLevels(too_high);
  EXPECT_FALSE(content::ZoomValuesEqual(levels.back(), too_high));
}
