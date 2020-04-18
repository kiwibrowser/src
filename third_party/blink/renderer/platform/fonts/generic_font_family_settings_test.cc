// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/platform/fonts/generic_font_family_settings.h"

#include "testing/gtest/include/gtest/gtest.h"

namespace blink {

TEST(GenericFontFamilySettingsTest, FirstAvailableFontFamily) {
  GenericFontFamilySettings settings;
  EXPECT_TRUE(settings.Standard().IsEmpty());

  // Returns the first available font if starts with ",".
  settings.UpdateStandard(",not exist, Arial");
  EXPECT_EQ("Arial", settings.Standard());

  // Otherwise returns any strings as they were set.
  AtomicString non_lists[] = {
      "Arial", "not exist", "not exist, Arial",
  };
  for (const AtomicString& name : non_lists) {
    settings.UpdateStandard(name);
    EXPECT_EQ(name, settings.Standard());
  }
}

}  // namespace blink
