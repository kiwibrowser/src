// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/core/dom/first_letter_pseudo_element.h"

#include <gtest/gtest.h>

namespace blink {

class FirstLetterPseudoElementTest : public testing::Test {};

TEST_F(FirstLetterPseudoElementTest, DoesNotBreakEmoji) {
  const UChar emoji[] = {0xD83D, 0xDE31, 0};
  EXPECT_EQ(2u, FirstLetterPseudoElement::FirstLetterLength(emoji));
}

TEST_F(FirstLetterPseudoElementTest, UnicodePairBreaking) {
  const UChar test_string[] = {0xD800, 0xDD00, 'A', 0xD800, 0xDD00,
                               0xD800, 0xDD00, 'B', 0};
  EXPECT_EQ(7u, FirstLetterPseudoElement::FirstLetterLength(test_string));
}

}  // namespace blink
