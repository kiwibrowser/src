// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/chrome/browser/ui/tools_menu/reading_list_menu_view_item.h"

#import <UIKit/UIKit.h>

#include "ios/chrome/browser/ui/reading_list/number_badge_view.h"
#include "ios/chrome/browser/ui/reading_list/text_badge_view.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "testing/platform_test.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

@interface ReadingListMenuViewCell (Testing)

@property(nonatomic, strong) NumberBadgeView* numberBadge;
@property(nonatomic, strong) TextBadgeView* textBadge;

@end

namespace {

class ReadingListMenuViewCellTest : public PlatformTest {
 public:
  ReadingListMenuViewCellTest()
      : readingListMenuViewCell_([[ReadingListMenuViewCell alloc] init]) {
    [readingListMenuViewCell_ initializeViews];
  }

 protected:
  ReadingListMenuViewCell* readingListMenuViewCell_;
};

// Test that the |isNumberBadgeHidden| and |isTextBadgeHidden| properties are
// initialized correctly.
TEST_F(ReadingListMenuViewCellTest, Initialization) {
  EXPECT_TRUE(readingListMenuViewCell_.numberBadge.isHidden);
  EXPECT_TRUE(readingListMenuViewCell_.textBadge.isHidden);
}

// Test that calling updateBadgeCount with a positive value causes the number
// badge to appear but the text badge to remain hidden.
TEST_F(ReadingListMenuViewCellTest, NumberBadgeAlone) {
  [readingListMenuViewCell_ updateBadgeCount:1 animated:NO];

  EXPECT_FALSE(readingListMenuViewCell_.numberBadge.isHidden);
  EXPECT_TRUE(readingListMenuViewCell_.textBadge.isHidden);
}

// Test that calling updateBadgeCount with a non-positive value while the number
// badge is visible causes the number badge to disappear and the text badge
// to remain hidden.
TEST_F(ReadingListMenuViewCellTest, NumberBadgeAloneHides) {
  [readingListMenuViewCell_ updateBadgeCount:1 animated:NO];
  [readingListMenuViewCell_ updateBadgeCount:0 animated:NO];

  EXPECT_TRUE(readingListMenuViewCell_.numberBadge.isHidden);
  EXPECT_TRUE(readingListMenuViewCell_.textBadge.isHidden);
}

// Test that calling updateShowTextBadge with |YES| causes the text badge
// to appear but the number badge to remain hidden.
TEST_F(ReadingListMenuViewCellTest, TextBadgeAlone) {
  [readingListMenuViewCell_ updateShowTextBadge:YES animated:NO];

  EXPECT_TRUE(readingListMenuViewCell_.numberBadge.isHidden);
  EXPECT_FALSE(readingListMenuViewCell_.textBadge.isHidden);
}

// Test that calling updateShowTextBadge with |NO| while the text badge is
// visible causes the text badge to disappear and the number badge to remain
// hidden.
TEST_F(ReadingListMenuViewCellTest, TextBadgeAloneHides) {
  [readingListMenuViewCell_ updateShowTextBadge:YES animated:NO];
  [readingListMenuViewCell_ updateShowTextBadge:NO animated:NO];

  EXPECT_TRUE(readingListMenuViewCell_.numberBadge.isHidden);
  EXPECT_TRUE(readingListMenuViewCell_.textBadge.isHidden);
}

// Test that calling updateBadgeCount with a positive value while the text badge
// is shown causes the number badge to appear but the text badge to disappear.
TEST_F(ReadingListMenuViewCellTest, NumberBadgeHidesTextBadge) {
  [readingListMenuViewCell_ updateShowTextBadge:YES animated:NO];
  [readingListMenuViewCell_ updateBadgeCount:1 animated:NO];

  EXPECT_FALSE(readingListMenuViewCell_.numberBadge.isHidden);
  EXPECT_TRUE(readingListMenuViewCell_.textBadge.isHidden);
}

// Test that calling updateShowTextBadge with |YES| does not cause the text
// badge to appear if the number badge is already visible.
TEST_F(ReadingListMenuViewCellTest,
       TextBadgeDoesNotShowIfNumberBadgeIsVisible) {
  [readingListMenuViewCell_ updateBadgeCount:1 animated:NO];
  [readingListMenuViewCell_ updateShowTextBadge:YES animated:NO];

  EXPECT_FALSE(readingListMenuViewCell_.numberBadge.isHidden);
  EXPECT_TRUE(readingListMenuViewCell_.textBadge.isHidden);
}

}  // namespace
