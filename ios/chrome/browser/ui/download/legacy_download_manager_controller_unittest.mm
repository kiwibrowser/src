// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/chrome/browser/ui/download/legacy_download_manager_controller.h"

#import <UIKit/UIKit.h>

#include <memory>

#import "ios/chrome/browser/store_kit/store_kit_launcher.h"
#import "ios/chrome/browser/store_kit/store_kit_tab_helper.h"
#import "ios/chrome/browser/web/chrome_web_test.h"
#include "ios/web/public/test/test_web_thread.h"
#include "net/base/net_errors.h"
#include "net/http/http_response_headers.h"
#include "net/url_request/test_url_fetcher_factory.h"
#include "net/url_request/url_fetcher_delegate.h"
#include "net/url_request/url_request_test_util.h"
#include "testing/gtest_mac.h"
#include "testing/platform_test.h"
#import "third_party/ocmock/OCMock/OCMock.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

using net::HttpResponseHeaders;
using net::URLRequestStatus;

@interface LegacyDownloadManagerController (ExposedForTesting)
- (UIView*)documentContainer;
- (UIView*)progressBar;
- (UIImageView*)documentIcon;
- (UIImageView*)foldIcon;
- (UILabel*)timeLeftLabel;
- (UILabel*)fileTypeLabel;
- (UILabel*)fileNameLabel;
- (UILabel*)errorOrSizeLabel;
- (UIImageView*)errorIcon;
- (UIView*)actionBar;
- (UIButton*)downloadButton;
- (UIButton*)cancelButton;
- (UIButton*)openInButton;
- (UIButton*)googleDriveButton;
- (long long)totalFileSize;
@end

namespace {

const GURL kTestURL = GURL("http://www.example.com/test_download_file.txt");

class LegacyDownloadManagerControllerTest : public ChromeWebTest {
 protected:
  void SetUp() override {
    ChromeWebTest::SetUp();
    _fetcher_factory.reset(new net::TestURLFetcherFactory());
    StoreKitTabHelper::CreateForWebState(web_state());
    StoreKitTabHelper* helper = StoreKitTabHelper::FromWebState(web_state());
    id mock_launcher =
        [OCMockObject niceMockForProtocol:@protocol(StoreKitLauncher)];
    helper->SetLauncher(mock_launcher);
    _controller =
        [[LegacyDownloadManagerController alloc] initWithWebState:web_state()
                                                downloadURL:kTestURL
                                         baseViewController:nil];
  }
  std::unique_ptr<net::TestURLFetcherFactory> _fetcher_factory;
  __strong LegacyDownloadManagerController* _controller;
};

TEST_F(LegacyDownloadManagerControllerTest, TestXibViewConnections) {
  EXPECT_TRUE([_controller documentContainer]);
  EXPECT_TRUE([_controller progressBar]);
  EXPECT_TRUE([_controller documentIcon]);
  EXPECT_TRUE([_controller foldIcon]);
  EXPECT_TRUE([_controller timeLeftLabel]);
  EXPECT_TRUE([_controller fileTypeLabel]);
  EXPECT_TRUE([_controller fileNameLabel]);
  EXPECT_TRUE([_controller errorOrSizeLabel]);
  EXPECT_TRUE([_controller errorIcon]);
  EXPECT_TRUE([_controller actionBar]);
  EXPECT_TRUE([_controller downloadButton]);
  EXPECT_TRUE([_controller cancelButton]);
  EXPECT_TRUE([_controller openInButton]);
  EXPECT_TRUE([_controller googleDriveButton]);
}

// TODO(crbug.com/804250): this test is flaky.
TEST_F(LegacyDownloadManagerControllerTest, TestStart) {
  [_controller start];
  EXPECT_TRUE(
      [[UIApplication sharedApplication] isNetworkActivityIndicatorVisible]);
  net::TestURLFetcher* fetcher = _fetcher_factory->GetFetcherByID(0);
  EXPECT_TRUE(fetcher != NULL);
  EXPECT_EQ(kTestURL, fetcher->GetOriginalURL());
}

TEST_F(LegacyDownloadManagerControllerTest, TestOnHeadFetchCompleteSuccess) {
  [_controller start];
  net::TestURLFetcher* fetcher = _fetcher_factory->GetFetcherByID(0);

  URLRequestStatus success_status(URLRequestStatus::SUCCESS, 0);
  fetcher->set_status(success_status);
  fetcher->set_response_code(200);
  scoped_refptr<HttpResponseHeaders> headers;
  headers = new HttpResponseHeaders("HTTP/1.x 200 OK\0");
  headers->AddHeader("Content-Length: 1000000000");  // ~1GB
  fetcher->set_response_headers(headers);

  fetcher->delegate()->OnURLFetchComplete(fetcher);
  EXPECT_FALSE(
      [[UIApplication sharedApplication] isNetworkActivityIndicatorVisible]);

  EXPECT_EQ(1000000000, [_controller totalFileSize]);
  NSString* fileSizeText = [NSByteCountFormatter
      stringFromByteCount:1000000000
               countStyle:NSByteCountFormatterCountStyleFile];
  EXPECT_NSEQ(fileSizeText, [_controller errorOrSizeLabel].text);
  EXPECT_FALSE([_controller downloadButton].hidden);
}

TEST_F(LegacyDownloadManagerControllerTest, TestOnHeadFetchCompleteFailure) {
  [_controller start];
  net::TestURLFetcher* fetcher = _fetcher_factory->GetFetcherByID(0);

  URLRequestStatus failure_status(URLRequestStatus::FAILED,
                                  net::ERR_DNS_TIMED_OUT);
  fetcher->set_status(failure_status);

  fetcher->delegate()->OnURLFetchComplete(fetcher);
  EXPECT_FALSE(
      [[UIApplication sharedApplication] isNetworkActivityIndicatorVisible]);

  EXPECT_TRUE([_controller fileTypeLabel].hidden);
  EXPECT_FALSE([_controller downloadButton].hidden);
  EXPECT_FALSE([_controller errorIcon].hidden);
  EXPECT_FALSE([_controller errorOrSizeLabel].hidden);
}

}  // namespace
