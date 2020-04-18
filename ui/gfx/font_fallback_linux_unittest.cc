// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/gfx/font_fallback_linux.h"

#include "base/strings/utf_string_conversions.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "ui/gfx/font.h"

namespace gfx {

// If the Type 1 Symbol.pfb font is installed, it is returned as fallback font
// for the PUA character 0xf6db. This test ensures we're not returning Type 1
// fonts as fallback.
TEST(FontFallbackLinuxTest, NoType1InFallbackFonts) {
  FallbackFontData font_fallback_data =
      GetFallbackFontForChar(0xf6db, std::string());
  if (font_fallback_data.filename.length() >= 3u) {
    std::string extension = font_fallback_data.filename.substr(
        font_fallback_data.filename.length() - 3);
    EXPECT_NE(extension, "pfb");
  } else {
    EXPECT_EQ(font_fallback_data.filename.length(), 0u);
  }
}

}  // namespace gfx
