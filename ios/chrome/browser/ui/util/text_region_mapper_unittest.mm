// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import <UIKit/UIKit.h>

#include "base/mac/foundation_util.h"
#include "ios/chrome/browser/ui/util/text_region_mapper.h"
#import "ios/third_party/material_components_ios/src/components/Typography/src/MaterialTypography.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "testing/gtest_mac.h"
#include "testing/platform_test.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

namespace {
class TextRegionMapperTest : public PlatformTest {
 protected:
  void InitMapper(NSAttributedString* string, CGRect bounds) {
    _string = [string copy];
    _textMapper =
        [[CoreTextRegionMapper alloc] initWithAttributedString:_string
                                                        bounds:bounds];
  }

  CGRect RectAtIndex(NSArray* array, NSUInteger index) {
    NSValue* value = base::mac::ObjCCastStrict<NSValue>(array[index]);
    return [value CGRectValue];
  }

  NSDictionary* AttributesForTextAlignment(NSTextAlignment alignment) {
    NSMutableParagraphStyle* style =
        [[NSMutableParagraphStyle defaultParagraphStyle] mutableCopy];
    [style setAlignment:alignment];
    return @{NSParagraphStyleAttributeName : style};
  }

  NSAttributedString* _string;
  CoreTextRegionMapper* _textMapper;
};
}

TEST_F(TextRegionMapperTest, SimpleTextTest) {
  CGRect bounds = CGRectMake(0, 0, 500, 100);
  NSString* string = @"Simple Test";
  InitMapper([[NSAttributedString alloc] initWithString:string], bounds);

  // Simple case: a single word in a string in a large bounding rect.
  // Rect bounding word should be inside bounding rect, and should be
  // wider than it is tall.
  NSRange range = NSMakeRange(0, 6);  // "Simple".
  NSArray* rects = [_textMapper rectsForRange:range];
  ASSERT_EQ(1UL, [rects count]);
  CGRect rect = RectAtIndex(rects, 0);
  EXPECT_TRUE(CGRectContainsRect(bounds, rect));
  EXPECT_GT(CGRectGetWidth(rect), CGRectGetHeight(rect));

  // Range that ends at end of string still generates a rect.
  range = NSMakeRange(9, 2);
  rects = [_textMapper rectsForRange:range];
  ASSERT_EQ(1UL, [rects count]);
  rect = RectAtIndex(rects, 0);
  EXPECT_TRUE(CGRectContainsRect(bounds, rect));

  // Simple null cases: range of zero length, range outside of string.
  range = NSMakeRange(6, 0);
  rects = [_textMapper rectsForRange:range];
  EXPECT_EQ(0UL, [rects count]) << "Range has length 0.";

  range = NSMakeRange([_string length] + 1, 5);
  rects = [_textMapper rectsForRange:range];
  EXPECT_EQ(0UL, [rects count]) << "Range starts beyond string.";

  range = NSMakeRange([_string length] - 2, 3);
  rects = [_textMapper rectsForRange:range];
  EXPECT_EQ(0UL, [rects count]) << "Range ends beyond string.";
}

TEST_F(TextRegionMapperTest, TextAlignmentTest) {
  CGRect bounds = CGRectMake(0, 0, 500, 100);
  NSAttributedString* string;
  string = [[NSAttributedString alloc]
      initWithString:@"Simple Test"
          attributes:AttributesForTextAlignment(NSTextAlignmentLeft)];
  InitMapper(string, bounds);

  // First word of left-aligned string should at the left edge of the bounds.
  NSRange range = NSMakeRange(0, 6);  // "Simple".
  NSArray* rects = [_textMapper rectsForRange:range];
  ASSERT_EQ(1UL, [rects count]);
  CGRect rect = RectAtIndex(rects, 0);
  EXPECT_TRUE(CGRectContainsRect(bounds, rect));
  EXPECT_GT(CGRectGetWidth(rect), CGRectGetHeight(rect));
  EXPECT_EQ(CGRectGetMinX(rect), CGRectGetMinX(bounds));

  string = [[NSAttributedString alloc]
      initWithString:@"Simple Test"
          attributes:AttributesForTextAlignment(NSTextAlignmentRight)];
  InitMapper(string, bounds);

  // Last word of right-aligned string should at the right edge of the bounds.
  range = NSMakeRange(6, 5);  // "Test".
  rects = [_textMapper rectsForRange:range];
  ASSERT_EQ(1UL, [rects count]);
  rect = RectAtIndex(rects, 0);
  EXPECT_TRUE(CGRectContainsRect(bounds, rect));
  EXPECT_GT(CGRectGetWidth(rect), CGRectGetHeight(rect));
  EXPECT_EQ(CGRectGetMaxX(rect), CGRectGetMaxX(bounds));
}

TEST_F(TextRegionMapperTest, CJKTest) {
  CGRect bounds = CGRectMake(0, 0, 345, 65);
  // clang-format off
  NSString* CJKString =
      @"“触摸搜索”会将所选字词和当前页面（作为上下文）一起发送给 Google 搜索。"
      @"您可以在设置中停用此功能。";
  // clang-format on
  NSMutableDictionary* attributes = [NSMutableDictionary
      dictionaryWithDictionary:AttributesForTextAlignment(NSTextAlignmentLeft)];
  attributes[NSFontAttributeName] =
      [[MDCTypography fontLoader] regularFontOfSize:16];
  NSAttributedString* string =
      [[NSAttributedString alloc] initWithString:CJKString
                                      attributes:attributes];
  InitMapper(string, bounds);

  NSRange range = NSMakeRange(0, 6);  // "“触摸搜索”".
  NSArray* rects = [_textMapper rectsForRange:range];
  ASSERT_EQ(1UL, [rects count]);
}

/*
 Further unit tests should cover additional cases:
 - In several languages.
 - Range split across line break == two rects
 - Bidi text with text range across scripts gets two rects.
 - Various string attributions.
 - Isolate buggy CoreText case.
*/
