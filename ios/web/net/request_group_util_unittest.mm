// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/web/net/request_group_util.h"

#import <Foundation/Foundation.h>
#include <stddef.h>

#include "testing/gtest/include/gtest/gtest.h"
#include "testing/platform_test.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

using RequestGroupUtilTest = PlatformTest;

// Checks that all newly generated groupID are unique and that there are no
// duplicates.
TEST_F(RequestGroupUtilTest, RequestGroupID) {
  NSMutableSet* set = [[NSMutableSet alloc] init];
  const size_t kGenerated = 2000;
  for (size_t i = 0; i < kGenerated; ++i)
    [set addObject:web::GenerateNewRequestGroupID()];
  EXPECT_EQ(kGenerated, [set count]);
}

// Tests that the ExtractRequestGroupIDFromUserAgent function behaves as
// intended.
TEST_F(RequestGroupUtilTest, ExtractRequestGroupIDFromUserAgent) {
  EXPECT_FALSE(web::ExtractRequestGroupIDFromUserAgent(nil));
  EXPECT_FALSE(web::ExtractRequestGroupIDFromUserAgent(
      @"Lynx/2.8.1pre.9 libwww-FM/2.14"));
  EXPECT_FALSE(web::ExtractRequestGroupIDFromUserAgent(@"    "));
  EXPECT_TRUE([web::ExtractRequestGroupIDFromUserAgent(@"Mozilla/3.04 (WinNT)")
      isEqualToString:@"WinNT"]);
}
