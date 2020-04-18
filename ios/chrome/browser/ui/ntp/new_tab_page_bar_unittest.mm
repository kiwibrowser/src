// Copyright 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/chrome/browser/ui/ntp/new_tab_page_bar.h"
#import "ios/chrome/browser/ui/content_suggestions/ntp_home_constant.h"
#import "ios/chrome/browser/ui/ntp/new_tab_page_bar_item.h"
#include "ios/chrome/browser/ui/ui_util.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "testing/gtest_mac.h"
#include "testing/platform_test.h"
#import "third_party/ocmock/OCMock/OCMock.h"
#import "third_party/ocmock/gtest_support.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

@interface NewTabPageBar (Testing)
- (void)buttonDidTap:(UIButton*)button;
@end

namespace {

class NewTabPageBarTest : public PlatformTest {
 protected:
  void SetUp() override {
    CGRect frame = CGRectMake(0, 0, 320, 44);
    bar_ = [[NewTabPageBar alloc] initWithFrame:frame];
  };
  NewTabPageBar* bar_;
};

TEST_F(NewTabPageBarTest, SetItems) {
  NewTabPageBarItem* firstItem = [NewTabPageBarItem
      newTabPageBarItemWithTitle:@"Home"
                      identifier:ntp_home::HOME_PANEL
                           image:[UIImage imageNamed:@"ntp_bookmarks"]];
  // Tests that identifier test function can return both true and false.
  EXPECT_TRUE(firstItem.identifier == ntp_home::HOME_PANEL);

  NewTabPageBarItem* secondItem = [NewTabPageBarItem
      newTabPageBarItemWithTitle:@"Bookmarks"
                      identifier:ntp_home::BOOKMARKS_PANEL
                           image:[UIImage imageNamed:@"ntp_bookmarks"]];
  NewTabPageBarItem* thirdItem = [NewTabPageBarItem
      newTabPageBarItemWithTitle:@"RecentTabs"
                      identifier:ntp_home::RECENT_TABS_PANEL
                           image:[UIImage imageNamed:@"ntp_bookmarks"]];

  [bar_ setItems:[NSArray arrayWithObject:firstItem]];
  EXPECT_EQ(bar_.buttons.count, 1U);
  [bar_ setItems:[NSArray arrayWithObjects:firstItem, secondItem, nil]];
  EXPECT_EQ(bar_.buttons.count, 2U);
  [bar_ setItems:[NSArray
                     arrayWithObjects:firstItem, secondItem, thirdItem, nil]];
  EXPECT_EQ(bar_.buttons.count, 3U);
  [bar_ setItems:[NSArray arrayWithObject:firstItem]];
  EXPECT_EQ(bar_.buttons.count, 1U);
}

}  // anonymous namespace
