// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/core/html/forms/html_text_area_element.h"

#include "testing/gtest/include/gtest/gtest.h"
#include "third_party/blink/renderer/platform/wtf/text/string_builder.h"

namespace blink {

TEST(HTMLTextAreaElementTest, SanitizeUserInputValue) {
  UChar kLeadSurrogate = 0xD800;
  EXPECT_EQ("", HTMLTextAreaElement::SanitizeUserInputValue("", 0));
  EXPECT_EQ("", HTMLTextAreaElement::SanitizeUserInputValue("a", 0));
  EXPECT_EQ("", HTMLTextAreaElement::SanitizeUserInputValue("\n", 0));
  StringBuilder builder;
  builder.Append(kLeadSurrogate);
  String lead_surrogate = builder.ToString();
  EXPECT_EQ("", HTMLTextAreaElement::SanitizeUserInputValue(lead_surrogate, 0));

  EXPECT_EQ("", HTMLTextAreaElement::SanitizeUserInputValue("", 1));
  EXPECT_EQ("", HTMLTextAreaElement::SanitizeUserInputValue(lead_surrogate, 1));
  EXPECT_EQ("a", HTMLTextAreaElement::SanitizeUserInputValue("a", 1));
  EXPECT_EQ("\n", HTMLTextAreaElement::SanitizeUserInputValue("\n", 1));
  EXPECT_EQ("\n", HTMLTextAreaElement::SanitizeUserInputValue("\n", 2));

  EXPECT_EQ("abc", HTMLTextAreaElement::SanitizeUserInputValue(
                       String("abc") + lead_surrogate, 4));
  EXPECT_EQ("a\ncd", HTMLTextAreaElement::SanitizeUserInputValue("a\ncdef", 4));
  EXPECT_EQ("a\rcd", HTMLTextAreaElement::SanitizeUserInputValue("a\rcdef", 4));
  EXPECT_EQ("a\r\ncd",
            HTMLTextAreaElement::SanitizeUserInputValue("a\r\ncdef", 4));
}

}  // namespace blink
