// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/public/common/manifest_util.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "url/gurl.h"

namespace content {

TEST(ManifestUtilTest, WebDisplayModeConversions) {
  struct ReversibleConversion {
    blink::WebDisplayMode display_mode;
    std::string lowercase_display_mode_string;
  } reversible_conversions[] = {
      {blink::kWebDisplayModeUndefined, ""},
      {blink::kWebDisplayModeBrowser, "browser"},
      {blink::kWebDisplayModeMinimalUi, "minimal-ui"},
      {blink::kWebDisplayModeStandalone, "standalone"},
      {blink::kWebDisplayModeFullscreen, "fullscreen"},
  };

  for (const ReversibleConversion& conversion : reversible_conversions) {
    EXPECT_EQ(
        conversion.display_mode,
        WebDisplayModeFromString(conversion.lowercase_display_mode_string));
    EXPECT_EQ(conversion.lowercase_display_mode_string,
              WebDisplayModeToString(conversion.display_mode));
  }

  // WebDisplayModeFromString() should work with non-lowercase strings.
  EXPECT_EQ(blink::kWebDisplayModeFullscreen,
            WebDisplayModeFromString("Fullscreen"));

  // WebDisplayModeFromString() should return
  // blink::WebDisplayModeUndefined if the string isn't known.
  EXPECT_EQ(blink::kWebDisplayModeUndefined,
            WebDisplayModeFromString("random"));
}

TEST(ManifestUtilTest, WebScreenOrientationLockTypeConversions) {
  struct ReversibleConversion {
    blink::WebScreenOrientationLockType orientation;
    std::string lowercase_orientation_string;
  } reversible_conversions[] = {
      {blink::kWebScreenOrientationLockDefault, ""},
      {blink::kWebScreenOrientationLockPortraitPrimary, "portrait-primary"},
      {blink::kWebScreenOrientationLockPortraitSecondary, "portrait-secondary"},
      {blink::kWebScreenOrientationLockLandscapePrimary, "landscape-primary"},
      {blink::kWebScreenOrientationLockLandscapeSecondary,
       "landscape-secondary"},
      {blink::kWebScreenOrientationLockAny, "any"},
      {blink::kWebScreenOrientationLockLandscape, "landscape"},
      {blink::kWebScreenOrientationLockPortrait, "portrait"},
      {blink::kWebScreenOrientationLockNatural, "natural"},
  };

  for (const ReversibleConversion& conversion : reversible_conversions) {
    EXPECT_EQ(conversion.orientation,
              WebScreenOrientationLockTypeFromString(
                  conversion.lowercase_orientation_string));
    EXPECT_EQ(conversion.lowercase_orientation_string,
              WebScreenOrientationLockTypeToString(conversion.orientation));
  }

  // WebScreenOrientationLockTypeFromString() should work with non-lowercase
  // strings.
  EXPECT_EQ(blink::kWebScreenOrientationLockNatural,
            WebScreenOrientationLockTypeFromString("Natural"));

  // WebScreenOrientationLockTypeFromString() should return
  // blink::WebScreenOrientationLockDefault if the string isn't known.
  EXPECT_EQ(blink::kWebScreenOrientationLockDefault,
            WebScreenOrientationLockTypeFromString("random"));
}

}  // namespace content
