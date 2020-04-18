// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/chrome/browser/ui/settings/cells/password_details_item.h"

#include "testing/gtest/include/gtest/gtest.h"
#import "testing/gtest_mac.h"
#include "testing/platform_test.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

namespace {

using PasswordDetailsItemTest = PlatformTest;

// Tests that the text label and showing status are set properly after a call to
// |configureCell:|.
TEST_F(PasswordDetailsItemTest, ConfigureCell) {
  PasswordDetailsItem* item = [[PasswordDetailsItem alloc] initWithType:0];
  PasswordDetailsCell* cell = [[[item cellClass] alloc] init];
  EXPECT_TRUE([cell isMemberOfClass:[PasswordDetailsCell class]]);
  EXPECT_NSEQ(nil, cell.textLabel.text);
  NSString* text = @"Test text";
  NSString* obscuredText = @"•••••••••";

  item.text = text;
  [item configureCell:cell];
  EXPECT_NSEQ(obscuredText, cell.textLabel.text);

  item.showingText = YES;
  [item configureCell:cell];
  EXPECT_NSEQ(text, cell.textLabel.text);

  item.showingText = NO;
  [item configureCell:cell];
  EXPECT_NSEQ(obscuredText, cell.textLabel.text);
}

}  // namespace
