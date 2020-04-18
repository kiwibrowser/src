// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/chrome/browser/ui/external_search/external_search_mediator.h"

#import <UIKit/UIKit.h>

#include "testing/gtest/include/gtest/gtest.h"
#include "testing/platform_test.h"
#include "third_party/ocmock/OCMock/OCMock.h"
#include "third_party/ocmock/gtest_support.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

using ExternalSearchMediatorTest = PlatformTest;

// Checks that the mediator calls the appropriate UIApplication method with the
// expected URL.
TEST_F(ExternalSearchMediatorTest, LaunchExternalSearch) {
  ExternalSearchMediator* mediator = [[ExternalSearchMediator alloc] init];
  id application = OCMClassMock([UIApplication class]);
  mediator.application = application;
  NSURL* expectedURL = [NSURL URLWithString:@"testexternalsearch://"];
  OCMExpect([application openURL:expectedURL
                         options:[OCMArg any]
               completionHandler:[OCMArg any]]);
  [mediator launchExternalSearch];

  EXPECT_OCMOCK_VERIFY(application);
}
