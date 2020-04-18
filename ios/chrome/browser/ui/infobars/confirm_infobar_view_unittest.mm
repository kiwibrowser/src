// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/chrome/browser/ui/infobars/confirm_infobar_view.h"

#include "testing/gtest/include/gtest/gtest.h"
#include "testing/gtest_mac.h"
#include "testing/platform_test.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

@interface ConfirmInfoBarView (Testing)
- (CGFloat)buttonsHeight;
- (CGFloat)buttonMargin;
- (CGFloat)computeRequiredHeightAndLayoutSubviews:(BOOL)layout;
- (CGFloat)heightThatFitsButtonsUnderOtherWidgets:(CGFloat)heightOfFirstLine
                                           layout:(BOOL)layout;
- (CGFloat)minimumInfobarHeight;
- (NSString*)stripMarkersFromString:(NSString*)string;
- (const std::vector<std::pair<NSUInteger, NSRange>>&)linkRanges;
@end

namespace {

const int kShortStringLength = 4;
const int kLongStringLength = 1000;

class ConfirmInfoBarViewTest : public PlatformTest {
 protected:
  void SetUp() override {
    PlatformTest::SetUp();
    CGFloat screenWidth = [UIScreen mainScreen].bounds.size.width;
    confirmInfobarView_ = [[ConfirmInfoBarView alloc]
        initWithFrame:CGRectMake(0, 0, screenWidth, 0)];
    [confirmInfobarView_ addCloseButtonWithTag:0 target:nil action:nil];
  }

  NSString* RandomString(int numberOfCharacters) {
    NSMutableString* string = [NSMutableString string];
    NSString* letters = @"abcde ";
    for (int i = 0; i < numberOfCharacters; i++) {
      [string
          appendFormat:@"%C", [letters characterAtIndex:arc4random_uniform(
                                                            [letters length])]];
    }
    return string;
  }

  NSString* ShortRandomString() { return RandomString(kShortStringLength); }

  NSString* LongRandomString() { return RandomString(kLongStringLength); }

  CGFloat InfobarHeight() {
    return [confirmInfobarView_ computeRequiredHeightAndLayoutSubviews:NO];
  }

  CGFloat MinimumInfobarHeight() {
    return [confirmInfobarView_ minimumInfobarHeight];
  }

  CGFloat ButtonsHeight() { return [confirmInfobarView_ buttonsHeight]; }

  CGFloat ButtonMargin() { return [confirmInfobarView_ buttonMargin]; }

  void TestLinkDetectionHelper(
      NSString* input,
      NSString* expectedOutput,
      const std::vector<std::pair<NSUInteger, NSRange>>& expectedRanges) {
    NSString* output = [confirmInfobarView_ stripMarkersFromString:input];
    EXPECT_NSEQ(expectedOutput, output);
    const std::vector<std::pair<NSUInteger, NSRange>>& ranges =
        [confirmInfobarView_ linkRanges];
    EXPECT_EQ(expectedRanges.size(), ranges.size());
    for (unsigned int i = 0; i < expectedRanges.size(); ++i) {
      EXPECT_EQ(expectedRanges[i].first, ranges[i].first);
      EXPECT_TRUE(NSEqualRanges(expectedRanges[i].second, ranges[i].second));
    }
  }

