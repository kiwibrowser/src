// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/chrome/browser/ui/table_view/cells/table_view_text_item.h"

#include "base/mac/foundation_util.h"
#import "ios/chrome/browser/ui/table_view/chrome_table_view_styler.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "testing/gtest_mac.h"
#include "testing/platform_test.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

namespace {
using TableViewTextItemTest = PlatformTest;
}

// Tests that the UILabels are set properly after a call to |configureCell:|.
TEST_F(TableViewTextItemTest, TextLabels) {
  NSString* text = @"Cell text";

  TableViewTextItem* item = [[TableViewTextItem alloc] initWithType:0];
  item.text = text;

  id cell = [[[item cellClass] alloc] init];
  ASSERT_TRUE([cell isMemberOfClass:[TableViewTextCell class]]);

  TableViewTextCell* textCell =
      base::mac::ObjCCastStrict<TableViewTextCell>(cell);
  EXPECT_FALSE(textCell.textLabel.text);

  ChromeTableViewStyler* styler = [[ChromeTableViewStyler alloc] init];
  [item configureCell:textCell withStyler:styler];
  EXPECT_NSEQ(text, textCell.textLabel.text);
}

TEST_F(TableViewTextItemTest, ConfigureCellWithStyler) {
  TableViewTextItem* item = [[TableViewTextItem alloc] initWithType:0];
  TableViewTextCell* cell = [[[item cellClass] alloc] init];
  ASSERT_TRUE([cell isMemberOfClass:[TableViewTextCell class]]);

  ChromeTableViewStyler* styler = [[ChromeTableViewStyler alloc] init];
  UIColor* testColor = [UIColor redColor];
  styler.tableViewBackgroundColor = testColor;
  [item configureCell:cell withStyler:styler];
  EXPECT_NSEQ(testColor, cell.textLabel.backgroundColor);
}
