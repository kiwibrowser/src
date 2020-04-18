// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/web/shell/test/earl_grey/web_shell_test_case.h"

#import <EarlGrey/EarlGrey.h>

#import "ios/web/public/test/http_server/http_server.h"
#import "ios/web/shell/test/earl_grey/shell_matchers.h"
#include "testing/coverage_util_ios.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

using web::test::HttpServer;

@implementation WebShellTestCase

// Overrides |testInvocations| to skip all tests if a system alert view is
// shown, since this isn't a case a user would encounter (i.e. they would
// dismiss the alert first).
+ (NSArray*)testInvocations {
  // TODO(crbug.com/654085): Simply skipping all tests isn't the best way to
  // handle this, it would be better to have something that is more obvious
  // on the bots that this is wrong, without making it look like test flake.
  NSError* error = nil;
  [[EarlGrey selectElementWithMatcher:grey_systemAlertViewShown()]
      assertWithMatcher:grey_nil()
                  error:&error];
  if (error != nil) {
    NSLog(@"System alert view is present, so skipping all tests!");
    return @[];
  }
  return [super testInvocations];
}

// Set up called once for the class.
+ (void)setUp {
  [super setUp];
  HttpServer::GetSharedInstance().StartOrDie();

  coverage_util::ConfigureCoverageReportPath();
}

// Tear down called once for the class.
+ (void)tearDown {
  HttpServer::GetSharedInstance().Stop();
  [super tearDown];
}

// Tear down called after each test.
- (void)tearDown {
  HttpServer::GetSharedInstance().RemoveAllResponseProviders();
  [super tearDown];
}

@end
