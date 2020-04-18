// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import <EarlGrey/EarlGrey.h>

#import "ios/chrome/test/app/chrome_test_util.h"
#import "ios/chrome/test/app/history_test_util.h"
#import "ios/chrome/test/app/tab_test_util.h"
#import "ios/chrome/test/earl_grey/chrome_earl_grey.h"
#import "ios/chrome/test/earl_grey/chrome_matchers.h"
#import "ios/chrome/test/earl_grey/chrome_test_case.h"
#import "ios/testing/wait_util.h"
#import "ios/web/public/test/http_server/html_response_provider.h"
#import "ios/web/public/test/http_server/html_response_provider_impl.h"
#import "ios/web/public/test/http_server/http_server.h"
#include "ios/web/public/test/http_server/http_server_util.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

using web::test::HttpServer;

// Test case for NTP tiles.
@interface NTPTilesTest : ChromeTestCase
@end

@implementation NTPTilesTest

- (void)tearDown {
  GREYAssertTrue(chrome_test_util::ClearBrowsingHistory(),
                 @"Clearing Browsing History timed out");
  [[GREYUIThreadExecutor sharedInstance] drainUntilIdle];
  [super tearDown];
}

// Tests that loading a URL ends up creating an NTP tile.
- (void)testTopSitesTileAfterLoadURL {
  std::map<GURL, std::string> responses;
  GURL URL = web::test::HttpServer::MakeUrl("http://simple_tile.html");
  responses[URL] =
      "<head><title>title1</title></head>"
      "<body>You are here.</body>";
  web::test::SetUpSimpleHttpServer(responses);

  // Clear history and verify that the tile does not exist.
  GREYAssertTrue(chrome_test_util::ClearBrowsingHistory(),
                 @"Clearing Browsing History timed out");
  [[GREYUIThreadExecutor sharedInstance] drainUntilIdle];
  chrome_test_util::OpenNewTab();

  [[EarlGrey selectElementWithMatcher:
                 chrome_test_util::StaticTextWithAccessibilityLabel(@"title1")]
      assertWithMatcher:grey_nil()];

  [ChromeEarlGrey loadURL:URL];

  // After loading URL, need to do another action before opening a new tab
  // with the icon present.
  [ChromeEarlGrey goBack];

  chrome_test_util::OpenNewTab();
  // New tab page is not loaded within a single tick so wait before assert.
  [ChromeEarlGrey waitForPageToFinishLoading];

  [[EarlGrey selectElementWithMatcher:
                 chrome_test_util::StaticTextWithAccessibilityLabel(@"title1")]
      assertWithMatcher:grey_notNil()];
}

// Tests that only one NTP tile is displayed for a TopSite that involves a
// redirect.
- (void)testTopSitesTileAfterRedirect {
  std::map<GURL, HtmlResponseProviderImpl::Response> responses;
  const GURL firstRedirectURL = HttpServer::MakeUrl("http://firstRedirect/");
  const GURL destinationURL = HttpServer::MakeUrl("http://destination/");
  responses[firstRedirectURL] = HtmlResponseProviderImpl::GetRedirectResponse(
      destinationURL, net::HTTP_MOVED_PERMANENTLY);

  // Add titles to both responses, which is what will show up on the NTP.
  responses[firstRedirectURL].body =
      "<head><title>title1</title></head>"
      "<body>Should redirect away.</body>";

  const char kFinalPageContent[] =
      "<head><title>title2</title></head>"
      "<body>redirect complete</body>";
  responses[destinationURL] =
      HtmlResponseProviderImpl::GetSimpleResponse(kFinalPageContent);
  std::unique_ptr<web::DataResponseProvider> provider(
      new HtmlResponseProvider(responses));
  web::test::SetUpHttpServer(std::move(provider));

  // Clear history and verify that the tile does not exist.
  GREYAssertTrue(chrome_test_util::ClearBrowsingHistory(),
                 @"Clearing Browsing History timed out");
  [[GREYUIThreadExecutor sharedInstance] drainUntilIdle];
  chrome_test_util::OpenNewTab();
  [[EarlGrey selectElementWithMatcher:
                 chrome_test_util::StaticTextWithAccessibilityLabel(@"title2")]
      assertWithMatcher:grey_nil()];

  // Load first URL and expect redirect to destination URL.
  [ChromeEarlGrey loadURL:firstRedirectURL];
  [ChromeEarlGrey waitForWebViewContainingText:"redirect complete"];

  // After loading URL, need to do another action before opening a new tab
  // with the icon present.
  [ChromeEarlGrey goBack];
  chrome_test_util::OpenNewTab();

  // Which of the two tiles that is displayed is an implementation detail, and
  // this test helps document it. The purpose of the test is to verify that only
  // one tile is displayed.
  [[EarlGrey selectElementWithMatcher:
                 chrome_test_util::StaticTextWithAccessibilityLabel(@"title2")]
      assertWithMatcher:grey_notNil()];
  [[EarlGrey selectElementWithMatcher:
                 chrome_test_util::StaticTextWithAccessibilityLabel(@"title1")]
      assertWithMatcher:grey_nil()];
}

@end
