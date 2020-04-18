// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/ozone/platform/x11/x11_cursor_factory_ozone.h"

#include "testing/gtest/include/gtest/gtest.h"
#include "third_party/skia/include/core/SkBitmap.h"
#include "ui/gfx/geometry/point.h"

namespace ui {

TEST(X11CursorFactoryOzoneTest, InvisibleRefcount) {
  X11CursorFactoryOzone factory;

  // Building an image cursor with an invalid SkBitmap should return the
  // invisible cursor in X11. The invisible cursor instance should have more
  // than a single reference since the factory should hold a reference and
  // CreateImageCursor should return an incremented refcount.
  X11CursorOzone* invisible_cursor = static_cast<X11CursorOzone*>(
      factory.CreateImageCursor(SkBitmap(), gfx::Point(), 1.0f));
  ASSERT_FALSE(invisible_cursor->HasOneRef());

  // Release our refcount on the cursor
  factory.UnrefImageCursor(invisible_cursor);

  // The invisible cursor should still exist.
  EXPECT_TRUE(invisible_cursor->HasOneRef());
}

}  // namespace ui
