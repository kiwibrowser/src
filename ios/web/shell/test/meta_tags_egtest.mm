// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <map>

#include "base/strings/stringprintf.h"
#import "base/test/ios/wait_util.h"
#import "ios/web/public/test/http_server/http_server.h"
#include "ios/web/public/test/http_server/http_server_util.h"
#import "ios/web/shell/test/earl_grey/shell_earl_grey.h"
#import "ios/web/shell/test/earl_grey/shell_matchers.h"
#import "ios/web/shell/test/earl_grey/web_shell_test_case.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

namespace {

// A simple test page with generic content.
const char kDestinationPage[] = "You've arrived!";

// Template for a test page with META refresh tag. Required template arguments
// are: refresh time in seconds (integer) and destination URL for redirect
// (string).
const char kRefreshMetaPageTemplate[] =
    "<!DOCTYPE html>"
    "<html>"
    "  <head><meta HTTP-EQUIV='REFRESH' content='%d;url=%s'></head>"
    "  <body></body>"
    "</html>";

}  // namespace

using web::test::HttpServer;
using web::AddressFieldText;

// META tag test cases for the web shell.
@interface MetaTagsTestCase : WebShellTestCase
@end

@implementation MetaTagsTestCase

// Tests loading of a page with a META tag having a refresh value of 0 seconds.
- (void)testMetaRefresh0Seconds {
  [self runMetaRefreshTestWithRefreshInterval:0];
}

// Tests loading of a page with a META tag having a refresh value of 3 seconds.
- (void)testMetaRefresh3Seconds {
  [self runMetaRefreshTestWithRefreshInterval:3];
}

// Loads a page with a META tag having a refresh value of |refreshInterval|
// seconds and verifies that redirect happens.
- (void)runMetaRefreshTestWithRefreshInterval:(int)refreshIntervalInSeconds {
  // Create map of canned responses and set up the test HTML server.
  std::map<GURL, std::string> responses;
  const GURL originURL = HttpServer::MakeUrl("http://origin");
  const GURL destinationURL = HttpServer::MakeUrl("http://destination");
  responses[originURL] =
      base::StringPrintf(kRefreshMetaPageTemplate, refreshIntervalInSeconds,
                         destinationURL.spec().c_str());
  responses[destinationURL] = kDestinationPage;
  web::test::SetUpSimpleHttpServer(responses);

  [ShellEarlGrey loadURL:originURL];

  // Wait for redirect.
  base::test::ios::SpinRunLoopWithMinDelay(
      base::TimeDelta::FromSecondsD(refreshIntervalInSeconds));

  // Verify that redirect happened.
  [[EarlGrey selectElementWithMatcher:AddressFieldText(destinationURL.spec())]
      assertWithMatcher:grey_notNil()];
  [ShellEarlGrey waitForWebViewContainingText:kDestinationPage];
}

@end
