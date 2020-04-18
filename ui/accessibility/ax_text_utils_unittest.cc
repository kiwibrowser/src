// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <stddef.h>

#include "base/strings/utf_string_conversions.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "ui/accessibility/ax_enums.mojom.h"
#include "ui/accessibility/ax_text_utils.h"

namespace ui {

TEST(AXTextUtils, FindAccessibleTextBoundaryWord) {
  const base::string16 text =
      base::UTF8ToUTF16("Hello there.This/is\ntesting.");
  const size_t text_length = text.length();
  std::vector<int> line_start_offsets;
  line_start_offsets.push_back(19);
  size_t result;

  result = FindAccessibleTextBoundary(text, line_start_offsets, WORD_BOUNDARY,
                                      0, FORWARDS_DIRECTION,
                                      ax::mojom::TextAffinity::kDownstream);
  EXPECT_EQ(6UL, result);
  result = FindAccessibleTextBoundary(text, line_start_offsets, WORD_BOUNDARY,
                                      5, BACKWARDS_DIRECTION,
                                      ax::mojom::TextAffinity::kDownstream);
  EXPECT_EQ(0UL, result);
  result = FindAccessibleTextBoundary(text, line_start_offsets, WORD_BOUNDARY,
                                      6, FORWARDS_DIRECTION,
                                      ax::mojom::TextAffinity::kDownstream);
  EXPECT_EQ(12UL, result);
  result = FindAccessibleTextBoundary(text, line_start_offsets, WORD_BOUNDARY,
                                      11, BACKWARDS_DIRECTION,
                                      ax::mojom::TextAffinity::kDownstream);
  EXPECT_EQ(6UL, result);
  result = FindAccessibleTextBoundary(text, line_start_offsets, WORD_BOUNDARY,
                                      12, BACKWARDS_DIRECTION,
                                      ax::mojom::TextAffinity::kDownstream);
  EXPECT_EQ(12UL, result);
  result = FindAccessibleTextBoundary(text, line_start_offsets, WORD_BOUNDARY,
                                      15, FORWARDS_DIRECTION,
                                      ax::mojom::TextAffinity::kDownstream);
  EXPECT_EQ(17UL, result);
  result = FindAccessibleTextBoundary(text, line_start_offsets, WORD_BOUNDARY,
                                      15, BACKWARDS_DIRECTION,
                                      ax::mojom::TextAffinity::kDownstream);
  EXPECT_EQ(12UL, result);
  result = FindAccessibleTextBoundary(text, line_start_offsets, WORD_BOUNDARY,
                                      16, FORWARDS_DIRECTION,
                                      ax::mojom::TextAffinity::kDownstream);
  EXPECT_EQ(17UL, result);
  result = FindAccessibleTextBoundary(text, line_start_offsets, WORD_BOUNDARY,
                                      17, FORWARDS_DIRECTION,
                                      ax::mojom::TextAffinity::kDownstream);
  EXPECT_EQ(20UL, result);
  result = FindAccessibleTextBoundary(text, line_start_offsets, WORD_BOUNDARY,
                                      20, FORWARDS_DIRECTION,
                                      ax::mojom::TextAffinity::kDownstream);
  EXPECT_EQ(text_length, result);
  result = FindAccessibleTextBoundary(text, line_start_offsets, WORD_BOUNDARY,
                                      text_length, BACKWARDS_DIRECTION,
                                      ax::mojom::TextAffinity::kDownstream);
  EXPECT_EQ(20UL, result);
}

TEST(AXTextUtils, FindAccessibleTextBoundaryLine) {
  const base::string16 text = base::UTF8ToUTF16("Line 1.\nLine 2\n\t");
  const size_t text_length = text.length();
  std::vector<int> line_start_offsets;
  line_start_offsets.push_back(8);
  line_start_offsets.push_back(15);
  size_t result;

  // Basic cases.
  result = FindAccessibleTextBoundary(text, line_start_offsets, LINE_BOUNDARY,
                                      5, FORWARDS_DIRECTION,
                                      ax::mojom::TextAffinity::kDownstream);
  EXPECT_EQ(8UL, result);
  result = FindAccessibleTextBoundary(text, line_start_offsets, LINE_BOUNDARY,
                                      9, BACKWARDS_DIRECTION,
                                      ax::mojom::TextAffinity::kDownstream);
  EXPECT_EQ(8UL, result);
  result = FindAccessibleTextBoundary(text, line_start_offsets, LINE_BOUNDARY,
                                      10, FORWARDS_DIRECTION,
                                      ax::mojom::TextAffinity::kDownstream);
  EXPECT_EQ(15UL, result);

  // Edge cases.
  result = FindAccessibleTextBoundary(text, line_start_offsets, LINE_BOUNDARY,
                                      text_length, BACKWARDS_DIRECTION,
                                      ax::mojom::TextAffinity::kDownstream);
  EXPECT_EQ(15UL, result);

  // When the start_offset is at the start of the next line and we are searching
  // backwards, it should not move.
  result = FindAccessibleTextBoundary(text, line_start_offsets, LINE_BOUNDARY,
                                      15, BACKWARDS_DIRECTION,
                                      ax::mojom::TextAffinity::kDownstream);
  EXPECT_EQ(15UL, result);

  // When the start_offset is at a hard line break and we are searching
  // backwards, it should return the start of the previous line.
  result = FindAccessibleTextBoundary(text, line_start_offsets, LINE_BOUNDARY,
                                      14, BACKWARDS_DIRECTION,
                                      ax::mojom::TextAffinity::kDownstream);
  EXPECT_EQ(8UL, result);

  // When the start_offset is at the start of a line and we are searching
  // forwards, it should return the start of the next line.
  result = FindAccessibleTextBoundary(text, line_start_offsets, LINE_BOUNDARY,
                                      8, FORWARDS_DIRECTION,
                                      ax::mojom::TextAffinity::kDownstream);
  EXPECT_EQ(15UL, result);

  // When there is no previous line break and we are searching backwards,
  // it should return 0.
  result = FindAccessibleTextBoundary(text, line_start_offsets, LINE_BOUNDARY,
                                      4, BACKWARDS_DIRECTION,
                                      ax::mojom::TextAffinity::kDownstream);
  EXPECT_EQ(0UL, result);

  // When we are at the start of the last line and we are searching forwards.
  // it should return the text length.
  result = FindAccessibleTextBoundary(text, line_start_offsets, LINE_BOUNDARY,
                                      15, FORWARDS_DIRECTION,
                                      ax::mojom::TextAffinity::kDownstream);
  EXPECT_EQ(text_length, result);
}

}  // namespace ui
