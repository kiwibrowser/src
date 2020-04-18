// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/platform/wtf/text/integer_to_string_conversion.h"

#include "testing/gtest/include/gtest/gtest.h"
#include "third_party/blink/renderer/platform/wtf/text/string_view.h"

namespace WTF {

TEST(IntegerToStringConversionTest, SimpleIntConversion) {
  const IntegerToStringConverter<int> conv(100500);
  EXPECT_EQ(StringView(conv.Characters8(), conv.length()),
            StringView("100500"));
}

}  // namespace WTF
