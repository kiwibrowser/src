// Copyright 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/chrome/browser/ui/chrome_web_view_factory.h"

#include <memory>

#include "base/run_loop.h"
#include "ios/chrome/browser/browser_state/test_chrome_browser_state_isolated_context.h"
#import "ios/chrome/browser/web/chrome_web_client.h"
#include "ios/web/net/request_group_util.h"
#include "ios/web/net/request_tracker_impl.h"
#include "ios/web/public/test/scoped_testing_web_client.h"
#include "ios/web/public/test/test_web_thread_bundle.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "testing/gtest_mac.h"
#include "testing/platform_test.h"
#import "third_party/ocmock/OCMock/OCMock.h"
#import "third_party/ocmock/gtest_support.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

using web::RequestTrackerImpl;

namespace {

class ChromeWebViewFactoryTest : public PlatformTest {
 public:
  ChromeWebViewFactoryTest()
      : web_client_(std::make_unique<ChromeWebClient>()) {}

 protected:
  web::TestWebThreadBundle thread_bundle_;
  web::ScopedTestingWebClient web_client_;
  TestChromeBrowserStateWithIsolatedContext chrome_browser_state_;

 private:
  DISALLOW_COPY_AND_ASSIGN(ChromeWebViewFactoryTest);
};

TEST_F(ChromeWebViewFactoryTest, TestTrackerForExternal) {
  [ChromeWebViewFactory setBrowserStateToUseForExternal:&chrome_browser_state_];
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunused-variable"
  UIWebView* webView = [ChromeWebViewFactory
      newExternalWebView:chrome_browser_state_.SharingService()];
#pragma clang diagnostic pop
  // Check that the tracker is registered
  RequestTrackerImpl* tracker = RequestTrackerImpl::GetTrackerForRequestGroupID(
      ChromeWebView::kExternalRequestGroupID);
  EXPECT_TRUE(tracker) << "Tracker not registered.";
  // External should use the request context for the sharing service, and not
  // the main one.
  net::URLRequestContextGetter* expected_getter = [ChromeWebViewFactory
      requestContextForExternalService:chrome_browser_state_.SharingService()];
  EXPECT_EQ(expected_getter->GetURLRequestContext(),
            tracker->GetRequestContext());
  EXPECT_FALSE(chrome_browser_state_.MainContextCalled());
  // Unregister the tracker.
  [ChromeWebViewFactory externalSessionFinished];
  base::RunLoop().RunUntilIdle();
  EXPECT_FALSE(RequestTrackerImpl::GetTrackerForRequestGroupID(
      ChromeWebView::kExternalRequestGroupID));
  [ChromeWebViewFactory setBrowserStateToUseForExternal:nullptr];
}

}  // namespace
