// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/chrome/browser/ui/collection_view/cells/collection_view_text_item.h"

#import <CoreGraphics/CoreGraphics.h>
#import <UIKit/UIKit.h>

#import "ios/chrome/browser/ui/collection_view/cells/collection_view_text_cell.h"
#import "ios/chrome/browser/ui/collection_view/cells/test_utils.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "testing/gtest_mac.h"
#include "testing/platform_test.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

namespace {

using CollectionViewTextItemTest = PlatformTest;

// Test that accessory type is copied over to the cell from the item.
TEST_F(CollectionViewTextItemTest, ConfigureCellPortsAccessoryType) {
  CollectionViewTextItem* item =
      [[CollectionViewTextItem alloc] initWithType:0];
  item.accessoryType = MDCCollectionViewCellAccessoryCheckmark;
  CollectionViewTextCell* cell = [[[item cellClass] alloc] init];
  EXPECT_TRUE([cell isMemberOfClass:[CollectionViewTextCell class]]);
  EXPECT_EQ(MDCCollectionViewCellAccessoryNone, [cell accessoryType]);
  [item configureCell:cell];
  EXPECT_EQ(MDCCollectionViewCellAccessoryCheckmark, [cell accessoryType]);
}

// Test that text properties are copied over to the cell from the item.
TEST_F(CollectionViewTextItemTest, ConfigureCellPortsTextCellProperties) {
  CollectionViewTextItem* item =
      [[CollectionViewTextItem alloc] initWithType:0];
  item.text = @"some text";
  item.detailText = @"some detail text";
  CollectionViewTextCell* cell = [[[item cellClass] alloc] init];
  EXPECT_TRUE([cell isMemberOfClass:[CollectionViewTextCell class]]);
  EXPECT_FALSE([cell textLabel].text);
  EXPECT_FALSE([cell detailTextLabel].text);
  [item configureCell:cell];
  EXPECT_NSEQ(@"some text", [cell textLabel].text);
  EXPECT_NSEQ(@"some detail text", [cell detailTextLabel].text);
}

// Test that if the item has no accessibilityLabel, the cell gets one composed
// of the text and detailText.
TEST_F(CollectionViewTextItemTest, ConfigureCellDerivesAccessibilityLabel) {
  CollectionViewTextItem* item =
      [[CollectionViewTextItem alloc] initWithType:0];
  item.text = @"some text";
  item.detailText = @"some detail text";
  CollectionViewTextCell* cell = [[[item cellClass] alloc] init];
  EXPECT_TRUE([cell isMemberOfClass:[CollectionViewTextCell class]]);
  EXPECT_FALSE([cell textLabel].accessibilityLabel);
  [item configureCell:cell];
  EXPECT_NSEQ(@"some text, some detail text", [cell accessibilityLabel]);
}

// Test that if the item has a non-empty accessibilityLabel, this is copied
// over to the cell.
TEST_F(CollectionViewTextItemTest, ConfigureCellPortsAccessibilityLabel) {
  CollectionViewTextItem* item =
      [[CollectionViewTextItem alloc] initWithType:0];
  item.accessibilityLabel = @"completely different label";
  // Also set text and detailText to verify that the derived
  // accessibilityLabel is not composed of those.
  item.text = @"some text";
  item.detailText = @"some detail text";
  CollectionViewTextCell* cell = [[[item cellClass] alloc] init];
  EXPECT_TRUE([cell isMemberOfClass:[CollectionViewTextCell class]]);
  EXPECT_FALSE([cell textLabel].accessibilityLabel);
  [item configureCell:cell];
  EXPECT_NSEQ(@"completely different label", [cell accessibilityLabel]);
}

}  // namespace
