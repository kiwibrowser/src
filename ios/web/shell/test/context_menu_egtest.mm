// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import <EarlGrey/EarlGrey.h>
#import <UIKit/UIKit.h>
#import <WebKit/WebKit.h>
#import <XCTest/XCTest.h>

#import "base/ios/block_types.h"
#import "ios/testing/earl_grey/matchers.h"
#import "ios/web/public/test/http_server/http_server.h"
#include "ios/web/public/test/http_server/http_server_util.h"
#import "ios/web/public/test/web_view_interaction_test_util.h"
#import "ios/web/shell/test/app/web_shell_test_util.h"
#include "ios/web/shell/test/app/web_view_interaction_test_util.h"
#import "ios/web/shell/test/earl_grey/shell_actions.h"
#import "ios/web/shell/test/earl_grey/shell_earl_grey.h"
#import "ios/web/shell/test/earl_grey/shell_matchers.h"
#import "ios/web/shell/test/earl_grey/web_shell_test_case.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

using testing::ButtonWithAccessibilityLabel;
using testing::ElementToDismissAlert;

// Context menu test cases for the web shell.
@interface ContextMenuTestCase : WebShellTestCase
@end

@implementation ContextMenuTestCase

// Tests context menu appears on a regular link.
- (void)testContextMenu {
  // Create map of canned responses and set up the test HTML server.
  std::map<GURL, std::string> responses;
  GURL initialURL = web::test::HttpServer::MakeUrl("http://contextMenuOpen");
  GURL destinationURL = web::test::HttpServer::MakeUrl("http://destination");
  // The initial page contains a link to the destination URL.
  std::string linkID = "link";
  std::string linkText = "link for context menu";
  responses[initialURL] =
      "<body>"
      "<a href='" +
      destinationURL.spec() + "' id='" + linkID + "'>" + linkText +
      "</a>"
      "</span></body>";

  web::test::SetUpSimpleHttpServer(responses);
  [ShellEarlGrey loadURL:initialURL];
  [ShellEarlGrey waitForWebViewContainingText:linkText];

  [[EarlGrey selectElementWithMatcher:web::WebView()]
      performAction:web::LongPressElementForContextMenu(linkID)];

  id<GREYMatcher> copyItem = ButtonWithAccessibilityLabel(@"Copy Link");

  // Context menu should have a "copy link" item.
  [[EarlGrey selectElementWithMatcher:copyItem]
      assertWithMatcher:grey_notNil()];

  // Dismiss the context menu.
  [[EarlGrey selectElementWithMatcher:ElementToDismissAlert(@"Cancel")]
      performAction:grey_tap()];

  // Context menu should go away after the tap.
  [[EarlGrey selectElementWithMatcher:copyItem] assertWithMatcher:grey_nil()];
}

// Tests context menu on element that has WebkitTouchCallout set to none from an
// ancestor and overridden.
- (void)testContextMenuWebkitTouchCalloutOverride {
  // Create map of canned responses and set up the test HTML server.
  std::map<GURL, std::string> responses;
  GURL initialURL =
      web::test::HttpServer::MakeUrl("http://contextMenuDisabledByWebkit");
  GURL destinationURL = web::test::HttpServer::MakeUrl("http://destination");
  // The initial page contains a link to the destination URL that has an
  // ancestor that disables the context menu via -webkit-touch-callout.
  std::string linkID = "link";
  std::string linkText = "override no-callout link";
  responses[initialURL] =
      "<body style='-webkit-touch-callout: none'>"
      "<a href='" +
      destinationURL.spec() + "' style='-webkit-touch-callout: default' id='" +
      linkID + "'>" + linkText +
      "</a>"
      "</body>";

  web::test::SetUpSimpleHttpServer(responses);
  [ShellEarlGrey loadURL:initialURL];
  [ShellEarlGrey waitForWebViewContainingText:linkText];

  [[EarlGrey selectElementWithMatcher:web::WebView()]
      performAction:web::LongPressElementForContextMenu(linkID)];

  id<GREYMatcher> copyItem = ButtonWithAccessibilityLabel(@"Copy Link");

  // Context menu should have a "copy link" item.
  [[EarlGrey selectElementWithMatcher:copyItem]
      assertWithMatcher:grey_notNil()];

  // Dismiss the context menu.
  [[EarlGrey selectElementWithMatcher:ElementToDismissAlert(@"Cancel")]
      performAction:grey_tap()];

  // Context menu should go away after the tap.
  [[EarlGrey selectElementWithMatcher:copyItem] assertWithMatcher:grey_nil()];
}

@end
