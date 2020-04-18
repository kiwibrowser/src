// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/chrome/browser/ui/bookmarks/bookmark_home_view_controller.h"

#include "base/test/scoped_feature_list.h"
#include "components/bookmarks/browser/bookmark_model.h"
#import "ios/chrome/browser/browser_state/test_chrome_browser_state.h"
#include "ios/chrome/browser/ui/bookmarks/bookmark_ios_unittest.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

namespace {

using BookmarkHomeViewControllerTest = BookmarkIOSUnitTest;

TEST_F(BookmarkHomeViewControllerTest,
       TableViewPopulatedAfterBookmarkModelLoaded) {
  @autoreleasepool {
    BookmarkHomeViewController* controller = [[BookmarkHomeViewController alloc]
        initWithLoader:nil
          browserState:chrome_browser_state_.get()
            dispatcher:nil];

    [controller setRootNode:_bookmarkModel->mobile_node()];
    EXPECT_EQ(1, [controller numberOfSectionsInTableView:controller.tableView]);
  }
}

}  // namespace
