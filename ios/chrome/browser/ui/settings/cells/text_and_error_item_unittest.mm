// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/chrome/browser/ui/settings/cells/text_and_error_item.h"
#import "ios/third_party/material_components_ios/src/components/Palettes/src/MaterialPalettes.h"

#include "testing/gtest/include/gtest/gtest.h"
#import "testing/gtest_mac.h"
#include "testing/platform_test.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

namespace {

using TextAndErrorItemTest = PlatformTest;

// Tests that the error UIImage and UILabels are set properly after a call to
// |configureCell:|.
TEST_F(TextAndErrorItemTest, ImageViewAndLabels) {
  TextAndErrorItem* item = [[TextAndErrorItem alloc] initWithType:0];
  item.text = @"Test";
  item.shouldDisplayError = YES;
  item.enabled = YES;

  // Text color options.
  UIColor* enabledColor = [[MDCPalette greyPalette] tint900];
  UIColor* disabledColor = [[MDCPalette greyPalette] tint500];

  id cell = [[[item cellClass] alloc] init];
  ASSERT_TRUE([cell isMemberOfClass:[TextAndErrorCell class]]);

  TextAndErrorCell* signInCell = cell;
  EXPECT_FALSE(signInCell.errorIcon.image);
  EXPECT_FALSE(signInCell.textLabel.text);

  [item configureCell:cell];
  EXPECT_TRUE(signInCell.errorIcon.image);
  EXPECT_NSEQ(item.text, signInCell.textLabel.text);
  EXPECT_NSEQ(enabledColor, signInCell.textLabel.textColor);

  item.shouldDisplayError = NO;

  [item configureCell:cell];
  EXPECT_FALSE(signInCell.errorIcon.image);
  EXPECT_NSEQ(item.text, signInCell.textLabel.text);

  item.enabled = NO;
  [item configureCell:cell];
  EXPECT_NSEQ(disabledColor, signInCell.textLabel.textColor);
}

}  // namespace
