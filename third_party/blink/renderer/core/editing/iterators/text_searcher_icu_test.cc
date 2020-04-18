// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/core/editing/iterators/text_searcher_icu.h"

#include "testing/gtest/include/gtest/gtest.h"
#include "third_party/blink/renderer/platform/wtf/text/string_view.h"
#include "third_party/blink/renderer/platform/wtf/text/wtf_string.h"

namespace blink {

namespace {

String MakeUTF16(const char* str) {
  String utf16_string = String::FromUTF8(str);
  utf16_string.Ensure16Bit();
  return utf16_string;
}

}  // namespace

TEST(TextSearcherICUTest, FindSubstring) {
  TextSearcherICU searcher;
  const String& pattern = MakeUTF16("substring");
  searcher.SetPattern(pattern, true);

  const String& text = MakeUTF16("Long text with substring content.");
  searcher.SetText(text.Characters16(), text.length());

  MatchResultICU result;

  EXPECT_TRUE(searcher.NextMatchResult(result));
  EXPECT_NE(0u, result.start);
  EXPECT_NE(0u, result.length);
  ASSERT_LT(result.length, text.length());
  EXPECT_EQ(pattern, text.Substring(result.start, result.length));

  EXPECT_FALSE(searcher.NextMatchResult(result));
  EXPECT_EQ(0u, result.start);
  EXPECT_EQ(0u, result.length);
}

TEST(TextSearcherICUTest, FindIgnoreCaseSubstring) {
  TextSearcherICU searcher;
  const String& pattern = MakeUTF16("substring");
  searcher.SetPattern(pattern, false);

  const String& text = MakeUTF16("Long text with SubStrinG content.");
  searcher.SetText(text.Characters16(), text.length());

  MatchResultICU result;
  EXPECT_TRUE(searcher.NextMatchResult(result));
  EXPECT_NE(0u, result.start);
  EXPECT_NE(0u, result.length);
  ASSERT_LT(result.length, text.length());
  EXPECT_EQ(pattern,
            text.Substring(result.start, result.length).DeprecatedLower());

  searcher.SetPattern(pattern, true);
  searcher.SetOffset(0u);
  EXPECT_FALSE(searcher.NextMatchResult(result));
  EXPECT_EQ(0u, result.start);
  EXPECT_EQ(0u, result.length);
}

TEST(TextSearcherICUTest, FindSubstringWithOffset) {
  TextSearcherICU searcher;
  const String& pattern = MakeUTF16("substring");
  searcher.SetPattern(pattern, true);

  const String& text =
      MakeUTF16("Long text with substring content. Second substring");
  searcher.SetText(text.Characters16(), text.length());

  MatchResultICU first_result;

  EXPECT_TRUE(searcher.NextMatchResult(first_result));
  EXPECT_NE(0u, first_result.start);
  EXPECT_NE(0u, first_result.length);

  MatchResultICU second_result;
  EXPECT_TRUE(searcher.NextMatchResult(second_result));
  EXPECT_NE(0u, second_result.start);
  EXPECT_NE(0u, second_result.length);

  searcher.SetOffset(first_result.start + first_result.length);

  MatchResultICU offset_result;
  EXPECT_TRUE(searcher.NextMatchResult(offset_result));
  EXPECT_EQ(offset_result.start, second_result.start);
  EXPECT_EQ(offset_result.length, second_result.length);

  searcher.SetOffset(first_result.start);

  EXPECT_TRUE(searcher.NextMatchResult(offset_result));
  EXPECT_EQ(offset_result.start, first_result.start);
  EXPECT_EQ(offset_result.length, first_result.length);
}

}  // namespace blink
