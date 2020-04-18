// Copyright 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/chrome/browser/ui/history_popup/tab_history_popup_controller.h"

#include <memory>
#include <utility>

#include "components/sessions/core/session_types.h"
#import "ios/chrome/browser/ui/history_popup/tab_history_view_controller.h"
#include "ios/chrome/browser/ui/ui_util.h"
#include "ios/web/public/navigation_item.h"
#include "ios/web/public/navigation_item_list.h"
#include "ios/web/public/referrer.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "testing/platform_test.h"
#import "third_party/ocmock/OCMock/OCMock.h"
#include "ui/gfx/ios/uikit_util.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

@interface TabHistoryPopupController (Testing)
+ (CGFloat)popupWidthForItems:(const web::NavigationItemList)items;
@property(nonatomic, strong)
    TabHistoryViewController* tabHistoryTableViewController;
@end

namespace {
static const CGFloat kTabHistoryMinWidth = 250.0;
static const CGFloat kTabHistoryMaxWidthLandscapePhone = 350.0;

class TabHistoryPopupControllerTest : public PlatformTest {
 protected:
  TabHistoryPopupControllerTest() : PlatformTest() {
    parent_ = [[UIView alloc] initWithFrame:[UIScreen mainScreen].bounds];
    // Create test items and populate |items_|.
    web::Referrer referrer(GURL("http://www.example.com"),
                           web::ReferrerPolicyDefault);
    items_.push_back(web::NavigationItem::Create());
    items_.back()->SetURL(GURL("http://www.example.com/0"));
    items_.back()->SetReferrer(referrer);
    items_.push_back(web::NavigationItem::Create());
    items_.back()->SetURL(GURL("http://www.example.com/1"));
    items_.back()->SetReferrer(referrer);
    items_.push_back(web::NavigationItem::Create());
    items_.back()->SetURL(GURL("http://www.example.com/2"));
    items_.back()->SetReferrer(referrer);
    // Create the popup controller using CRWSessionEntries created from the
    // NavigationItems in |items_|.
    popup_ = [[TabHistoryPopupController alloc]
        initWithOrigin:CGPointZero
            parentView:parent_
                 items:web::CreateRawNavigationItemList(items_)
            dispatcher:nil];
  }

  web::ScopedNavigationItemList items_;
  __strong UIView* parent_;
  __strong TabHistoryPopupController* popup_;
};

TEST_F(TabHistoryPopupControllerTest, TestTableSize) {
  NSInteger number_of_rows = 0;

  UICollectionView* collectionView =
      [[popup_ tabHistoryTableViewController] collectionView];

  NSInteger number_of_sections = [collectionView numberOfSections];
  for (NSInteger section = 0; section < number_of_sections; ++section) {
    number_of_rows += [collectionView numberOfItemsInSection:section];
  }

  EXPECT_EQ(3, number_of_rows);
}

TEST_F(TabHistoryPopupControllerTest, TestCalculatePopupWidth) {
  CGFloat minWidth = kTabHistoryMinWidth;
  CGFloat maxWidth = kTabHistoryMinWidth;
  if (!IsIPadIdiom()) {
    UIInterfaceOrientation orientation =
        [[UIApplication sharedApplication] statusBarOrientation];
    if (!UIInterfaceOrientationIsPortrait(orientation))
      maxWidth = kTabHistoryMaxWidthLandscapePhone;
  } else {
    maxWidth = ui::AlignValueToUpperPixel(
        [UIApplication sharedApplication].keyWindow.frame.size.width * .85);
  }

  // Add an item with a short URL and verify that the minimum width is returned.
  web::ScopedNavigationItemList items;
  web::Referrer referrer(GURL("http://www.example.com"),
                         web::ReferrerPolicyDefault);
  items.push_back(web::NavigationItem::Create());
  items.back()->SetURL(GURL("http://foo.com/"));
  items.back()->SetReferrer(referrer);
  web::NavigationItemList raw_items = web::CreateRawNavigationItemList(items);
  CGFloat width = [TabHistoryPopupController popupWidthForItems:raw_items];
  EXPECT_EQ(minWidth, width);

  // Add an item with a medium URL and verify that the returned width is between
  // the minimum and maximum.
  items.push_back(web::NavigationItem::Create());
  items.back()->SetURL(GURL("http://www.example.com/mediumurl"));
  items.back()->SetReferrer(referrer);
  raw_items.push_back(items.back().get());
  width = [TabHistoryPopupController popupWidthForItems:raw_items];
  EXPECT_GE(width, minWidth);
  EXPECT_LE(width, maxWidth);

  // Add an item with a long URL and verify that the maximum width is returned.
  std::string long_url =
      "http://www.example.com/this/is/areally/long/url/that/"
      "is/larger/than/the/maximum/table/width/so/its/text/will/get/cut/off/and/"
      "the/max/width/is/used/";
  items.push_back(web::NavigationItem::Create());
  items.back()->SetURL(GURL(long_url));
  items.back()->SetReferrer(referrer);
  raw_items.push_back(items.back().get());
  width = [TabHistoryPopupController popupWidthForItems:raw_items];
  EXPECT_EQ(maxWidth, width);
}

}  // namespace
