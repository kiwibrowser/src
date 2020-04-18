// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/chrome/browser/ui/util/core_text_util.h"

#include "testing/gtest/include/gtest/gtest.h"
#include "testing/gtest_mac.h"
#include "testing/platform_test.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

namespace {

// TODO(crbug.com/601772): Add tests for other core_text_util tests.

using CoreTextUtilTest = PlatformTest;

// Tests that the minimum line height attribute is reflected in GetLineHeight().
TEST_F(CoreTextUtilTest, MinLineHeightText) {
  CGFloat min_line_height = 30.0;
  NSMutableParagraphStyle* style = [[NSMutableParagraphStyle alloc] init];
  [style setMinimumLineHeight:min_line_height];
  NSMutableAttributedString* str = [[NSMutableAttributedString alloc]
      initWithString:@"test"
          attributes:@{NSParagraphStyleAttributeName : style}];
  ASSERT_EQ(min_line_height,
            core_text_util::GetLineHeight(str, NSMakeRange(0, [str length])));
}

// Tests that the maximum line height attribute is reflected in GetLineHeight().
TEST_F(CoreTextUtilTest, MaxLineHeightText) {
  CGFloat max_line_height = 10.0;
  NSMutableParagraphStyle* style = [[NSMutableParagraphStyle alloc] init];
  [style setMaximumLineHeight:max_line_height];
  NSMutableAttributedString* str = [[NSMutableAttributedString alloc]
      initWithString:@"test"
          attributes:@{NSParagraphStyleAttributeName : style}];
  ASSERT_EQ(max_line_height,
            core_text_util::GetLineHeight(str, NSMakeRange(0, [str length])));
}

// Tests that the line height multiple attribute is reflected in
// GetLineHeight().
TEST_F(CoreTextUtilTest, LineHeightMultipleTest) {
  UIFont* font = [UIFont systemFontOfSize:20.0];
  CGFloat font_line_height = font.ascender - font.descender;
  CGFloat line_height_multiple = 2.0;
  NSMutableParagraphStyle* style = [[NSMutableParagraphStyle alloc] init];
  [style setLineHeightMultiple:line_height_multiple];
  NSMutableAttributedString* str = [[NSMutableAttributedString alloc]
      initWithString:@"test"
          attributes:@{
            NSParagraphStyleAttributeName : style,
            NSFontAttributeName : font
          }];
  ASSERT_EQ(font_line_height * line_height_multiple,
            core_text_util::GetLineHeight(str, NSMakeRange(0, [str length])));
}

}  // namespace
