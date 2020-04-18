// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/chrome/browser/ui/app_launcher/app_launcher_util.h"

#include "testing/gtest/include/gtest/gtest.h"
#include "testing/gtest_mac.h"
#include "testing/platform_test.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

using ExternalAppLauncherUtilTest = PlatformTest;

// A URL with a malformed scheme should return the original URL string.
TEST_F(ExternalAppLauncherUtilTest, TestMalformedScheme) {
  EXPECT_NSEQ(@"malformed:////", GetFormattedAbsoluteUrlWithSchemeRemoved(
                                     [NSURL URLWithString:@"malformed:////"]));
}

// A URL with a properly formed scheme should return the string following the
// scheme.
TEST_F(ExternalAppLauncherUtilTest, TestProperlyFormedExternalUrl) {
  EXPECT_NSEQ(@"+1234", GetFormattedAbsoluteUrlWithSchemeRemoved(
                            [NSURL URLWithString:@"facetime://+1234"]));
  EXPECT_NSEQ(@"+abcd", GetFormattedAbsoluteUrlWithSchemeRemoved(
                            [NSURL URLWithString:@"facetime-audio://+abcd"]));
  EXPECT_NSEQ(@"+1-650-555-1212",
              GetFormattedAbsoluteUrlWithSchemeRemoved(
                  [NSURL URLWithString:@"tel://+1-650-555-1212"]));
}

// Percent encoding should be removed from a properly formed URL string.
TEST_F(ExternalAppLauncherUtilTest, TestRemovingPercentEncoding) {
  EXPECT_NSEQ(@"+1 650 555 1212",
              GetFormattedAbsoluteUrlWithSchemeRemoved(
                  [NSURL URLWithString:@"tel://+1%20650%20555%201212"]));
}
