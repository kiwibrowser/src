// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/platform/text/platform_locale.h"

#include "testing/gtest/include/gtest/gtest.h"

namespace blink {

TEST(PlatformLocaleTest, StripInvalidNumberCharacters) {
  std::unique_ptr<Locale> locale = Locale::Create("ar");
  String result = locale->StripInvalidNumberCharacters(
      String::FromUTF8("abc\xD9\xA0ghi"), "0123456789");
  // ARABIC-INDIC DIGIT ZERO U+0660 = \xD9\xA0 in UTF-8
  EXPECT_EQ(String::FromUTF8("\xD9\xA0"), result);
}
}
