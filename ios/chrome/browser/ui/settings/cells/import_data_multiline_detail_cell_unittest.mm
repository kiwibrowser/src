// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/chrome/browser/ui/settings/cells/import_data_multiline_detail_cell.h"

#import "ios/chrome/browser/ui/collection_view/cells/collection_view_detail_item.h"
#include "testing/gtest/include/gtest/gtest.h"
#import "testing/gtest_mac.h"
#include "testing/platform_test.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

namespace {

using ImportDataMultilineDetailItemTest = PlatformTest;

// Tests that the text and detail text are honoured after a call to
// |configureCell:|.
TEST_F(ImportDataMultilineDetailItemTest, ConfigureCell) {
  CollectionViewDetailItem* item =
      [[CollectionViewDetailItem alloc] initWithType:0];
  item.cellClass = [ImportDataMultilineDetailCell class];
  NSString* text = @"Test Text";
  NSString* detailText = @"Test Detail Text that can span multiple lines. For "
                         @"example, this line probably fits on three or four "
                         @"lines.";

  item.text = text;
  item.detailText = detailText;

  id cell = [[[item cellClass] alloc] init];
  ASSERT_TRUE([cell isMemberOfClass:[ImportDataMultilineDetailCell class]]);

  ImportDataMultilineDetailCell* multilineDetailCell =
      static_cast<ImportDataMultilineDetailCell*>(cell);
  EXPECT_FALSE(multilineDetailCell.textLabel.text);
  EXPECT_FALSE(multilineDetailCell.detailTextLabel.text);

  [item configureCell:cell];
  EXPECT_NSEQ(text, multilineDetailCell.textLabel.text);
  EXPECT_NSEQ(detailText, multilineDetailCell.detailTextLabel.text);
}

// Tests that the text label of an ImportDataMultilineDetailCell only has one
// line but the detail text label spans multiple lines.
TEST_F(ImportDataMultilineDetailItemTest, MultipleLines) {
  ImportDataMultilineDetailCell* cell =
      [[ImportDataMultilineDetailCell alloc] init];
  EXPECT_EQ(1, cell.textLabel.numberOfLines);
  EXPECT_EQ(0, cell.detailTextLabel.numberOfLines);
}

}  // namespace
