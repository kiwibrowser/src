// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <string>

#include "extensions/common/image_util.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "third_party/skia/include/core/SkColor.h"
#include "ui/gfx/color_utils.h"

namespace extensions {

void RunPassHexTest(const std::string& css_string, SkColor expected_result) {
  SkColor color = 0;
  EXPECT_TRUE(image_util::ParseHexColorString(css_string, &color));
  EXPECT_EQ(color, expected_result);
}

void RunFailHexTest(const std::string& css_string) {
  SkColor color = 0;
  EXPECT_FALSE(image_util::ParseHexColorString(css_string, &color));
}

void RunPassHslTest(const std::string& hsl_string, SkColor expected) {
  SkColor color = 0;
  EXPECT_TRUE(image_util::ParseHslColorString(hsl_string, &color));
  EXPECT_EQ(color, expected);
}

void RunFailHslTest(const std::string& hsl_string) {
  SkColor color = 0;
  EXPECT_FALSE(image_util::ParseHslColorString(hsl_string, &color));
}

void RunPassRgbTest(const std::string& rgb_string, SkColor expected) {
  SkColor color = 0;
  EXPECT_TRUE(image_util::ParseRgbColorString(rgb_string, &color));
  EXPECT_EQ(color, expected);
}

void RunFailRgbTest(const std::string& rgb_string) {
  SkColor color = 0;
  EXPECT_FALSE(image_util::ParseRgbColorString(rgb_string, &color));
}

TEST(ImageUtilTest, ChangeBadgeBackgroundNormalCSS) {
  RunPassHexTest("#34006A", SkColorSetARGB(0xFF, 0x34, 0, 0x6A));
}

TEST(ImageUtilTest, ChangeBadgeBackgroundShortCSS) {
  RunPassHexTest("#A1E", SkColorSetARGB(0xFF, 0xAA, 0x11, 0xEE));
}

TEST(ImageUtilTest, ParseHexWithAlphaCSS) {
  RunPassHexTest("#340061CC", SkColorSetARGB(0xCC, 0x34, 0, 0x61));
}

TEST(ImageUtilTest, ParseHexWithAlphaShortCSS) {
  RunPassHexTest("#A1E9", SkColorSetARGB(0x99, 0xAA, 0x11, 0xEE));
}

TEST(ImageUtilTest, ChangeBadgeBackgroundCSSNoHash) {
  RunFailHexTest("11FF22");
}

TEST(ImageUtilTest, ChangeBadgeBackgroundCSSTooShort) {
  RunFailHexTest("#FF22C");
}

TEST(ImageUtilTest, ChangeBadgeBackgroundCSSTooLong) {
  RunFailHexTest("#FF22128");
}

TEST(ImageUtilTest, ChangeBadgeBackgroundCSSInvalid) {
  RunFailHexTest("#-22128");
}

TEST(ImageUtilTest, ChangeBadgeBackgroundCSSInvalidWithPlus) {
  RunFailHexTest("#+22128");
}

TEST(ImageUtilTest, AcceptHsl) {
  // Run basic color tests.
  RunPassHslTest("hsl(0, 100%, 50%)", SK_ColorRED);
  RunPassHslTest("hsl(120, 100%, 50%)", SK_ColorGREEN);
  RunPassHslTest("hsl(240, 100%, 50%)", SK_ColorBLUE);
  RunPassHslTest("hsl(180, 100%, 50%)", SK_ColorCYAN);

  // Passing in >100% saturation should be equivalent to 100%.
  RunPassHslTest("hsl(120, 200%, 50%)", SK_ColorGREEN);

  // Passing in the same degree +/- full rotations should be equivalent.
  RunPassHslTest("hsl(480, 100%, 50%)", SK_ColorGREEN);
  RunPassHslTest("hsl(-240, 100%, 50%)", SK_ColorGREEN);

  // We should be able to parse doubles
  RunPassHslTest("hsl(120.0, 100.0%, 50.0%)", SK_ColorGREEN);
}

TEST(ImageUtilTest, InvalidHsl) {
  RunFailHslTest("(0,100%,50%)");
  RunFailHslTest("[0, 100, 50]");
  RunFailHslTest("hs l(0,100%,50%)");
  RunFailHslTest("rgb(0,100%,50%)");
  RunFailHslTest("hsl(0,100%)");
  RunFailHslTest("hsl(100%,50%)");
  RunFailHslTest("hsl(120, 100, 50)");
  RunFailHslTest("hsl[120, 100%, 50%]");
  RunFailHslTest("hsl(120, 100%, 50%, 1.0)");
  RunFailHslTest("hsla(120, 100%, 50%)");
}

TEST(ImageUtilTest, AcceptHsla) {
  // Run basic color tests.
  RunPassHslTest("hsla(0, 100%, 50%, 1.0)", SK_ColorRED);
  RunPassHslTest("hsla(0, 100%, 50%, 0.0)",
                 SkColorSetARGB(0x00, 0xFF, 0x00, 0x00));
  RunPassHslTest("hsla(0, 100%, 50%, 0.5)",
                 SkColorSetARGB(0x7F, 0xFF, 0x00, 0x00));
  RunPassHslTest("hsla(0, 100%, 50%, 0.25)",
                 SkColorSetARGB(0x3F, 0xFF, 0x00, 0x00));
  RunPassHslTest("hsla(0, 100%, 50%, 0.75)",
                 SkColorSetARGB(0xBF, 0xFF, 0x00, 0x00));

  // We should able to parse integer alpha value.
  RunPassHslTest("hsla(0, 100%, 50%, 1)", SK_ColorRED);
}

TEST(ImageUtilTest, AcceptRgb) {
  // Run basic color tests.
  RunPassRgbTest("rgb(255,0,0)", SK_ColorRED);
  RunPassRgbTest("rgb(0,    255, 0)", SK_ColorGREEN);
  RunPassRgbTest("rgb(0, 0, 255)", SK_ColorBLUE);
}

TEST(ImageUtilTest, InvalidRgb) {
  RunFailRgbTest("(0,100,50)");
  RunFailRgbTest("[0, 100, 50]");
  RunFailRgbTest("rg b(0,100,50)");
  RunFailRgbTest("rgb(0,-100, 10)");
  RunFailRgbTest("rgb(100,50)");
  RunFailRgbTest("rgb(120.0, 100.6, 50.3)");
  RunFailRgbTest("rgb[120, 100, 50]");
  RunFailRgbTest("rgb(120, 100, 50, 1.0)");
  RunFailRgbTest("rgba(120, 100, 50)");
  RunFailRgbTest("rgb(0, 300, 0)");
  // This is valid RGB but we don't support percentages yet.
  RunFailRgbTest("rgb(100%, 0%, 100%)");
}

TEST(ImageUtilTest, AcceptRgba) {
  // Run basic color tests.
  RunPassRgbTest("rgba(255, 0, 0, 1.0)", SK_ColorRED);
  RunPassRgbTest("rgba(255, 0, 0, 0.0)",
                 SkColorSetARGB(0x00, 0xFF, 0x00, 0x00));
  RunPassRgbTest("rgba(255, 0, 0, 0.5)",
                 SkColorSetARGB(0x7F, 0xFF, 0x00, 0x00));
  RunPassRgbTest("rgba(255, 0, 0, 0.25)",
                 SkColorSetARGB(0x3F, 0xFF, 0x00, 0x00));
  RunPassRgbTest("rgba(255, 0, 0, 0.75)",
                 SkColorSetARGB(0xBF, 0xFF, 0x00, 0x00));

  // We should able to parse an integer alpha value.
  RunPassRgbTest("rgba(255, 0, 0, 1)", SK_ColorRED);
}

TEST(ImageUtilTest, BasicColorKeyword) {
  SkColor color = 0;
  EXPECT_TRUE(image_util::ParseCssColorString("red", &color));
  EXPECT_EQ(color, SK_ColorRED);

  EXPECT_TRUE(image_util::ParseCssColorString("blue", &color));
  EXPECT_EQ(color, SK_ColorBLUE);

  EXPECT_FALSE(image_util::ParseCssColorString("my_red", &color));
}

}  // namespace extensions
