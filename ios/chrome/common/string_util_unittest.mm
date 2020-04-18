// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ios/chrome/common/string_util.h"

#import <UIKit/UIKit.h>

#include "testing/gtest/include/gtest/gtest.h"
#include "testing/gtest_mac.h"
#include "testing/platform_test.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

namespace {

using StringUtilTest = PlatformTest;

TEST_F(StringUtilTest, ParseStringWithLink) {
  NSArray* const all_test_data = @[
    @{
      @"input" : @"Text without link.",
      @"expected" : @"Text without link.",
      @"link range" : [NSValue valueWithRange:NSMakeRange(NSNotFound, 0)]
    },
    @{
      @"input" : @"Text with empty link BEGIN_LINK END_LINK.",
      @"expected" : @"Text with empty link .",
      @"link range" : [NSValue valueWithRange:NSMakeRange(21, 0)]
    },
    @{
      @"input" : @"Text with BEGIN_LINK and no end link.",
      @"expected" : @"Text with BEGIN_LINK and no end link.",
      @"link range" : [NSValue valueWithRange:NSMakeRange(NSNotFound, 0)]
    },
    @{
      @"input" : @"Text with valid BEGIN_LINK link END_LINK and spaces.",
      @"expected" : @"Text with valid link and spaces.",
      @"link range" : [NSValue valueWithRange:NSMakeRange(16, 4)]
    },
    @{
      @"input" : @"Text with valid BEGIN_LINKlinkEND_LINK and no spaces.",
      @"expected" : @"Text with valid link and no spaces.",
      @"link range" : [NSValue valueWithRange:NSMakeRange(16, 4)]
    }
  ];
  for (NSDictionary* test_data : all_test_data) {
    NSString* input_text = test_data[@"input"];
    NSString* expected_text = test_data[@"expected"];
    NSRange expected_range = [test_data[@"link range"] rangeValue];

    EXPECT_NSEQ(expected_text, ParseStringWithLink(input_text, nullptr));

    // Initialize |range| with some values that are not equal to the expected
    // ones.
    NSRange range = NSMakeRange(1000, 2000);
    EXPECT_NSEQ(expected_text, ParseStringWithLink(input_text, &range));
    EXPECT_EQ(expected_range.location, range.location);
    EXPECT_EQ(expected_range.length, range.length);
  }
}

TEST_F(StringUtilTest, CleanNSStringForDisplay) {
  NSArray* const all_test_data = @[
    @{
      @"input" : @"Clean String",
      @"remove_graphic_chars" : @NO,
      @"expected" : @"Clean String"
    },
    @{
      @"input" : @"  \t String with leading and trailing  whitespaces   ",
      @"remove_graphic_chars" : @NO,
      @"expected" : @"String with leading and trailing whitespaces"
    },
    @{
      @"input" : @"  \n\n\r String with \n\n\r\n\r newline characters \n\n\r",
      @"remove_graphic_chars" : @NO,
      @"expected" : @"String with newline characters"
    },
    @{
      @"input" : @"String with an   arrow ⟰ that remains intact",
      @"remove_graphic_chars" : @NO,
      @"expected" : @"String with an arrow ⟰ that remains intact"
    },
    @{
      @"input" : @"String with an   arrow ⟰ that gets cleaned up",
      @"remove_graphic_chars" : @YES,
      @"expected" : @"String with an arrow that gets cleaned up"
    },
  ];

  for (NSDictionary* test_data : all_test_data) {
    NSString* input_text = test_data[@"input"];
    NSString* expected_text = test_data[@"expected"];
    BOOL remove_graphic_chars = [test_data[@"remove_graphic_chars"] boolValue];

    EXPECT_NSEQ(expected_text,
                CleanNSStringForDisplay(input_text, remove_graphic_chars));
  }
}

using StringUnitTest = PlatformTest;

TEST_F(StringUnitTest, SubstringOfWidth) {
  // returns nil for zero-length strings
  EXPECT_NSEQ(SubstringOfWidth(@"", @{}, 100, NO), nil);
  EXPECT_NSEQ(SubstringOfWidth(nil, @{}, 100, NO), nil);

  // This font should always exist
  UIFont* sys_font = [UIFont systemFontOfSize:18.0f];
  NSDictionary* attributes = @{NSFontAttributeName : sys_font};

  EXPECT_NSEQ(SubstringOfWidth(@"asdf", attributes, 100, NO), @"asdf");

  // construct some string of known lengths
  NSString* leading = @"some text";
  NSString* trailing = @"some other text";
  NSString* mid = @"some text for the method to do some actual work";
  NSString* long_string =
      [[leading stringByAppendingString:mid] stringByAppendingString:trailing];

  CGFloat leading_width = [leading sizeWithAttributes:attributes].width;
  CGFloat trailing_width = [trailing sizeWithAttributes:attributes].width;

  NSString* leading_calculated =
      SubstringOfWidth(long_string, attributes, leading_width, NO);
  EXPECT_NSEQ(leading, leading_calculated);

  NSString* trailing_calculated =
      SubstringOfWidth(long_string, attributes, trailing_width, YES);
  EXPECT_NSEQ(trailing, trailing_calculated);
}
}  // namespace
