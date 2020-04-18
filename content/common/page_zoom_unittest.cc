// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/public/common/page_zoom.h"

#include "testing/gtest/include/gtest/gtest.h"

TEST(PageZoomTest, ZoomValuesEqual) {
  // Test two identical values.
  EXPECT_TRUE(content::ZoomValuesEqual(1.5, 1.5));

  // Test two values that are close enough to be considered equal.
  EXPECT_TRUE(content::ZoomValuesEqual(1.5, 1.49999999));

  // Test two values that are close, but should not be considered equal.
  EXPECT_FALSE(content::ZoomValuesEqual(1.5, 1.4));
}

