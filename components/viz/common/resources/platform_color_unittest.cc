// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/viz/common/resources/platform_color.h"

#include <stddef.h>

#include "testing/gtest/include/gtest/gtest.h"

namespace viz {
namespace {

// Verify SameComponentOrder on this platform.
TEST(PlatformColorTest, SameComponentOrder) {
  bool rgba = !!SK_B32_SHIFT;

  for (size_t i = 0; i <= RESOURCE_FORMAT_MAX; ++i) {
    ResourceFormat format = static_cast<ResourceFormat>(i);
    switch (format) {
      case RGBA_8888:
        EXPECT_EQ(rgba, PlatformColor::SameComponentOrder(format));
        break;
      case BGRA_8888:
        EXPECT_NE(rgba, PlatformColor::SameComponentOrder(format));
        break;
      // The following formats are not platform colors.
      case ALPHA_8:
      case LUMINANCE_8:
      case RGB_565:
      case RGBA_4444:
      case ETC1:
      case RED_8:
      case LUMINANCE_F16:
      case RGBA_F16:
      case R16_EXT:
        break;
    }
  }
}

}  // namespace
}  // namespace viz
