// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/base/cursor/cursor_loader_x11.h"

#include "base/logging.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "third_party/skia/include/core/SkBitmap.h"
#include "ui/base/cursor/cursor_util.h"
#include "ui/display/display.h"

namespace ui {

TEST(CursorLoaderX11Test, ScaleAndRotate) {
  SkBitmap bitmap;
  bitmap.allocN32Pixels(10, 5);
  bitmap.eraseColor(SK_ColorBLACK);

  gfx::Point hotpoint(3,4);

  ScaleAndRotateCursorBitmapAndHotpoint(1.0f, display::Display::ROTATE_0,
                                        &bitmap, &hotpoint);
  EXPECT_EQ(10, bitmap.width());
  EXPECT_EQ(5, bitmap.height());
  EXPECT_EQ("3,4", hotpoint.ToString());

  ScaleAndRotateCursorBitmapAndHotpoint(1.0f, display::Display::ROTATE_90,
                                        &bitmap, &hotpoint);

  EXPECT_EQ(5, bitmap.width());
  EXPECT_EQ(10, bitmap.height());
  EXPECT_EQ("1,3", hotpoint.ToString());

  ScaleAndRotateCursorBitmapAndHotpoint(2.0f, display::Display::ROTATE_180,
                                        &bitmap, &hotpoint);
  EXPECT_EQ(10, bitmap.width());
  EXPECT_EQ(20, bitmap.height());
  EXPECT_EQ("8,14", hotpoint.ToString());

  ScaleAndRotateCursorBitmapAndHotpoint(1.0f, display::Display::ROTATE_270,
                                        &bitmap, &hotpoint);
  EXPECT_EQ(20, bitmap.width());
  EXPECT_EQ(10, bitmap.height());
  EXPECT_EQ("14,2", hotpoint.ToString());
}

}  // namespace ui
