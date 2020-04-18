// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/chrome/browser/ui/settings/cells/autofill_data_item.h"

#import "ios/third_party/material_components_ios/src/components/CollectionCells/src/MaterialCollectionCells.h"
#include "testing/gtest/include/gtest/gtest.h"
#import "testing/gtest_mac.h"
#include "testing/platform_test.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

using AutofillDataItemTest = PlatformTest;

// Tests that the UILabels are set properly after a call to |configureCell:|.
TEST_F(AutofillDataItemTest, TextLabels) {
  AutofillDataItem* item = [[AutofillDataItem alloc] initWithType:0];
  NSString* mainText = @"Main text";
  NSString* leadingDetailText = @"Leading detail text";
  NSString* trailingDetailText = @"Trailing detail text";

  item.text = mainText;
  item.leadingDetailText = leadingDetailText;
  item.trailingDetailText = trailingDetailText;
  item.accessoryType = MDCCollectionViewCellAccessoryCheckmark;

  id cell = [[[item cellClass] alloc] init];
  ASSERT_TRUE([cell isMemberOfClass:[AutofillDataCell class]]);

  AutofillDataCell* autofillDataCell = cell;
  EXPECT_FALSE(autofillDataCell.textLabel.text);
  EXPECT_FALSE(autofillDataCell.leadingDetailTextLabel.text);
  EXPECT_FALSE(autofillDataCell.trailingDetailTextLabel.text);
  EXPECT_EQ(MDCCollectionViewCellAccessoryNone, autofillDataCell.accessoryType);

  [item configureCell:cell];
  EXPECT_NSEQ(mainText, autofillDataCell.textLabel.text);
  EXPECT_NSEQ(leadingDetailText, autofillDataCell.leadingDetailTextLabel.text);
  EXPECT_NSEQ(trailingDetailText,
              autofillDataCell.trailingDetailTextLabel.text);
  EXPECT_EQ(MDCCollectionViewCellAccessoryCheckmark,
            autofillDataCell.accessoryType);
}
