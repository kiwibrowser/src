// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ios/chrome/browser/ssl/ios_ssl_error_handler.h"

#include "base/mac/bind_objc_block.h"
#include "base/test/scoped_feature_list.h"
#include "ios/chrome/browser/browser_state/test_chrome_browser_state.h"
#import "ios/chrome/browser/ssl/captive_portal_detector_tab_helper.h"
#include "ios/chrome/browser/ssl/captive_portal_features.h"
#include "ios/web/public/interstitials/web_interstitial.h"
#import "ios/web/public/test/web_test_with_web_state.h"
#import "ios/web/public/web_state/web_state.h"
#include "net/ssl/ssl_info.h"
#include "net/test/cert_test_util.h"
#include "net/test/test_data_directory.h"
#include "services/network/test/test_url_loader_factory.h"
#import "third_party/ocmock/OCMock/OCMock.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

namespace {
const char kTestCertFileName[] = "ok_cert.pem";
const char kTestHostName[] = "https://chromium.test/";
}  // namespace

// Test fixture for IOSSSLErrorHandler class.
class IOSSSLErrorHandlerTest : public web::WebTestWithWebState {
 protected:
  IOSSSLErrorHandlerTest()
      : browser_state_(builder_.Build()),
        cert_(net::ImportCertFromFile(net::GetTestCertsDirectory(),
                                      kTestCertFileName)) {}

  // web::WebTestWithWebState overrides:
  void SetUp() override {
    web::WebTestWithWebState::SetUp();
    id captive_portal_detector_tab_helper_delegate = [OCMockObject
        mockForProtocol:@protocol(CaptivePortalDetectorTabHelperDelegate)];
    // Use a testing URLLoaderFactory so that these tests don't attempt to make
    // network requests.
    CaptivePortalDetectorTabHelper::CreateForWebState(
        web_state(), captive_portal_detector_tab_helper_delegate,
        &test_loader_factory_);
    ASSERT_TRUE(cert_);
    ASSERT_FALSE(web_state()->IsShowingWebInterstitial());

    // Transient item can only be added for pending non-app-specific loads.
    AddPendingItem(GURL(kTestHostName),
                   ui::PageTransition::PAGE_TRANSITION_TYPED);
  }
  web::BrowserState* GetBrowserState() override { return browser_state_.get(); }

  // Returns certificate for testing.
  scoped_refptr<net::X509Certificate> cert() { return cert_; }

 private:
  network::TestURLLoaderFactory test_loader_factory_;
  TestChromeBrowserState::Builder builder_;
  std::unique_ptr<TestChromeBrowserState> browser_state_;
  scoped_refptr<net::X509Certificate> cert_;
};

// Tests non-overridable error handling.
TEST_F(IOSSSLErrorHandlerTest, NonOverridable) {
  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitAndDisableFeature(kCaptivePortalFeature);

  net::SSLInfo ssl_info;
  ssl_info.cert = cert();
  GURL url(kTestHostName);
  __block bool do_not_proceed_callback_called = false;
  IOSSSLErrorHandler::HandleSSLError(
      web_state(), net::ERR_CERT_AUTHORITY_INVALID, ssl_info, url, false,
      base::BindBlockArc(^(bool proceed) {
        EXPECT_FALSE(proceed);
        do_not_proceed_callback_called = true;
      }));

  // Make sure that interstitial is displayed.
  EXPECT_TRUE(web_state()->IsShowingWebInterstitial());
  web::WebInterstitial* interstitial = web_state()->GetWebInterstitial();
  EXPECT_TRUE(interstitial);

  // Make sure callback is called on dismissal.
  interstitial->DontProceed();
  EXPECT_TRUE(do_not_proceed_callback_called);
}

// Tests proceed with overridable error.
// Flaky: http://crbug.com/660343.
TEST_F(IOSSSLErrorHandlerTest, DISABLED_OverridableProceed) {
  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitAndDisableFeature(kCaptivePortalFeature);

  net::SSLInfo ssl_info;
  ssl_info.cert = cert();
  GURL url(kTestHostName);
  __block bool proceed_callback_called = false;
  IOSSSLErrorHandler::HandleSSLError(
      web_state(), net::ERR_CERT_AUTHORITY_INVALID, ssl_info, url, true,
      base::BindBlockArc(^(bool proceed) {
        EXPECT_TRUE(proceed);
        proceed_callback_called = true;
      }));

  // Make sure that interstitial is displayed.
  EXPECT_TRUE(web_state()->IsShowingWebInterstitial());
  web::WebInterstitial* interstitial = web_state()->GetWebInterstitial();
  EXPECT_TRUE(interstitial);

  // Make sure callback is called on dismissal.
  interstitial->Proceed();
  EXPECT_TRUE(proceed_callback_called);
}

// Tests do not proceed with overridable error.
TEST_F(IOSSSLErrorHandlerTest, OverridableDontProceed) {
  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitAndDisableFeature(kCaptivePortalFeature);

  net::SSLInfo ssl_info;
  ssl_info.cert = cert();
  GURL url(kTestHostName);
  __block bool do_not_proceed_callback_called = false;
  IOSSSLErrorHandler::HandleSSLError(
      web_state(), net::ERR_CERT_AUTHORITY_INVALID, ssl_info, url, true,
      base::BindBlockArc(^(bool proceed) {
        EXPECT_FALSE(proceed);
        do_not_proceed_callback_called = true;
      }));

  // Make sure that interstitial is displayed.
  EXPECT_TRUE(web_state()->IsShowingWebInterstitial());
  web::WebInterstitial* interstitial = web_state()->GetWebInterstitial();
  EXPECT_TRUE(interstitial);

  // Make sure callback is called on dismissal.
  interstitial->DontProceed();
  EXPECT_TRUE(do_not_proceed_callback_called);
}
