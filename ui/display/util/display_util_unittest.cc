// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/display/util/display_util.h"

#include "testing/gtest/include/gtest/gtest.h"

namespace display {

TEST(DisplayUtilTest, TestBlackListedDisplay) {
  EXPECT_TRUE(IsDisplaySizeBlackListed(gfx::Size(10, 10)));
  EXPECT_TRUE(IsDisplaySizeBlackListed(gfx::Size(40, 30)));
  EXPECT_TRUE(IsDisplaySizeBlackListed(gfx::Size(50, 40)));
  EXPECT_TRUE(IsDisplaySizeBlackListed(gfx::Size(160, 90)));
  EXPECT_TRUE(IsDisplaySizeBlackListed(gfx::Size(160, 100)));

  EXPECT_FALSE(IsDisplaySizeBlackListed(gfx::Size(50, 60)));
  EXPECT_FALSE(IsDisplaySizeBlackListed(gfx::Size(100, 70)));
  EXPECT_FALSE(IsDisplaySizeBlackListed(gfx::Size(272, 181)));
}

}  // namespace display
