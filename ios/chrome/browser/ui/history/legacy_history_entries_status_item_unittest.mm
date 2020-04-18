// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/chrome/browser/ui/history/legacy_history_entries_status_item.h"

#import "base/test/ios/wait_util.h"
#include "components/strings/grit/components_strings.h"
#include "ios/chrome/browser/chrome_url_constants.h"
#import "ios/chrome/browser/ui/collection_view/cells/collection_view_footer_item.h"
#import "ios/chrome/browser/ui/history/history_entries_status_item_delegate.h"
#import "ios/chrome/browser/ui/util/label_link_controller.h"
#import "ios/chrome/common/string_util.h"
#include "ios/chrome/grit/ios_strings.h"
#include "testing/gtest/include/gtest/gtest.h"
#import "testing/gtest_mac.h"
#include "testing/platform_test.h"
#include "third_party/ocmock/OCMock/OCMock.h"
#include "third_party/ocmock/gtest_support.h"
#include "ui/base/l10n/l10n_util.h"
#include "ui/base/l10n/l10n_util_mac.h"
#include "url/gurl.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

// Mock delegate for HistoryEntriesStatusItem. Implement mock delegate rather
// than use OCMock because delegate method takes a GURL as a parameter.
@interface MockEntriesStatusItemDelegate
    : NSObject<HistoryEntriesStatusItemDelegate>
@property(nonatomic) BOOL delegateCalledForSyncURL;
@property(nonatomic) BOOL delegateCalledForBrowsingDataURL;
@end

@implementation MockEntriesStatusItemDelegate
@synthesize delegateCalledForSyncURL = _delegateCalledForSyncURL;
@synthesize delegateCalledForBrowsingDataURL =
    _delegateCalledForBrowsingDataURL;
- (void)historyEntriesStatusItem:(LegacyHistoryEntriesStatusItem*)item
               didRequestOpenURL:(const GURL&)URL {
  GURL browsingDataURL(kHistoryMyActivityURL);
  if (URL == browsingDataURL) {
    self.delegateCalledForBrowsingDataURL = YES;
  }
}
@end

namespace {

using LegacyHistoryEntriesStatusItemTest = PlatformTest;

// Tests that configuring a cell for HistoryEntriesStatusItem with hidden
// property set to YES results in an empty label.
TEST_F(LegacyHistoryEntriesStatusItemTest, TestHidden) {
  LegacyHistoryEntriesStatusItem* item =
      [[LegacyHistoryEntriesStatusItem alloc] initWithType:0];
  item.hidden = YES;
  LegacyHistoryEntriesStatusCell* cell =
      [[LegacyHistoryEntriesStatusCell alloc] init];
  [item configureCell:cell];
  EXPECT_FALSE(cell.textLabel.text);
}

// Tests that tapping on links on a configured cell invokes
// the HistoryEntriesStatusItemDelegate method.
TEST_F(LegacyHistoryEntriesStatusItemTest, TestDelegate) {
  LegacyHistoryEntriesStatusItem* item =
      [[LegacyHistoryEntriesStatusItem alloc] initWithType:0];
  LegacyHistoryEntriesStatusCell* cell =
      [[LegacyHistoryEntriesStatusCell alloc] init];
  MockEntriesStatusItemDelegate* delegate =
      [[MockEntriesStatusItemDelegate alloc] init];
  item.delegate = delegate;
  item.hidden = NO;
  [item configureCell:cell];

  // Layout the cell so that links are drawn.
  cell.frame = CGRectMake(0, 0, 360, 100);
  [cell setNeedsLayout];
  [cell layoutIfNeeded];

  // Tap link for more info on browsing data.
  GURL browsing_data_url(kHistoryMyActivityURL);
  CGRect browsing_data_rect = [[[cell.labelLinkController
      tapRectsForURL:browsing_data_url] objectAtIndex:0] CGRectValue];
  [cell.labelLinkController
      tapLabelAtPoint:CGPointMake(CGRectGetMidX(browsing_data_rect),
                                  CGRectGetMidY(browsing_data_rect))];

  base::test::ios::WaitUntilCondition(^bool() {
    return delegate.delegateCalledForBrowsingDataURL;
  });
}
}  // namespace
