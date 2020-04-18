// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/chrome/browser/ui/collection_view/cells/activity_indicator_cell.h"

#import "ios/chrome/browser/ui/collection_view/cells/collection_view_item.h"
#import "ios/third_party/material_components_ios/src/components/ActivityIndicator/src/MaterialActivityIndicator.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "testing/platform_test.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

namespace {

using ActivityIndicatorItemTest = PlatformTest;

// Tests that when an ActivityIndicatorCell is configured, it has a
// MDCActivityIndicator that is animating.
TEST_F(ActivityIndicatorItemTest, CellDisplaysActivityIndicator) {
  CollectionViewItem* item = [[CollectionViewItem alloc] initWithType:0];
  item.cellClass = [ActivityIndicatorCell class];
  ActivityIndicatorCell* cell =
      [[ActivityIndicatorCell alloc] initWithFrame:CGRectZero];
  [item configureCell:cell];
  MDCActivityIndicator* activity_indicator = cell.activityIndicator;
  EXPECT_TRUE([activity_indicator isDescendantOfView:cell]);
  EXPECT_TRUE(activity_indicator.isAnimating);
}
}
