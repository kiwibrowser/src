// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/chrome/browser/ui/settings/cells/card_multiline_item.h"

#include "testing/gtest/include/gtest/gtest.h"
#import "testing/gtest_mac.h"
#include "testing/platform_test.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

namespace {

using CardMultilineItemTest = PlatformTest;

// Tests that the text is honoured after a call to |configureCell:|.
TEST_F(CardMultilineItemTest, ConfigureCell) {
  CardMultilineItem* item = [[CardMultilineItem alloc] initWithType:0];
  NSString* text = @"Test Disclaimer";

  item.text = text;

  id cell = [[[item cellClass] alloc] init];
  ASSERT_TRUE([cell isMemberOfClass:[CardMultilineCell class]]);

  CardMultilineCell* disclaimerCell = static_cast<CardMultilineCell*>(cell);
  EXPECT_FALSE(disclaimerCell.textLabel.text);

  [item configureCell:cell];
  EXPECT_NSEQ(text, disclaimerCell.textLabel.text);
}

// Tests that the text label of an CardMultilineCell spans multiple
// lines.
TEST_F(CardMultilineItemTest, MultipleLines) {
  CardMultilineCell* cell = [[CardMultilineCell alloc] init];
  EXPECT_EQ(0, cell.textLabel.numberOfLines);
}

}  // namespace
