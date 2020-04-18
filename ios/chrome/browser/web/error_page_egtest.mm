// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import <EarlGrey/EarlGrey.h>

#include <string>

#include "base/feature_list.h"
#import "base/mac/bind_objc_block.h"
#include "base/strings/sys_string_conversions.h"
#import "ios/chrome/test/earl_grey/chrome_earl_grey.h"
#import "ios/chrome/test/earl_grey/chrome_test_case.h"
#include "ios/testing/embedded_test_server_handlers.h"
#include "ios/web/public/features.h"
#include "net/test/embedded_test_server/default_handlers.h"
#include "net/test/embedded_test_server/http_request.h"
#include "net/test/embedded_test_server/http_response.h"
#include "net/test/embedded_test_server/request_handler_util.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

namespace {
// Returns ERR_CONNECTION_CLOSED error message.
std::string GetErrorMessage() {
  return net::ErrorToShortString(net::ERR_CONNECTION_CLOSED);
}
NSString* GetNSErrorMessage() {
  return base::SysUTF8ToNSString(GetErrorMessage());
}
}  // namespace

// Tests critical user journeys reloated to page load errors.
@interface ErrorPageTestCase : ChromeTestCase
// YES if test server is replying with valid HTML content (URL query). NO if
// test server closes the socket.
@property(atomic) bool serverRespondsWithContent;
@end

@implementation ErrorPageTestCase
@synthesize serverRespondsWithContent = _serverRespondsWithContent;

- (void)setUp {
  [super setUp];

  RegisterDefaultHandlers(self.testServer);
  self.testServer->RegisterRequestHandler(base::BindRepeating(
      &net::test_server::HandlePrefixedRequest, "/echo-query",
      base::BindRepeating(&testing::HandleEchoQueryOrCloseSocket,
                          base::ConstRef(_serverRespondsWithContent))));
  self.testServer->RegisterRequestHandler(
      base::BindRepeating(&net::test_server::HandlePrefixedRequest, "/iframe",
                          base::BindRepeating(&testing::HandleIFrame)));

  GREYAssertTrue(self.testServer->Start(), @"Test server failed to start.");
}

// Loads the URL which fails to load, then sucessfully reloads the page.
- (void)testReloadErrorPage {
  // No response leads to ERR_CONNECTION_CLOSED error.
  self.serverRespondsWithContent = NO;
  [ChromeEarlGrey loadURL:self.testServer->GetURL("/echo-query?foo")];
  if (base::FeatureList::IsEnabled(web::features::kWebErrorPages)) {
    [ChromeEarlGrey waitForWebViewContainingText:GetErrorMessage()];
  } else {
    [ChromeEarlGrey waitForStaticHTMLViewContainingText:GetNSErrorMessage()];
  }

  // Reload the page, which should load without errors.
  self.serverRespondsWithContent = YES;
  [ChromeEarlGrey reload];
  [ChromeEarlGrey waitForWebViewContainingText:"foo"];
}

// Sucessfully loads the page, stops the server and reloads the page.
- (void)testReloadPageAfterServerIsDown {
  // Sucessfully load the page.
  self.serverRespondsWithContent = YES;
  [ChromeEarlGrey loadURL:self.testServer->GetURL("/echo-query?foo")];
  [ChromeEarlGrey waitForWebViewContainingText:"foo"];

  // Reload the page, no response leads to ERR_CONNECTION_CLOSED error.
  self.serverRespondsWithContent = NO;
  [ChromeEarlGrey reload];
  if (base::FeatureList::IsEnabled(web::features::kWebErrorPages)) {
    [ChromeEarlGrey waitForWebViewContainingText:GetErrorMessage()];
  } else {
    [ChromeEarlGrey waitForStaticHTMLViewContainingText:GetNSErrorMessage()];
  }
}

// Sucessfully loads the page, goes back, stops the server, goes forward and
// reloads.
// TODO(crbug.com/840489): Remove this test.
- (void)testGoForwardAfterServerIsDownAndReload {
  // First page loads sucessfully.
  [ChromeEarlGrey loadURL:self.testServer->GetURL("/echo")];
  [ChromeEarlGrey waitForWebViewContainingText:"Echo"];

  // Second page loads sucessfully.
  self.serverRespondsWithContent = YES;
  [ChromeEarlGrey loadURL:self.testServer->GetURL("/echo-query?foo")];
  [ChromeEarlGrey waitForWebViewContainingText:"foo"];

  // Go back to the first page.
  [ChromeEarlGrey goBack];
  [ChromeEarlGrey waitForWebViewContainingText:"Echo"];

#if TARGET_IPHONE_SIMULATOR
  // Go forward. The response will be retrieved from the page cache and will not
  // present the error page. Page cache may not always exist on device (which is
  // more memory constrained), so this part of the test is simulator-only.
  self.serverRespondsWithContent = NO;
  [ChromeEarlGrey goForward];
  [ChromeEarlGrey waitForWebViewContainingText:"foo"];

  // Reload bypasses the cache.
  [ChromeEarlGrey reload];
  if (base::FeatureList::IsEnabled(web::features::kWebErrorPages)) {
    [ChromeEarlGrey waitForWebViewContainingText:GetErrorMessage()];
  } else {
    [ChromeEarlGrey waitForStaticHTMLViewContainingText:GetNSErrorMessage()];
  }
#endif  // TARGET_IPHONE_SIMULATOR
}

// Sucessfully loads the page, then loads the URL which fails to load, then
// sucessfully goes back to the first page.
// TODO(crbug.com/840489): Remove this test.
- (void)testGoBackFromErrorPage {
  // First page loads sucessfully.
  [ChromeEarlGrey loadURL:self.testServer->GetURL("/echo")];
  [ChromeEarlGrey waitForWebViewContainingText:"Echo"];

  // Second page fails to load.
  [ChromeEarlGrey loadURL:self.testServer->GetURL("/close-socket")];
  if (base::FeatureList::IsEnabled(web::features::kWebErrorPages)) {
    [ChromeEarlGrey waitForWebViewContainingText:GetErrorMessage()];
  } else {
    [ChromeEarlGrey waitForStaticHTMLViewContainingText:GetNSErrorMessage()];
  }

  // Going back should sucessfully load the first page.
  [ChromeEarlGrey goBack];
  [ChromeEarlGrey waitForWebViewContainingText:"Echo"];
}

// Loads the URL which redirects to unresponsive server.
// TODO(crbug.com/840489): Remove this test.
- (void)testRedirectToFailingURL {
  // No response leads to ERR_CONNECTION_CLOSED error.
  self.serverRespondsWithContent = NO;
  [ChromeEarlGrey
      loadURL:self.testServer->GetURL("/server-redirect?echo-query")];
  if (base::FeatureList::IsEnabled(web::features::kWebErrorPages)) {
    [ChromeEarlGrey waitForWebViewContainingText:GetErrorMessage()];
  } else {
    [ChromeEarlGrey waitForStaticHTMLViewContainingText:GetNSErrorMessage()];
  }
}

// Loads the page with iframe, and that iframe fails to load. There should be no
// error page if the main frame has sucessfully loaded.
// TODO(crbug.com/840489): Remove this test.
- (void)testErrorPageInIFrame {
  [ChromeEarlGrey loadURL:self.testServer->GetURL("/iframe?echo-query")];
  [ChromeEarlGrey
      waitForWebViewContainingCSSSelector:"iframe[src*='echo-query']"];
}

@end
