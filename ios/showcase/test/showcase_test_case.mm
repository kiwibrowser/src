// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/showcase/test/showcase_test_case.h"

#import <EarlGrey/EarlGrey.h>

#import "base/logging.h"
#import "base/mac/foundation_util.h"
#import "ios/showcase/core/app_delegate.h"
#include "testing/coverage_util_ios.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

@implementation ShowcaseTestCase

+ (void)setUp {
  coverage_util::ConfigureCoverageReportPath();
}

// Overrides testInvocations so the set of tests run can be modified, as
// necessary.
+ (NSArray*)testInvocations {
  NSError* error = nil;
  [[EarlGrey selectElementWithMatcher:grey_systemAlertViewShown()]
      assertWithMatcher:grey_nil()
                  error:&error];
  if (error != nil) {
    LOG(ERROR) << "System alert view is present, so skipping all tests.";
    return @[];
  }
  return [super testInvocations];
}

- (void)setUp {
  AppDelegate* delegate = base::mac::ObjCCastStrict<AppDelegate>(
      [UIApplication sharedApplication].delegate);
  [delegate setupUI];
}

@end
