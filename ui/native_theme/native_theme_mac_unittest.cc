// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/native_theme/native_theme.h"

#include "testing/gtest/include/gtest/gtest.h"

namespace ui {

// Test to ensure any system colors that are looked up by name exist on all Mac
// platforms Chrome supports, and that their colorspace and component count is
// sane.
TEST(NativeThemeMacTest, SystemColorsExist) {
  NativeTheme* native_theme = NativeTheme::GetInstanceForNativeUi();
  ASSERT_TRUE(native_theme);
  for (int i = 0; i < NativeTheme::kColorId_NumColors; ++i) {
    // While 0 is a valid color, no system color should be fully transparent.
    // This is also to probe for CHECKs.
    EXPECT_NE(
        static_cast<SkColor>(0),
        native_theme->GetSystemColor(static_cast<NativeTheme::ColorId>(i)))
            << "GetSystemColor() unexpectedly gave a fully transparent color.";
  }
}

}  // namespace ui
