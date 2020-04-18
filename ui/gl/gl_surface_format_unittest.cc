// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "testing/gtest/include/gtest/gtest.h"
#include "ui/gl/gl_surface_format.h"

namespace gl {

TEST(GLSurfaceFormatTest, BasicTest) {
  {
    // Check default format properties.
    GLSurfaceFormat format = GLSurfaceFormat();
    EXPECT_EQ(32, format.GetBufferSize());
    EXPECT_EQ(GLSurfaceFormat::PIXEL_LAYOUT_DONT_CARE,
              format.GetPixelLayout());
  }

  {
    // Check rgb565 format as used for low-end Android devices.
    GLSurfaceFormat format = GLSurfaceFormat();
    format.SetRGB565();
    EXPECT_EQ(16, format.GetBufferSize());
  }

  {
    // Check custom pixel layout.
    GLSurfaceFormat format = GLSurfaceFormat(
        GLSurfaceFormat::PIXEL_LAYOUT_RGBA);
    EXPECT_EQ(GLSurfaceFormat::PIXEL_LAYOUT_RGBA,
              format.GetPixelLayout());

    // The custom pixel layout should remain after other modifications.
    format.SetDepthBits(24);
    EXPECT_EQ(GLSurfaceFormat::PIXEL_LAYOUT_RGBA,
              format.GetPixelLayout());
    EXPECT_EQ(24, format.GetDepthBits());
  }

  {
    // Check IsCompatible
    GLSurfaceFormat format = GLSurfaceFormat();
    EXPECT_TRUE(format.IsCompatible(GLSurfaceFormat()));

    GLSurfaceFormat other = GLSurfaceFormat();
    other.SetRGB565();
    EXPECT_FALSE(format.IsCompatible(other));
    EXPECT_TRUE(other.IsCompatible(other));
  }
}

}
