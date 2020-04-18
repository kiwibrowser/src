// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/chrome/browser/ui/fancy_ui/bidi_container_view.h"

#include "base/i18n/rtl.h"
#include "testing/platform_test.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

namespace {

class BidiContainerViewTest : public PlatformTest {
 protected:
  void SetUp() override { defaultLocale_ = base::i18n::GetConfiguredLocale(); };
  void TearDown() override { base::i18n::SetICUDefaultLocale(defaultLocale_); }
  UIViewAutoresizing AutoresizingMaskForLocale(const char* locale,
                                               UIViewAutoresizing autoresizing);
  std::string defaultLocale_;
};

UIViewAutoresizing BidiContainerViewTest::AutoresizingMaskForLocale(
    const char* locale,
    UIViewAutoresizing autoresizing) {
  base::i18n::SetICUDefaultLocale(base::i18n::GetCanonicalLocale(locale));
  BidiContainerView* view =
      [[BidiContainerView alloc] initWithFrame:CGRectMake(0, 0, 100, 100)];
  UILabel* label = [[UILabel alloc] initWithFrame:CGRectMake(20, 30, 40, 50)];
  [label setAutoresizingMask:autoresizing];
  [view addSubview:label];
  [view layoutSubviews];
  return [label autoresizingMask];
}

TEST_F(BidiContainerViewTest, InitializeLeftToRight) {
  base::i18n::SetICUDefaultLocale(
      base::i18n::GetCanonicalLocale("en" /* English */));
  BidiContainerView* view =
      [[BidiContainerView alloc] initWithFrame:CGRectMake(0, 0, 100, 100)];
  UILabel* label = [[UILabel alloc] initWithFrame:CGRectMake(20, 30, 40, 50)];
  [view addSubview:label];
  [view layoutSubviews];
  CGRect labelFrame = [label frame];
  EXPECT_EQ(20, labelFrame.origin.x);
  EXPECT_EQ(30, labelFrame.origin.y);
  EXPECT_EQ(40, labelFrame.size.width);
  EXPECT_EQ(50, labelFrame.size.height);
}

TEST_F(BidiContainerViewTest, InitializeRightToLeft) {
  base::i18n::SetICUDefaultLocale(
      base::i18n::GetCanonicalLocale("he" /* Hebrew */));
  BidiContainerView* view =
      [[BidiContainerView alloc] initWithFrame:CGRectMake(0, 0, 100, 100)];
  UILabel* label = [[UILabel alloc] initWithFrame:CGRectMake(20, 30, 40, 50)];
  [view addSubview:label];
  [view layoutSubviews];
  CGRect labelFrame = [label frame];
  EXPECT_EQ(100 - 20 - 40 /* view.width - label.originX - label.width */,
            labelFrame.origin.x);
  EXPECT_EQ(30, labelFrame.origin.y);
  EXPECT_EQ(40, labelFrame.size.width);
  EXPECT_EQ(50, labelFrame.size.height);
}

TEST_F(BidiContainerViewTest, InitializeRightToLeftTwoViews) {
  base::i18n::SetICUDefaultLocale(
      base::i18n::GetCanonicalLocale("he" /* Hebrew */));
  BidiContainerView* view =
      [[BidiContainerView alloc] initWithFrame:CGRectMake(0, 0, 100, 100)];
  UILabel* label1 = [[UILabel alloc] initWithFrame:CGRectMake(20, 30, 40, 50)];
  [view addSubview:label1];
  UILabel* label2 = [[UILabel alloc] initWithFrame:CGRectMake(60, 30, 30, 50)];
  [view addSubview:label2];
  [view layoutSubviews];
  EXPECT_EQ(100 - 20 - 40 /* view.width - label1.originX - label1.width */,
            [label1 frame].origin.x);
  EXPECT_EQ(100 - 60 - 30 /* view.width - label2.originX - label2.width */,
            [label2 frame].origin.x);
}

TEST_F(BidiContainerViewTest, SetAutoresizingFlexibleLeftForRightToLeft) {
  UIViewAutoresizing otherFlags =
      UIViewAutoresizingFlexibleHeight | UIViewAutoresizingFlexibleBottomMargin;
  UIViewAutoresizing testedFlags = UIViewAutoresizingFlexibleLeftMargin;
  UIViewAutoresizing adjustedFlags =
      AutoresizingMaskForLocale("he" /* Hebrew */, otherFlags | testedFlags);
  EXPECT_EQ(UIViewAutoresizingFlexibleRightMargin | otherFlags, adjustedFlags);
  EXPECT_FALSE(UIViewAutoresizingFlexibleLeftMargin & adjustedFlags);
}

TEST_F(BidiContainerViewTest, SetAutoresizingFlexibleLeftForLeftToRight) {
  UIViewAutoresizing otherFlags =
      UIViewAutoresizingFlexibleHeight | UIViewAutoresizingFlexibleBottomMargin;
  UIViewAutoresizing testedFlags = UIViewAutoresizingFlexibleLeftMargin;
  UIViewAutoresizing adjustedFlags =
      AutoresizingMaskForLocale("en" /* English */, otherFlags | testedFlags);
  EXPECT_EQ(UIViewAutoresizingFlexibleLeftMargin | otherFlags, adjustedFlags);
  EXPECT_FALSE(UIViewAutoresizingFlexibleRightMargin & adjustedFlags);
}

TEST_F(BidiContainerViewTest, SetAutoresizingFlexibleRightForRightToLeft) {
  UIViewAutoresizing otherFlags = UIViewAutoresizingNone;
  UIViewAutoresizing testedFlags = UIViewAutoresizingFlexibleRightMargin;
  UIViewAutoresizing adjustedFlags =
      AutoresizingMaskForLocale("he" /* Hebrew */, otherFlags | testedFlags);
  EXPECT_EQ(UIViewAutoresizingFlexibleLeftMargin | otherFlags, adjustedFlags);
  EXPECT_FALSE(UIViewAutoresizingFlexibleRightMargin & adjustedFlags);
}

TEST_F(BidiContainerViewTest, SetAutoresizingFlexibleRightForLeftToRight) {
  UIViewAutoresizing otherFlags = UIViewAutoresizingNone;
  UIViewAutoresizing testedFlags = UIViewAutoresizingFlexibleRightMargin;
  UIViewAutoresizing adjustedFlags =
      AutoresizingMaskForLocale("en" /* English */, otherFlags | testedFlags);
  EXPECT_EQ(UIViewAutoresizingFlexibleRightMargin | otherFlags, adjustedFlags);
  EXPECT_FALSE(UIViewAutoresizingFlexibleLeftMargin & adjustedFlags);
}

TEST_F(BidiContainerViewTest, SetAutoresizingFlexibleBothForRightToLeft) {
  UIViewAutoresizing otherFlags =
      UIViewAutoresizingFlexibleWidth | UIViewAutoresizingFlexibleTopMargin;
  UIViewAutoresizing testedFlags = UIViewAutoresizingFlexibleRightMargin |
                                   UIViewAutoresizingFlexibleLeftMargin;
  UIViewAutoresizing adjustedFlags =
      AutoresizingMaskForLocale("he" /* Hebrew */, otherFlags | testedFlags);
  EXPECT_EQ(UIViewAutoresizingFlexibleRightMargin |
                UIViewAutoresizingFlexibleLeftMargin | otherFlags,
            adjustedFlags);
}

TEST_F(BidiContainerViewTest, SetAutoresizingFlexibleBothForLeftToRight) {
  UIViewAutoresizing otherFlags =
      UIViewAutoresizingFlexibleWidth | UIViewAutoresizingFlexibleTopMargin;
  UIViewAutoresizing testedFlags = UIViewAutoresizingFlexibleRightMargin |
                                   UIViewAutoresizingFlexibleLeftMargin;
  UIViewAutoresizing adjustedFlags =
      AutoresizingMaskForLocale("en" /* English */, otherFlags | testedFlags);
  EXPECT_EQ(UIViewAutoresizingFlexibleRightMargin |
                UIViewAutoresizingFlexibleLeftMargin | otherFlags,
            adjustedFlags);
}

TEST_F(BidiContainerViewTest, SetAutoresizingFlexibleNoneForRightToLeft) {
  UIViewAutoresizing flags = UIViewAutoresizingNone;
  UIViewAutoresizing adjustedFlags =
      AutoresizingMaskForLocale("he" /* Hebrew */, flags);
  EXPECT_EQ(flags, adjustedFlags);
}

TEST_F(BidiContainerViewTest, SetAutoresizingFlexibleNoneForLeftToRight) {
  UIViewAutoresizing flags = UIViewAutoresizingNone;
  UIViewAutoresizing adjustedFlags =
      AutoresizingMaskForLocale("en" /* English */, flags);
  EXPECT_EQ(flags, adjustedFlags);
}

}  // namespace
