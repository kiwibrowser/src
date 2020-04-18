// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "chrome/installer/mac/app/SystemInfo.h"

#include "testing/gtest/include/gtest/gtest.h"

namespace {

TEST(SystemInfoTest, GetArchReturnsExpectedString) {
  NSString* arch = [SystemInfo getArch];

  EXPECT_TRUE([arch isEqualToString:@"i486"] ||
              [arch isEqualToString:@"x86_64h"]);
}

TEST(SystemInfoTest, GetOSVersionMatchesRegexFormat) {
  NSString* os_version = [SystemInfo getOSVersion];

  NSRegularExpression* regex = [NSRegularExpression
      regularExpressionWithPattern:@"^10\\.(0|[1-9][0-9]*)\\.(0|[1-9][0-9]*)$"
                           options:0
                             error:nil];
  NSUInteger matches =
      [regex numberOfMatchesInString:os_version
                             options:0
                               range:NSMakeRange(0, os_version.length)];
  EXPECT_EQ(1u, matches);
}

}  // namespace
