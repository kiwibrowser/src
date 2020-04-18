// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/chrome/browser/ui/settings/cells/encryption_item.h"

#include "testing/gtest/include/gtest/gtest.h"
#import "testing/gtest_mac.h"
#include "testing/platform_test.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

namespace {

using EncryptionItemTest = PlatformTest;

// Tests that the text label, enabled status and accessory type are set properly
// after a call to |configureCell:|.
TEST_F(EncryptionItemTest, ConfigureCell) {
  EncryptionItem* item = [[EncryptionItem alloc] initWithType:0];
  EncryptionCell* cell = [[[item cellClass] alloc] init];
  EXPECT_TRUE([cell isMemberOfClass:[EncryptionCell class]]);
  EXPECT_NSEQ(nil, cell.textLabel.text);
  NSString* text = @"Test text";
  UIColor* enabledColor = cell.textLabel.textColor;

  item.text = text;
  item.enabled = NO;
  item.accessoryType = MDCCollectionViewCellAccessoryCheckmark;
  [item configureCell:cell];

  EXPECT_NSEQ(text, cell.textLabel.text);
  EXPECT_NE(enabledColor, cell.textLabel.textColor);
  EXPECT_EQ(MDCCollectionViewCellAccessoryCheckmark, cell.accessoryType);
}

}  // namespace
