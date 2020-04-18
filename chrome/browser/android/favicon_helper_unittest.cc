// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/android/favicon_helper.h"

#include "testing/gtest/include/gtest/gtest.h"

TEST(FaviconHelperTest, GetLargestSizeIndex) {
  std::vector<gfx::Size> sizes;
  gfx::Size size1 = gfx::Size(INT_MAX, INT_MAX);
  sizes.push_back(size1);
  gfx::Size size2 = gfx::Size(16, 16);
  sizes.push_back(size2);
  gfx::Size size3 = gfx::Size(32, 32);
  sizes.push_back(size3);
  EXPECT_EQ(2u, FaviconHelper::GetLargestSizeIndex(sizes));
  sizes.clear();
  sizes.push_back(size1);
  sizes.push_back(size1);
  EXPECT_EQ(0u, FaviconHelper::GetLargestSizeIndex(sizes));
}