  ConfirmInfoBarView* confirmInfobarView_;
};

TEST_F(ConfirmInfoBarViewTest, TestLayoutWithNoLabel) {
  // Do not call -addLabel: to test the case when there is no label.
  EXPECT_EQ(MinimumInfobarHeight(), InfobarHeight());
}

TEST_F(ConfirmInfoBarViewTest, TestLayoutWithShortLabel) {
  [confirmInfobarView_ addLabel:ShortRandomString()];
  EXPECT_EQ(MinimumInfobarHeight(), InfobarHeight());
}

TEST_F(ConfirmInfoBarViewTest, TestLayoutWithLongLabel) {
  [confirmInfobarView_ addLabel:LongRandomString()];
  EXPECT_LT(MinimumInfobarHeight(), InfobarHeight());
  EXPECT_EQ(0, [confirmInfobarView_ heightThatFitsButtonsUnderOtherWidgets:0
                                                                    layout:NO]);
}

TEST_F(ConfirmInfoBarViewTest, TestLayoutWithShortButtons) {
  [confirmInfobarView_ addLabel:ShortRandomString()];
  [confirmInfobarView_ addButton1:ShortRandomString()
                             tag1:0
                          button2:ShortRandomString()
                             tag2:0
                           target:nil
                           action:nil];
  EXPECT_EQ(MinimumInfobarHeight(), InfobarHeight());
  EXPECT_EQ(
      ButtonsHeight(),
      [confirmInfobarView_ heightThatFitsButtonsUnderOtherWidgets:0 layout:NO]);
}

TEST_F(ConfirmInfoBarViewTest, TestLayoutWithOneLongButtonAndOneShortButton) {
  [confirmInfobarView_ addLabel:ShortRandomString()];
  [confirmInfobarView_ addButton1:LongRandomString()
                             tag1:0
                          button2:ShortRandomString()
                             tag2:0
                           target:nil
                           action:nil];
  EXPECT_EQ(MinimumInfobarHeight() + ButtonsHeight() * 2 + ButtonMargin(),
            InfobarHeight());
  EXPECT_EQ(
      ButtonsHeight() * 2,
      [confirmInfobarView_ heightThatFitsButtonsUnderOtherWidgets:0 layout:NO]);
}

TEST_F(ConfirmInfoBarViewTest, TestLayoutWithShortLabelAndShortButton) {
  [confirmInfobarView_ addLabel:ShortRandomString()];
  [confirmInfobarView_ addButton:ShortRandomString()
                             tag:0
                          target:nil
                          action:nil];
  EXPECT_EQ(MinimumInfobarHeight(), InfobarHeight());
}

TEST_F(ConfirmInfoBarViewTest, TestLayoutWithShortLabelAndLongButton) {
  [confirmInfobarView_ addLabel:ShortRandomString()];
  [confirmInfobarView_ addButton:LongRandomString()
                             tag:0
                          target:nil
                          action:nil];
  EXPECT_EQ(MinimumInfobarHeight() + ButtonsHeight() + ButtonMargin(),
            InfobarHeight());
}

TEST_F(ConfirmInfoBarViewTest, TestLayoutWithLongLabelAndLongButtons) {
  [confirmInfobarView_ addLabel:LongRandomString()];
  [confirmInfobarView_ addButton1:ShortRandomString()
                             tag1:0
                          button2:LongRandomString()
                             tag2:0
                           target:nil
                           action:nil];
  EXPECT_LT(MinimumInfobarHeight() + ButtonsHeight() * 2, InfobarHeight());
}

TEST_F(ConfirmInfoBarViewTest, TestLinkDetection) {
  [confirmInfobarView_ addLabel:ShortRandomString()];
  NSString* linkFoo = [ConfirmInfoBarView stringAsLink:@"foo" tag:1];
  NSString* linkBar = [ConfirmInfoBarView stringAsLink:@"bar" tag:2];
  std::vector<std::pair<NSUInteger, NSRange>> ranges;
  // No link.
  TestLinkDetectionHelper(@"", @"", ranges);
  TestLinkDetectionHelper(@"foo", @"foo", ranges);
  // One link.
  ranges.push_back(std::make_pair(1, NSMakeRange(0, 3)));
  TestLinkDetectionHelper(linkFoo, @"foo", ranges);
  NSString* link1 = [NSString stringWithFormat:@"baz%@qux", linkFoo];
  // Link in the middle.
  ranges.clear();
  ranges.push_back(std::make_pair(1, NSMakeRange(3, 3)));
  TestLinkDetectionHelper(link1, @"bazfooqux", ranges);
  // Multiple links.
  NSString* link2 = [NSString stringWithFormat:@"%@%@", linkFoo, linkBar];
  ranges.clear();
  ranges.push_back(std::make_pair(1, NSMakeRange(0, 3)));
  ranges.push_back(std::make_pair(2, NSMakeRange(3, 3)));
  TestLinkDetectionHelper(link2, @"foobar", ranges);
  // Multiple links and text.
  NSString* link3 =
      [NSString stringWithFormat:@"baz%@qux%@tot", linkFoo, linkBar];
  ranges.clear();
  ranges.push_back(std::make_pair(1, NSMakeRange(3, 3)));
  ranges.push_back(std::make_pair(2, NSMakeRange(9, 3)));
  TestLinkDetectionHelper(link3, @"bazfooquxbartot", ranges);
}

}  // namespace
