// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import <UIKit/UIKit.h>

#include "ios/chrome/browser/ui/first_run/first_run_util.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "testing/platform_test.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

namespace {

using UICommonTest = PlatformTest;

TEST_F(UICommonTest, TestFixOrphanWord) {
  NSString* englishString =
      @"Lorem ipsum dolor sit amet, consectetur adipiscing elit. Donec tempus"
       " dignissim congue. Morbi pulvinar vitae purus at mollis. Sed laoreet "
       "euismod neque, eget laoreet nisi porttitor sed.";
  NSString* englishStringWithOrphan =
      @"Lorem ipsum dolor sit amet, consectetur adipiscing elit. Donec tempus"
       " dignissim congue. Morbi pulvinar vitae purus at mollis. Sed laoreet "
       "euismod neque, eget laoreet nisi.";
  // TODO(crbug.com/675342): clang_format does a poor job here. Remove when
  // fixed in clang_format.
  // clang-format off
  NSString* chineseString =
      @"那只敏捷的棕色狐狸跃过那只懒狗。那只敏捷的棕色狐狸跃过那只懒狗。"
       "那只敏捷的棕色狐狸跃过那只懒狗。那只敏捷的棕色狐狸跃过那只懒狗。"
       "那只敏捷的棕色狐狸跃过那只懒狗。那只敏捷的棕色狐狸跃过那只懒狗。";
  NSString* chineseStringWithOrphan =
      @"那只敏捷的棕色狐狸跃过那只懒狗。那只敏捷的棕色狐狸跃过那只懒狗。"
       "那只敏捷的棕色狐狸跃过那只懒狗。快速狐狸";
  // clang-format on

  UILabel* label = [[UILabel alloc] initWithFrame:CGRectMake(0, 0, 300, 500)];
  [label setText:englishString];
  FixOrphanWord(label);
  NSRange range = [[label text] rangeOfString:@"\n"];
  // Check that the label's text does not contain a newline.
  EXPECT_EQ(NSNotFound, static_cast<NSInteger>(range.location));

  [label setText:englishStringWithOrphan];
  FixOrphanWord(label);
  range = [[label text] rangeOfString:@"\n"];
  // Check that the label's text contains a newline.
  EXPECT_NE(NSNotFound, static_cast<NSInteger>(range.location));

  // Check the words after the newline.
  NSString* wordsAfterNewline =
      [[label text] substringFromIndex:(range.location + range.length)];
  EXPECT_TRUE([@"laoreet nisi." isEqualToString:wordsAfterNewline]);

  [label setText:chineseString];
  FixOrphanWord(label);
  range = [[label text] rangeOfString:@"\n"];
  // Check that the label's text does not contain a newline.
  EXPECT_EQ(NSNotFound, static_cast<NSInteger>(range.location));

  [label setText:chineseStringWithOrphan];
  FixOrphanWord(label);
  range = [[label text] rangeOfString:@"\n"];
  // Check that the label's text contains a newline.
  ASSERT_NE(NSNotFound, static_cast<NSInteger>(range.location));

  // Check the words after the newline.
  wordsAfterNewline =
      [[label text] substringFromIndex:(range.location + range.length)];
  EXPECT_TRUE([@"快速狐狸" isEqualToString:wordsAfterNewline]);
}
}
