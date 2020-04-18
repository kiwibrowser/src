// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import <EarlGrey/EarlGrey.h>

#include "base/test/scoped_feature_list.h"
#include "ios/chrome/grit/ios_strings.h"
#import "ios/chrome/test/earl_grey/chrome_earl_grey.h"
#import "ios/chrome/test/earl_grey/chrome_matchers.h"
#import "ios/chrome/test/earl_grey/chrome_test_case.h"
#include "ios/web/public/features.h"
#include "net/test/embedded_test_server/http_request.h"
#include "net/test/embedded_test_server/http_response.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

using chrome_test_util::ButtonWithAccessibilityLabelId;

namespace {

// Matcher for "Download" button on Download Manager UI.
id<GREYMatcher> DownloadButton() {
  return ButtonWithAccessibilityLabelId(IDS_IOS_DOWNLOAD_MANAGER_DOWNLOAD);
}

// Matcher for "Open In..." button on Download Manager UI.
id<GREYMatcher> OpenInButton() {
  return ButtonWithAccessibilityLabelId(IDS_IOS_OPEN_IN);
}

// Provides downloads landing page and download response.
std::unique_ptr<net::test_server::HttpResponse> GetResponse(
    const net::test_server::HttpRequest& request) {
  auto result = std::make_unique<net::test_server::BasicHttpResponse>();
  result->set_code(net::HTTP_OK);

  if (request.GetURL().path() == "/") {
    // Landing page with download links.
    result->set_content("<a id='download' href='/download'>Download</a>");
  } else if (request.GetURL().path() == "/download") {
    // Sucessfully provide download response.
    result->AddCustomHeader("Content-Type", "application/vnd.test");
  }

  return result;
}

}  // namespace

// Tests critical user journeys for Download Manager.
@interface DownloadManagerEGTest : ChromeTestCase {
  base::test::ScopedFeatureList _featureList;
}
@end

@implementation DownloadManagerEGTest

- (void)setUp {
  [super setUp];

  _featureList.InitAndEnableFeature(web::features::kNewFileDownload);

  self.testServer->RegisterRequestHandler(base::BindRepeating(&GetResponse));
  GREYAssertTrue(self.testServer->Start(), @"Test server failed to start.");
}

// Tests sucessfull download up to the point where "Open in..." button is
// presented. EarlGreay does not allow testing "Open in..." dialog, because it
// is run in a separate process.
- (void)testSucessfullDownload {
  [ChromeEarlGrey loadURL:self.testServer->GetURL("/")];
  [ChromeEarlGrey waitForWebViewContainingText:"Download"];
  [ChromeEarlGrey tapWebViewElementWithID:@"download"];

  [[EarlGrey selectElementWithMatcher:DownloadButton()]
      performAction:grey_tap()];

  [[EarlGrey selectElementWithMatcher:OpenInButton()]
      assertWithMatcher:grey_notNil()];
}

// Tests cancelling download UI.
- (void)testCancellingDownload {
  [ChromeEarlGrey loadURL:self.testServer->GetURL("/")];
  [ChromeEarlGrey waitForWebViewContainingText:"Download"];
  [ChromeEarlGrey tapWebViewElementWithID:@"download"];

  [[EarlGrey selectElementWithMatcher:DownloadButton()]
      assertWithMatcher:grey_notNil()];

  [[EarlGrey selectElementWithMatcher:chrome_test_util::CloseButton()]
      performAction:grey_tap()];

  [[EarlGrey selectElementWithMatcher:DownloadButton()]
      assertWithMatcher:grey_nil()];
}

@end
