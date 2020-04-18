// Copyright 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/web/web_state/ui/crw_web_controller.h"

#import <WebKit/WebKit.h>

#include <memory>
#include <utility>

#include "base/mac/foundation_util.h"
#include "base/path_service.h"
#include "base/scoped_observer.h"
#include "base/strings/stringprintf.h"
#include "base/strings/utf_string_conversions.h"
#import "base/test/ios/wait_util.h"
#import "ios/testing/ocmock_complex_type_helper.h"
#import "ios/testing/wait_util.h"
#import "ios/web/navigation/crw_session_controller.h"
#import "ios/web/navigation/navigation_item_impl.h"
#import "ios/web/navigation/navigation_manager_impl.h"
#import "ios/web/public/crw_navigation_item_storage.h"
#import "ios/web/public/crw_session_storage.h"
#import "ios/web/public/download/download_controller.h"
#import "ios/web/public/download/download_task.h"
#include "ios/web/public/referrer.h"
#include "ios/web/public/test/fakes/fake_download_controller_delegate.h"
#import "ios/web/public/test/fakes/test_native_content.h"
#import "ios/web/public/test/fakes/test_native_content_provider.h"
#import "ios/web/public/test/fakes/test_web_client.h"
#import "ios/web/public/test/fakes/test_web_state_delegate.h"
#include "ios/web/public/test/fakes/test_web_state_observer.h"
#import "ios/web/public/test/fakes/test_web_view_content_view.h"
#import "ios/web/public/test/web_view_content_test_util.h"
#import "ios/web/public/web_state/ui/crw_content_view.h"
#import "ios/web/public/web_state/ui/crw_native_content.h"
#import "ios/web/public/web_state/ui/crw_native_content_provider.h"
#import "ios/web/public/web_state/ui/crw_web_view_content_view.h"
#include "ios/web/public/web_state/url_verification_constants.h"
#include "ios/web/public/web_state/web_state_observer.h"
#import "ios/web/test/fakes/crw_fake_back_forward_list.h"
#import "ios/web/test/fakes/crw_fake_wk_navigation_response.h"
#include "ios/web/test/test_url_constants.h"
#import "ios/web/test/web_test_with_web_controller.h"
#import "ios/web/test/wk_web_view_crash_utils.h"
#import "ios/web/web_state/ui/crw_web_controller_container_view.h"
#import "ios/web/web_state/ui/web_view_js_utils.h"
#import "ios/web/web_state/web_state_impl.h"
#import "ios/web/web_state/wk_web_view_security_util.h"
#import "net/base/mac/url_conversions.h"
#include "net/cert/x509_util_ios_and_mac.h"
#include "net/ssl/ssl_info.h"
#include "net/test/cert_test_util.h"
#include "net/test/test_data_directory.h"
#include "testing/gtest/include/gtest/gtest.h"
#import "testing/gtest_mac.h"
#include "third_party/ocmock/OCMock/OCMock.h"
#include "third_party/ocmock/gtest_support.h"
#include "third_party/ocmock/ocmock_extensions.h"
#import "ui/base/test/ios/ui_view_test_utils.h"
#include "url/scheme_host_port.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

using testing::WaitUntilConditionOrTimeout;
using testing::kWaitForPageLoadTimeout;

@interface CRWWebController (PrivateAPI)
@property(nonatomic, readwrite) web::PageDisplayState pageDisplayState;
@end

namespace web {
namespace {

// Syntactically invalid URL per rfc3986.
const char kInvalidURL[] = "http://%3";

const char kTestURLString[] = "http://www.google.com/";
const char kTestAppSpecificURL[] = "testwebui://test/";

const char kTestMimeType[] = "application/vnd.test";

// Returns HTML for an optionally zoomable test page with |zoom_state|.
enum PageScalabilityType {
  PAGE_SCALABILITY_DISABLED = 0,
  PAGE_SCALABILITY_ENABLED,
};
NSString* GetHTMLForZoomState(const PageZoomState& zoom_state,
                              PageScalabilityType scalability_type) {
  NSString* const kHTMLFormat =
      @"<html><head><meta name='viewport' content="
       "'width=%f,minimum-scale=%f,maximum-scale=%f,initial-scale=%f,"
       "user-scalable=%@'/></head><body>Test</body></html>";
  CGFloat width = CGRectGetWidth([UIScreen mainScreen].bounds) /
      zoom_state.minimum_zoom_scale();
  BOOL scalability_enabled = scalability_type == PAGE_SCALABILITY_ENABLED;
  return [NSString
      stringWithFormat:kHTMLFormat, width, zoom_state.minimum_zoom_scale(),
                       zoom_state.maximum_zoom_scale(), zoom_state.zoom_scale(),
                       scalability_enabled ? @"yes" : @"no"];
}

// Forces |webController|'s view to render and waits until |webController|'s
// PageZoomState matches |zoom_state|.
void WaitForZoomRendering(CRWWebController* webController,
                          const PageZoomState& zoom_state) {
  ui::test::uiview_utils::ForceViewRendering(webController.view);
  base::test::ios::WaitUntilCondition(^bool() {
    return webController.pageDisplayState.zoom_state() == zoom_state;
  });
}

}  // namespace

// Test fixture for testing CRWWebController. Stubs out web view.
class CRWWebControllerTest : public WebTestWithWebController {
 protected:
  CRWWebControllerTest()
      : WebTestWithWebController(std::make_unique<TestWebClient>()) {}

  void SetUp() override {
    WebTestWithWebController::SetUp();
    mock_web_view_ = CreateMockWebView();
    scroll_view_ = [[UIScrollView alloc] init];
    SetWebViewURL(@(kTestURLString));
    [[[mock_web_view_ stub] andReturn:scroll_view_] scrollView];

    TestWebViewContentView* web_view_content_view =
        [[TestWebViewContentView alloc] initWithMockWebView:mock_web_view_
                                                 scrollView:scroll_view_];
    [web_controller() injectWebViewContentView:web_view_content_view];
  }

  void TearDown() override {
    EXPECT_OCMOCK_VERIFY(mock_web_view_);
    [web_controller() resetInjectedWebViewContentView];
    WebTestWithWebController::TearDown();
  }

  TestWebClient* GetWebClient() override {
    return static_cast<TestWebClient*>(
        WebTestWithWebController::GetWebClient());
  }

  // The value for web view OCMock objects to expect for |-setFrame:|.
  CGRect GetExpectedWebViewFrame() const {
    CGSize container_view_size = [UIScreen mainScreen].bounds.size;
    container_view_size.height -=
        CGRectGetHeight([UIApplication sharedApplication].statusBarFrame);
    return {CGPointZero, container_view_size};
  }

  void SetWebViewURL(NSString* url_string) {
    test_url_ = [NSURL URLWithString:url_string];
  }

  // Creates WebView mock.
  UIView* CreateMockWebView() {
    id result = [OCMockObject mockForClass:[WKWebView class]];
    [[result stub] serverTrust];

    mock_wk_list_ = [[CRWFakeBackForwardList alloc] init];
    OCMStub([result backForwardList]).andReturn(mock_wk_list_);
    // This uses |andDo| rather than |andReturn| since the URL it returns needs
    // to change when |test_url_| changes.
    OCMStub([result URL]).andDo(^(NSInvocation* invocation) {
      [invocation setReturnValue:&test_url_];
    });
    [[result stub] setNavigationDelegate:[OCMArg checkWithBlock:^(id delegate) {
                     navigation_delegate_ = delegate;
                     return YES;
                   }]];
    [[result stub] setUIDelegate:OCMOCK_ANY];
    [[result stub] setFrame:GetExpectedWebViewFrame()];
    [[result stub] addObserver:web_controller()
                    forKeyPath:OCMOCK_ANY
                       options:0
                       context:nullptr];
    [[result stub] removeObserver:web_controller() forKeyPath:OCMOCK_ANY];

    return result;
  }

  __weak id<WKNavigationDelegate> navigation_delegate_;
  UIScrollView* scroll_view_;
  id mock_web_view_;
  CRWFakeBackForwardList* mock_wk_list_;
  NSURL* test_url_;
};

// Tests that AllowCertificateError is called with correct arguments if
// WKWebView fails to load a page with bad SSL cert.
TEST_F(CRWWebControllerTest, SslCertError) {
  // Last arguments passed to AllowCertificateError must be in default state.
  ASSERT_FALSE(GetWebClient()->last_cert_error_code());
  ASSERT_FALSE(GetWebClient()->last_cert_error_ssl_info().is_valid());
  ASSERT_FALSE(GetWebClient()->last_cert_error_ssl_info().cert_status);
  ASSERT_FALSE(GetWebClient()->last_cert_error_request_url().is_valid());
  ASSERT_TRUE(GetWebClient()->last_cert_error_overridable());

  scoped_refptr<net::X509Certificate> cert =
      net::ImportCertFromFile(net::GetTestCertsDirectory(), "ok_cert.pem");
  ASSERT_TRUE(cert);
  base::ScopedCFTypeRef<CFMutableArrayRef> chain(
      net::x509_util::CreateSecCertificateArrayForX509Certificate(cert.get()));
  ASSERT_TRUE(chain);

  GURL url("https://chromium.test");
  NSError* error =
      [NSError errorWithDomain:NSURLErrorDomain
                          code:NSURLErrorServerCertificateHasUnknownRoot
                      userInfo:@{
                        kNSErrorPeerCertificateChainKey :
                            base::mac::CFToNSCast(chain.get()),
                        kNSErrorFailingURLKey : net::NSURLWithGURL(url),
                      }];
  NSObject* navigation = [[NSObject alloc] init];
  [navigation_delegate_ webView:mock_web_view_
      didStartProvisionalNavigation:static_cast<WKNavigation*>(navigation)];
  [navigation_delegate_ webView:mock_web_view_
      didFailProvisionalNavigation:static_cast<WKNavigation*>(navigation)
                         withError:error];

  // Verify correctness of AllowCertificateError method call.
  EXPECT_EQ(net::ERR_CERT_INVALID, GetWebClient()->last_cert_error_code());
  EXPECT_TRUE(GetWebClient()->last_cert_error_ssl_info().is_valid());
  EXPECT_EQ(net::CERT_STATUS_INVALID,
            GetWebClient()->last_cert_error_ssl_info().cert_status);
  EXPECT_EQ(url, GetWebClient()->last_cert_error_request_url());
  EXPECT_FALSE(GetWebClient()->last_cert_error_overridable());
}

// Test fixture to test |WebState::SetShouldSuppressDialogs|.
class DialogsSuppressionTest : public WebTestWithWebState {
 protected:
  DialogsSuppressionTest() : page_url_("https://chromium.test/") {}
  void SetUp() override {
    WebTestWithWebState::SetUp();
    LoadHtml(@"<html><body></body></html>", page_url_);
    web_state()->SetDelegate(&test_web_delegate_);
  }
  void TearDown() override {
    web_state()->SetDelegate(nullptr);
    WebTestWithWebState::TearDown();
  }
  TestJavaScriptDialogPresenter* js_dialog_presenter() {
    return test_web_delegate_.GetTestJavaScriptDialogPresenter();
  }
  const std::vector<TestJavaScriptDialog>& requested_dialogs() {
    return js_dialog_presenter()->requested_dialogs();
  }
  const GURL& page_url() { return page_url_; }

 private:
  TestWebStateDelegate test_web_delegate_;
  GURL page_url_;
};

// Tests that window.alert dialog is suppressed.
TEST_F(DialogsSuppressionTest, SuppressAlert) {
  TestWebStateObserver observer(web_state());
  ASSERT_FALSE(observer.did_suppress_dialog_info());
  web_state()->SetShouldSuppressDialogs(true);
  ExecuteJavaScript(@"alert('test')");
  ASSERT_TRUE(observer.did_suppress_dialog_info());
  EXPECT_EQ(web_state(), observer.did_suppress_dialog_info()->web_state);
};

// Tests that window.alert dialog is shown.
TEST_F(DialogsSuppressionTest, AllowAlert) {
  TestWebStateObserver observer(web_state());
  ASSERT_FALSE(observer.did_suppress_dialog_info());
  ASSERT_TRUE(requested_dialogs().empty());

  web_state()->SetShouldSuppressDialogs(false);
  ExecuteJavaScript(@"alert('test')");

  ASSERT_EQ(1U, requested_dialogs().size());
  TestJavaScriptDialog dialog = requested_dialogs()[0];
  EXPECT_EQ(web_state(), dialog.web_state);
  EXPECT_EQ(page_url(), dialog.origin_url);
  EXPECT_EQ(JAVASCRIPT_DIALOG_TYPE_ALERT, dialog.java_script_dialog_type);
  EXPECT_NSEQ(@"test", dialog.message_text);
  EXPECT_FALSE(dialog.default_prompt_text);
  ASSERT_FALSE(observer.did_suppress_dialog_info());
};

// Tests that window.confirm dialog is suppressed.
TEST_F(DialogsSuppressionTest, SuppressConfirm) {
  TestWebStateObserver observer(web_state());
  ASSERT_FALSE(observer.did_suppress_dialog_info());
  ASSERT_TRUE(requested_dialogs().empty());

  web_state()->SetShouldSuppressDialogs(true);
  EXPECT_NSEQ(@NO, ExecuteJavaScript(@"confirm('test')"));

  ASSERT_TRUE(requested_dialogs().empty());
  ASSERT_TRUE(observer.did_suppress_dialog_info());
  EXPECT_EQ(web_state(), observer.did_suppress_dialog_info()->web_state);
};

// Tests that window.confirm dialog is shown and its result is true.
TEST_F(DialogsSuppressionTest, AllowConfirmWithTrue) {
  TestWebStateObserver observer(web_state());
  ASSERT_FALSE(observer.did_suppress_dialog_info());
  ASSERT_TRUE(requested_dialogs().empty());

  js_dialog_presenter()->set_callback_success_argument(true);

  web_state()->SetShouldSuppressDialogs(false);
  EXPECT_NSEQ(@YES, ExecuteJavaScript(@"confirm('test')"));

  ASSERT_EQ(1U, requested_dialogs().size());
  TestJavaScriptDialog dialog = requested_dialogs()[0];
  EXPECT_EQ(web_state(), dialog.web_state);
  EXPECT_EQ(page_url(), dialog.origin_url);
  EXPECT_EQ(JAVASCRIPT_DIALOG_TYPE_CONFIRM, dialog.java_script_dialog_type);
  EXPECT_NSEQ(@"test", dialog.message_text);
  EXPECT_FALSE(dialog.default_prompt_text);
  ASSERT_FALSE(observer.did_suppress_dialog_info());
}

// Tests that window.confirm dialog is shown and its result is false.
TEST_F(DialogsSuppressionTest, AllowConfirmWithFalse) {
  TestWebStateObserver observer(web_state());
  ASSERT_FALSE(observer.did_suppress_dialog_info());
  ASSERT_TRUE(requested_dialogs().empty());

  web_state()->SetShouldSuppressDialogs(false);
  EXPECT_NSEQ(@NO, ExecuteJavaScript(@"confirm('test')"));

  ASSERT_EQ(1U, requested_dialogs().size());
  TestJavaScriptDialog dialog = requested_dialogs()[0];
  EXPECT_EQ(web_state(), dialog.web_state);
  EXPECT_EQ(page_url(), dialog.origin_url);
  EXPECT_EQ(JAVASCRIPT_DIALOG_TYPE_CONFIRM, dialog.java_script_dialog_type);
  EXPECT_NSEQ(@"test", dialog.message_text);
  EXPECT_FALSE(dialog.default_prompt_text);
  ASSERT_FALSE(observer.did_suppress_dialog_info());
}

// Tests that window.prompt dialog is suppressed.
TEST_F(DialogsSuppressionTest, SuppressPrompt) {
  TestWebStateObserver observer(web_state());
  ASSERT_FALSE(observer.did_suppress_dialog_info());
  ASSERT_TRUE(requested_dialogs().empty());

  web_state()->SetShouldSuppressDialogs(true);
  EXPECT_EQ([NSNull null], ExecuteJavaScript(@"prompt('Yes?', 'No')"));

  ASSERT_TRUE(requested_dialogs().empty());
  ASSERT_TRUE(observer.did_suppress_dialog_info());
  EXPECT_EQ(web_state(), observer.did_suppress_dialog_info()->web_state);
}

// Tests that window.prompt dialog is shown.
TEST_F(DialogsSuppressionTest, AllowPrompt) {
  TestWebStateObserver observer(web_state());
  ASSERT_FALSE(observer.did_suppress_dialog_info());
  ASSERT_TRUE(requested_dialogs().empty());

  js_dialog_presenter()->set_callback_user_input_argument(@"Maybe");

  web_state()->SetShouldSuppressDialogs(false);
  EXPECT_NSEQ(@"Maybe", ExecuteJavaScript(@"prompt('Yes?', 'No')"));

  ASSERT_EQ(1U, requested_dialogs().size());
  TestJavaScriptDialog dialog = requested_dialogs()[0];
  EXPECT_EQ(web_state(), dialog.web_state);
  EXPECT_EQ(page_url(), dialog.origin_url);
  EXPECT_EQ(JAVASCRIPT_DIALOG_TYPE_PROMPT, dialog.java_script_dialog_type);
  EXPECT_NSEQ(@"Yes?", dialog.message_text);
  EXPECT_NSEQ(@"No", dialog.default_prompt_text);
  ASSERT_FALSE(observer.did_suppress_dialog_info());
}

// Tests that window.open is suppressed.
TEST_F(DialogsSuppressionTest, SuppressWindowOpen) {
  TestWebStateObserver observer(web_state());
  ASSERT_FALSE(observer.did_suppress_dialog_info());
  ASSERT_TRUE(requested_dialogs().empty());

  web_state()->SetShouldSuppressDialogs(true);
  ExecuteJavaScript(@"window.open('')");

  ASSERT_TRUE(observer.did_suppress_dialog_info());
  EXPECT_EQ(web_state(), observer.did_suppress_dialog_info()->web_state);
}

// A separate test class, as none of the |CRWWebControllerTest| setup is
// needed.
class CRWWebControllerPageScrollStateTest : public WebTestWithWebController {
 protected:
  // Returns a PageDisplayState that will scroll a WKWebView to
  // |scrollOffset| and zoom the content by |relativeZoomScale|.
  inline PageDisplayState CreateTestPageDisplayState(
      CGPoint scroll_offset,
      CGFloat relative_zoom_scale,
      CGFloat original_minimum_zoom_scale,
      CGFloat original_maximum_zoom_scale,
      CGFloat original_zoom_scale) const {
    return PageDisplayState(scroll_offset.x, scroll_offset.y,
                            original_minimum_zoom_scale,
                            original_maximum_zoom_scale,
                            relative_zoom_scale * original_minimum_zoom_scale);
  }
};

// TODO(crbug/493427): Flaky on the bots.
TEST_F(CRWWebControllerPageScrollStateTest,
       FLAKY_SetPageDisplayStateWithUserScalableDisabled) {
#if !TARGET_IPHONE_SIMULATOR
  // TODO(crbug.com/493427): fails flakily on device, so skip it there.
  return;
#endif
  PageZoomState zoom_state(1.0, 5.0, 1.0);
  LoadHtml(GetHTMLForZoomState(zoom_state, PAGE_SCALABILITY_DISABLED));
  WaitForZoomRendering(web_controller(), zoom_state);
  PageZoomState original_zoom_state =
      web_controller().pageDisplayState.zoom_state();

  NavigationManager* nagivation_manager = web_state()->GetNavigationManager();
  nagivation_manager->GetLastCommittedItem()->SetPageDisplayState(
      CreateTestPageDisplayState(CGPointMake(1.0, 1.0),  // scroll offset
                                 3.0,                    // relative zoom scale
                                 1.0,    // original minimum zoom scale
                                 5.0,    // original maximum zoom scale
                                 1.0));  // original zoom scale
  [web_controller() restoreStateFromHistory];

  // |-restoreStateFromHistory| is async; wait for its completion.
  base::test::ios::WaitUntilCondition(^bool() {
    return web_controller().pageDisplayState.scroll_state().offset_x() == 1.0;
  });

  ASSERT_EQ(original_zoom_state,
            web_controller().pageDisplayState.zoom_state());
};

// TODO(crbug/493427): Flaky on the bots.
TEST_F(CRWWebControllerPageScrollStateTest,
       FLAKY_SetPageDisplayStateWithUserScalableEnabled) {
#if !TARGET_IPHONE_SIMULATOR
  // TODO(crbug.com/493427): fails flakily on device, so skip it there.
  return;
#endif
  PageZoomState zoom_state(1.0, 5.0, 1.0);

  LoadHtml(GetHTMLForZoomState(zoom_state, PAGE_SCALABILITY_ENABLED));
  WaitForZoomRendering(web_controller(), zoom_state);

  NavigationManager* nagivation_manager = web_state()->GetNavigationManager();
  nagivation_manager->GetLastCommittedItem()->SetPageDisplayState(
      CreateTestPageDisplayState(CGPointMake(1.0, 1.0),  // scroll offset
                                 3.0,                    // relative zoom scale
                                 1.0,    // original minimum zoom scale
                                 5.0,    // original maximum zoom scale
                                 1.0));  // original zoom scale
  [web_controller() restoreStateFromHistory];

  // |-restoreStateFromHistory| is async; wait for its completion.
  base::test::ios::WaitUntilCondition(^bool() {
    return web_controller().pageDisplayState.scroll_state().offset_x() == 1.0;
  });

  PageZoomState final_zoom_state =
      web_controller().pageDisplayState.zoom_state();
  EXPECT_FLOAT_EQ(3, final_zoom_state.zoom_scale() /
                        final_zoom_state.minimum_zoom_scale());
};

// Test fixture for testing visible security state.
typedef WebTestWithWebState CRWWebStateSecurityStateTest;

// Tests that loading HTTP page updates the SSLStatus.
TEST_F(CRWWebStateSecurityStateTest, LoadHttpPage) {
  TestWebStateObserver observer(web_state());
  ASSERT_FALSE(observer.did_change_visible_security_state_info());
  LoadHtml(@"<html><body></body></html>", GURL("http://chromium.test"));
  NavigationManager* nav_manager = web_state()->GetNavigationManager();
  NavigationItem* item = nav_manager->GetLastCommittedItem();
  EXPECT_EQ(SECURITY_STYLE_UNAUTHENTICATED, item->GetSSL().security_style);
  ASSERT_TRUE(observer.did_change_visible_security_state_info());
  EXPECT_EQ(web_state(),
            observer.did_change_visible_security_state_info()->web_state);
}

// Real WKWebView is required for CRWWebControllerInvalidUrlTest.
typedef WebTestWithWebState CRWWebControllerInvalidUrlTest;

// Tests that web controller does not navigate to about:blank if iframe src
// has invalid url. Web controller loads about:blank if page navigates to
// invalid url, but should do nothing if navigation is performed in iframe. This
// test prevents crbug.com/694865 regression.
TEST_F(CRWWebControllerInvalidUrlTest, IFrameWithInvalidURL) {
  GURL url("http://chromium.test");
  ASSERT_FALSE(GURL(kInvalidURL).is_valid());
  LoadHtml([NSString stringWithFormat:@"<iframe src='%s'/>", kInvalidURL], url);
  EXPECT_EQ(url, web_state()->GetLastCommittedURL());
}

// Real WKWebView is required for CRWWebControllerMessageFromIFrame.
typedef WebTestWithWebState CRWWebControllerMessageFromIFrame;

// Tests that invalid message from iframe does not cause a crash.
TEST_F(CRWWebControllerMessageFromIFrame, InvalidMessage) {
  static NSString* const kHTMLIFrameSendsInvalidMessage =
      @"<body><iframe name='f'></iframe></body>";

  LoadHtml(kHTMLIFrameSendsInvalidMessage);

  // Sending unknown command from iframe should not cause a crash.
  ExecuteJavaScript(
      @"var bad_message = {'command' : 'unknown.command'};"
       "frames['f'].__gCrWeb.message.invokeOnHost(bad_message);");
}

// Real WKWebView is required for CRWWebControllerJSExecutionTest.
typedef WebTestWithWebController CRWWebControllerJSExecutionTest;

// Tests that a script correctly evaluates to boolean.
TEST_F(CRWWebControllerJSExecutionTest, Execution) {
  LoadHtml(@"<p></p>");
  EXPECT_NSEQ(@YES, ExecuteJavaScript(@"true"));
  EXPECT_NSEQ(@NO, ExecuteJavaScript(@"false"));
}

// Tests that a script is not executed on windowID mismatch.
TEST_F(CRWWebControllerJSExecutionTest, WindowIdMissmatch) {
  LoadHtml(@"<p></p>");
  // Script is evaluated since windowID is matched.
  ExecuteJavaScript(@"window.test1 = '1';");
  EXPECT_NSEQ(@"1", ExecuteJavaScript(@"window.test1"));

  // Change windowID.
  ExecuteJavaScript(@"__gCrWeb['windowId'] = '';");

  // Script is not evaluated because of windowID mismatch.
  ExecuteJavaScript(@"window.test2 = '2';");
  EXPECT_FALSE(ExecuteJavaScript(@"window.test2"));
}

// Test fixture to test that DownloadControllerDelegate::OnDownloadCreated
// callback is trigerred if CRWWebController can not display the response.
class CRWWebControllerDownloadTest : public CRWWebControllerTest {
 protected:
  CRWWebControllerDownloadTest() : delegate_(download_controller()) {}

  // Calls webView:decidePolicyForNavigationResponse:decisionHandler: callback
  // and waits for decision handler call. Returns false if decision handler call
  // times out.
  bool CallDecidePolicyForNavigationResponseWithResponse(
      NSURLResponse* response,
      BOOL for_main_frame) WARN_UNUSED_RESULT {
    CRWFakeWKNavigationResponse* navigation_response =
        [[CRWFakeWKNavigationResponse alloc] init];
    navigation_response.response = response;
    navigation_response.forMainFrame = for_main_frame;

    // Wait for decidePolicyForNavigationResponse: callback.
    __block bool callback_called = false;
    if (for_main_frame) {
      // webView:didStartProvisionalNavigation: is not called for iframes.
      [navigation_delegate_ webView:mock_web_view_
          didStartProvisionalNavigation:nil];
    }
    [navigation_delegate_ webView:mock_web_view_
        decidePolicyForNavigationResponse:navigation_response
                          decisionHandler:^(WKNavigationResponsePolicy policy) {
                            callback_called = true;
                          }];
    return WaitUntilConditionOrTimeout(kWaitForPageLoadTimeout, ^{
      return callback_called;
    });
  }

  DownloadController* download_controller() {
    return DownloadController::FromBrowserState(GetBrowserState());
  }

  FakeDownloadControllerDelegate delegate_;
};

// Tests that webView:decidePolicyForNavigationResponse:decisionHandler: creates
// the DownloadTask for NSURLResponse.
TEST_F(CRWWebControllerDownloadTest, CreationWithNSURLResponse) {
  // Simulate download response.
  int64_t content_length = 10;
  NSURLResponse* response =
      [[NSURLResponse alloc] initWithURL:[NSURL URLWithString:@(kTestURLString)]
                                MIMEType:@(kTestMimeType)
                   expectedContentLength:content_length
                        textEncodingName:nil];
  ASSERT_TRUE(CallDecidePolicyForNavigationResponseWithResponse(
      response, /*for_main_frame=*/YES));

  // Verify that download task was created.
  ASSERT_EQ(1U, delegate_.alive_download_tasks().size());
  DownloadTask* task = delegate_.alive_download_tasks()[0].second.get();
  ASSERT_TRUE(task);
  EXPECT_TRUE(task->GetIndentifier());
  EXPECT_EQ(kTestURLString, task->GetOriginalUrl());
  EXPECT_EQ(content_length, task->GetTotalBytes());
  EXPECT_EQ("", task->GetContentDisposition());
  EXPECT_EQ(kTestMimeType, task->GetMimeType());
  EXPECT_TRUE(ui::PageTransitionTypeIncludingQualifiersIs(
      task->GetTransitionType(),
      ui::PageTransition::PAGE_TRANSITION_CLIENT_REDIRECT));
}

// Tests that webView:decidePolicyForNavigationResponse:decisionHandler: creates
// the DownloadTask for NSHTTPURLResponse.
TEST_F(CRWWebControllerDownloadTest, CreationWithNSHTTPURLResponse) {
  // Simulate download response.
  const char kContentDisposition[] = "attachment; filename=download.test";
  NSURLResponse* response = [[NSHTTPURLResponse alloc]
       initWithURL:[NSURL URLWithString:@(kTestURLString)]
        statusCode:200
       HTTPVersion:nil
      headerFields:@{
        @"content-disposition" : @(kContentDisposition),
      }];
  ASSERT_TRUE(CallDecidePolicyForNavigationResponseWithResponse(
      response, /*for_main_frame=*/YES));

  // Verify that download task was created.
  ASSERT_EQ(1U, delegate_.alive_download_tasks().size());
  DownloadTask* task = delegate_.alive_download_tasks()[0].second.get();
  ASSERT_TRUE(task);
  EXPECT_TRUE(task->GetIndentifier());
  EXPECT_EQ(kTestURLString, task->GetOriginalUrl());
  EXPECT_EQ(-1, task->GetTotalBytes());
  EXPECT_EQ(kContentDisposition, task->GetContentDisposition());
  EXPECT_EQ("", task->GetMimeType());
  EXPECT_TRUE(ui::PageTransitionTypeIncludingQualifiersIs(
      task->GetTransitionType(),
      ui::PageTransition::PAGE_TRANSITION_CLIENT_REDIRECT));
}

// Tests that webView:decidePolicyForNavigationResponse:decisionHandler: creates
// the DownloadTask for NSHTTPURLResponse and iframes.
TEST_F(CRWWebControllerDownloadTest, IFrameCreationWithNSHTTPURLResponse) {
  // Simulate download response.
  const char kContentDisposition[] = "attachment; filename=download.test";
  NSURLResponse* response = [[NSHTTPURLResponse alloc]
       initWithURL:[NSURL URLWithString:@(kTestURLString)]
        statusCode:200
       HTTPVersion:nil
      headerFields:@{
        @"content-disposition" : @(kContentDisposition),
      }];
  ASSERT_TRUE(CallDecidePolicyForNavigationResponseWithResponse(
      response, /*for_main_frame=*/NO));

  // Verify that download task was created.
  ASSERT_EQ(1U, delegate_.alive_download_tasks().size());
  DownloadTask* task = delegate_.alive_download_tasks()[0].second.get();
  ASSERT_TRUE(task);
  EXPECT_TRUE(task->GetIndentifier());
  EXPECT_EQ(kTestURLString, task->GetOriginalUrl());
  EXPECT_EQ(-1, task->GetTotalBytes());
  EXPECT_EQ(kContentDisposition, task->GetContentDisposition());
  EXPECT_EQ("", task->GetMimeType());
  EXPECT_TRUE(ui::PageTransitionTypeIncludingQualifiersIs(
      task->GetTransitionType(),
      ui::PageTransition::PAGE_TRANSITION_AUTO_SUBFRAME));
}

// Tests that webView:decidePolicyForNavigationResponse:decisionHandler: does
// not create the DownloadTask for unsupported data:// URLs.
TEST_F(CRWWebControllerDownloadTest, DataUrlResponse) {
  // Simulate download response.
  NSURLResponse* response =
      [[NSHTTPURLResponse alloc] initWithURL:[NSURL URLWithString:@"data:data"]
                                  statusCode:200
                                 HTTPVersion:nil
                                headerFields:nil];
  ASSERT_TRUE(CallDecidePolicyForNavigationResponseWithResponse(
      response, /*for_main_frame=*/YES));

  // Verify that download task was not created.
  EXPECT_TRUE(delegate_.alive_download_tasks().empty());
}

// Tests |currentURLWithTrustLevel:| method.
TEST_F(CRWWebControllerTest, CurrentUrlWithTrustLevel) {
  AddPendingItem(GURL("http://chromium.test"), ui::PAGE_TRANSITION_TYPED);

  [[[mock_web_view_ stub] andReturnBool:NO] hasOnlySecureContent];
  [static_cast<WKWebView*>([[mock_web_view_ stub] andReturn:@""]) title];

  // Stub out the injection process.
  [[mock_web_view_ stub] evaluateJavaScript:OCMOCK_ANY
                          completionHandler:OCMOCK_ANY];

  // Simulate a page load to trigger a URL update.
  [navigation_delegate_ webView:mock_web_view_
      didStartProvisionalNavigation:nil];
  [mock_wk_list_ setCurrentURL:@"http://chromium.test"];
  [navigation_delegate_ webView:mock_web_view_ didCommitNavigation:nil];

  URLVerificationTrustLevel trust_level = kNone;
  GURL url = [web_controller() currentURLWithTrustLevel:&trust_level];

  EXPECT_EQ(GURL(kTestURLString), url);
  EXPECT_EQ(kAbsolute, trust_level);
}

// Tests that when a placeholder navigation is preempted by another navigation,
// WebStateObservers get neither a DidStartNavigation nor a DidFinishNavigation
// call for the corresponding native URL navigation.
TEST_F(CRWWebControllerTest, AbortNativeUrlNavigation) {
  // The legacy navigation manager doesn't have the concept of placeholder
  // navigations.
  if (!GetWebClient()->IsSlimNavigationManagerEnabled())
    return;
  GURL native_url(
      url::SchemeHostPort(kTestNativeContentScheme, "ui", 0).Serialize());
  NSString* placeholder_url = [NSString
      stringWithFormat:@"%s%s", "about:blank?for=", native_url.spec().c_str()];
  TestWebStateObserver observer(web_state());

  WKNavigation* navigation =
      static_cast<WKNavigation*>([[NSObject alloc] init]);
  [static_cast<WKWebView*>([[mock_web_view_ stub] andReturn:navigation])
      loadRequest:OCMOCK_ANY];
  TestNativeContentProvider* mock_native_provider =
      [[TestNativeContentProvider alloc] init];
  [web_controller() setNativeProvider:mock_native_provider];

  AddPendingItem(native_url, ui::PAGE_TRANSITION_TYPED);

  // Trigger a placeholder navigation.
  [web_controller() loadCurrentURL];

  // Simulate the WKNavigationDelegate callbacks for the placeholder navigation
  // arriving after another pending item has already been created.
  AddPendingItem(GURL(kTestURLString), ui::PAGE_TRANSITION_TYPED);
  SetWebViewURL(placeholder_url);
  [navigation_delegate_ webView:mock_web_view_
      didStartProvisionalNavigation:navigation];
  [navigation_delegate_ webView:mock_web_view_ didCommitNavigation:navigation];
  [navigation_delegate_ webView:mock_web_view_ didFinishNavigation:navigation];

  EXPECT_FALSE(observer.did_start_navigation_info());
  EXPECT_FALSE(observer.did_finish_navigation_info());
}

// Test fixture for testing CRWWebController presenting native content.
class CRWWebControllerNativeContentTest : public WebTestWithWebController {
 protected:
  void SetUp() override {
    WebTestWithWebController::SetUp();
    mock_native_provider_ = [[TestNativeContentProvider alloc] init];
    [web_controller() setNativeProvider:mock_native_provider_];
  }

  void Load(const GURL& URL) {
    TestWebStateObserver observer(web_state());
    NavigationManagerImpl& navigation_manager =
        [web_controller() webStateImpl]->GetNavigationManagerImpl();
    navigation_manager.AddPendingItem(
        URL, Referrer(), ui::PAGE_TRANSITION_TYPED,
        NavigationInitiationType::BROWSER_INITIATED,
        NavigationManager::UserAgentOverrideOption::INHERIT);
    [web_controller() loadCurrentURL];

    // Native URL is loaded asynchronously with WKBasedNavigationManager. Wait
    // for navigation to finish before asserting.
    if (GetWebClient()->IsSlimNavigationManagerEnabled()) {
      TestWebStateObserver* observer_ptr = &observer;
      ASSERT_TRUE(WaitUntilConditionOrTimeout(kWaitForPageLoadTimeout, ^{
        return observer_ptr->did_finish_navigation_info() != nullptr;
      }));
    }
  }

  TestNativeContentProvider* mock_native_provider_;
};

// Tests WebState and NavigationManager correctly return native content URL.
TEST_F(CRWWebControllerNativeContentTest, NativeContentURL) {
  GURL url_to_load(kTestAppSpecificURL);
  TestNativeContent* content =
      [[TestNativeContent alloc] initWithURL:url_to_load virtualURL:GURL()];
  [mock_native_provider_ setController:content forURL:url_to_load];
  Load(url_to_load);
  URLVerificationTrustLevel trust_level = kNone;
  GURL gurl = [web_controller() currentURLWithTrustLevel:&trust_level];
  EXPECT_EQ(gurl, url_to_load);
  EXPECT_EQ(kAbsolute, trust_level);
  EXPECT_EQ([web_controller() webState]->GetVisibleURL(), url_to_load);
  NavigationManagerImpl& navigationManager =
      [web_controller() webStateImpl]->GetNavigationManagerImpl();
  EXPECT_EQ(navigationManager.GetVisibleItem()->GetURL(), url_to_load);
  EXPECT_EQ(navigationManager.GetVisibleItem()->GetVirtualURL(), url_to_load);
  EXPECT_EQ(navigationManager.GetLastCommittedItem()->GetURL(), url_to_load);
  EXPECT_EQ(navigationManager.GetLastCommittedItem()->GetVirtualURL(),
            url_to_load);
}

// Tests WebState and NavigationManager correctly return native content URL and
// VirtualURL
TEST_F(CRWWebControllerNativeContentTest, NativeContentVirtualURL) {
  GURL url_to_load(kTestAppSpecificURL);
  GURL virtual_url(kTestURLString);
  TestNativeContent* content =
      [[TestNativeContent alloc] initWithURL:virtual_url
                                  virtualURL:virtual_url];
  [mock_native_provider_ setController:content forURL:url_to_load];
  Load(url_to_load);
  URLVerificationTrustLevel trust_level = kNone;
  GURL gurl = [web_controller() currentURLWithTrustLevel:&trust_level];
  EXPECT_EQ(gurl, virtual_url);
  EXPECT_EQ(kAbsolute, trust_level);
  EXPECT_EQ([web_controller() webState]->GetVisibleURL(), virtual_url);
  NavigationManagerImpl& navigationManager =
      [web_controller() webStateImpl]->GetNavigationManagerImpl();
  EXPECT_EQ(navigationManager.GetVisibleItem()->GetURL(), url_to_load);
  EXPECT_EQ(navigationManager.GetVisibleItem()->GetVirtualURL(), virtual_url);
  EXPECT_EQ(navigationManager.GetLastCommittedItem()->GetURL(), url_to_load);
  EXPECT_EQ(navigationManager.GetLastCommittedItem()->GetVirtualURL(),
            virtual_url);
}

// Test fixture for window.open tests.
class WindowOpenByDomTest : public WebTestWithWebController {
 protected:
  WindowOpenByDomTest() : opener_url_("http://test") {}

  void SetUp() override {
    WebTestWithWebController::SetUp();
    web_state()->SetDelegate(&delegate_);
    LoadHtml(@"<html><body></body></html>", opener_url_);
  }
  // Executes JavaScript that opens a new window and returns evaluation result
  // as a string.
  id OpenWindowByDom() {
    NSString* const kOpenWindowScript =
        @"w = window.open('javascript:void(0);', target='_blank');"
         "w ? w.toString() : null;";
    id windowJSObject = ExecuteJavaScript(kOpenWindowScript);
    return windowJSObject;
  }

  // Executes JavaScript that closes previously opened window.
  void CloseWindow() { ExecuteJavaScript(@"w.close()"); }

  // URL of a page which opens child windows.
  const GURL opener_url_;
  TestWebStateDelegate delegate_;
};

// Tests that absence of web state delegate is handled gracefully.
TEST_F(WindowOpenByDomTest, NoDelegate) {
  web_state()->SetDelegate(nullptr);

  EXPECT_NSEQ([NSNull null], OpenWindowByDom());

  EXPECT_TRUE(delegate_.child_windows().empty());
  EXPECT_TRUE(delegate_.popups().empty());
}

// Tests that window.open triggered by user gesture opens a new non-popup
// window.
TEST_F(WindowOpenByDomTest, OpenWithUserGesture) {
  [web_controller() touched:YES];
  EXPECT_NSEQ(@"[object Window]", OpenWindowByDom());

  ASSERT_EQ(1U, delegate_.child_windows().size());
  ASSERT_TRUE(delegate_.child_windows()[0]);
  EXPECT_TRUE(delegate_.popups().empty());
}

// Tests that window.open executed w/o user gesture does not open a new window,
// but blocks popup instead.
TEST_F(WindowOpenByDomTest, BlockPopup) {
  ASSERT_FALSE([web_controller() userIsInteracting]);
  EXPECT_NSEQ([NSNull null], OpenWindowByDom());

  EXPECT_TRUE(delegate_.child_windows().empty());
  ASSERT_EQ(1U, delegate_.popups().size());
  EXPECT_EQ(GURL("javascript:void(0);"), delegate_.popups()[0].url);
  EXPECT_EQ(opener_url_, delegate_.popups()[0].opener_url);
}

// Tests that window.open executed w/o user gesture opens a new window, assuming
// that delegate allows popups.
TEST_F(WindowOpenByDomTest, DontBlockPopup) {
  delegate_.allow_popups(opener_url_);
  EXPECT_NSEQ(@"[object Window]", OpenWindowByDom());

  ASSERT_EQ(1U, delegate_.child_windows().size());
  ASSERT_TRUE(delegate_.child_windows()[0]);
  EXPECT_TRUE(delegate_.popups().empty());
}

// Tests that window.close closes the web state.
TEST_F(WindowOpenByDomTest, CloseWindow) {
  delegate_.allow_popups(opener_url_);
  ASSERT_NSEQ(@"[object Window]", OpenWindowByDom());

  ASSERT_EQ(1U, delegate_.child_windows().size());
  ASSERT_TRUE(delegate_.child_windows()[0]);
  EXPECT_TRUE(delegate_.popups().empty());

  delegate_.child_windows()[0]->SetDelegate(&delegate_);
  CloseWindow();

  EXPECT_TRUE(delegate_.child_windows().empty());
  EXPECT_TRUE(delegate_.popups().empty());
}

// Tests page title changes.
typedef WebTestWithWebState CRWWebControllerTitleTest;
TEST_F(CRWWebControllerTitleTest, TitleChange) {
  // Observes and waits for TitleWasSet call.
  class TitleObserver : public WebStateObserver {
   public:
    TitleObserver() = default;

    // Returns number of times |TitleWasSet| was called.
    int title_change_count() { return title_change_count_; }
    // WebStateObserver overrides:
    void TitleWasSet(WebState* web_state) override { title_change_count_++; }
    void WebStateDestroyed(WebState* web_state) override { NOTREACHED(); }

   private:
    int title_change_count_ = 0;

    DISALLOW_COPY_AND_ASSIGN(TitleObserver);
  };

  TitleObserver observer;
  ScopedObserver<WebState, WebStateObserver> scoped_observer(&observer);
  scoped_observer.Add(web_state());
  ASSERT_EQ(0, observer.title_change_count());

  // Expect TitleWasSet callback after the page is loaded.
  LoadHtml(@"<title>Title1</title>");
  EXPECT_EQ("Title1", base::UTF16ToUTF8(web_state()->GetTitle()));
  EXPECT_EQ(1, observer.title_change_count());

  // Expect at least one more TitleWasSet callback after changing title via
  // JavaScript. On iOS 10 WKWebView fires 3 callbacks after JS excucution
  // with the following title changes: "Title2", "" and "Title2".
  // TODO(crbug.com/696104): There should be only 2 calls of TitleWasSet.
  // Fix expecteation when WKWebView stops sending extra KVO calls.
  ExecuteJavaScript(@"window.document.title = 'Title2';");
  EXPECT_EQ("Title2", base::UTF16ToUTF8(web_state()->GetTitle()));
  EXPECT_GE(observer.title_change_count(), 2);
};

// Tests that fragment change navigations use title from the previous page.
TEST_F(CRWWebControllerTitleTest, FragmentChangeNavigationsUsePreviousTitle) {
  LoadHtml(@"<title>Title1</title>");
  ASSERT_EQ("Title1", base::UTF16ToUTF8(web_state()->GetTitle()));
  ExecuteJavaScript(@"window.location.hash = '#1'");
  EXPECT_EQ("Title1", base::UTF16ToUTF8(web_state()->GetTitle()));
}

// Test fixture for JavaScript execution.
class ScriptExecutionTest : public WebTestWithWebController {
 protected:
  // Calls |executeUserJavaScript:completionHandler:|, waits for script
  // execution completion, and synchronously returns the result.
  id ExecuteUserJavaScript(NSString* java_script, NSError** error) {
    __block id script_result = nil;
    __block NSError* script_error = nil;
    __block bool script_executed = false;
    [web_controller()
        executeUserJavaScript:java_script
            completionHandler:^(id local_result, NSError* local_error) {
              script_result = local_result;
              script_error = local_error;
              script_executed = true;
            }];

    WaitForCondition(^{
      return script_executed;
    });

    if (error) {
      *error = script_error;
    }
    return script_result;
  }
};

// Tests evaluating user script on an http page.
TEST_F(ScriptExecutionTest, UserScriptOnHttpPage) {
  LoadHtml(@"<html></html>", GURL(kTestURLString));
  NSError* error = nil;
  EXPECT_NSEQ(@0, ExecuteUserJavaScript(@"window.w = 0;", &error));
  EXPECT_FALSE(error);

  EXPECT_NSEQ(@0, ExecuteJavaScript(@"window.w"));
};

// Tests evaluating user script on app-specific page. Pages with app-specific
// URLs have elevated privileges and JavaScript execution should not be allowed
// for them.
TEST_F(ScriptExecutionTest, UserScriptOnAppSpecificPage) {
  LoadHtml(@"<html></html>", GURL(kTestURLString));

  // Change last committed URL to app-specific URL.
  NavigationManagerImpl& nav_manager =
      [web_controller() webStateImpl]->GetNavigationManagerImpl();
  nav_manager.AddPendingItem(
      GURL(kTestAppSpecificURL), Referrer(), ui::PAGE_TRANSITION_TYPED,
      NavigationInitiationType::BROWSER_INITIATED,
      NavigationManager::UserAgentOverrideOption::INHERIT);
  [nav_manager.GetSessionController() commitPendingItem];

  NSError* error = nil;
  EXPECT_FALSE(ExecuteUserJavaScript(@"window.w = 0;", &error));
  ASSERT_TRUE(error);
  EXPECT_NSEQ(kJSEvaluationErrorDomain, error.domain);
  EXPECT_EQ(JS_EVALUATION_ERROR_CODE_NO_WEB_VIEW, error.code);

  EXPECT_FALSE(ExecuteJavaScript(@"window.w"));
};

// Fixture class to test WKWebView crashes.
class CRWWebControllerWebProcessTest : public WebTestWithWebController {
 protected:
  void SetUp() override {
    WebTestWithWebController::SetUp();
    webView_ = BuildTerminatedWKWebView();
    TestWebViewContentView* webViewContentView = [[TestWebViewContentView alloc]
        initWithMockWebView:webView_
                 scrollView:[webView_ scrollView]];
    [web_controller() injectWebViewContentView:webViewContentView];

    // This test intentionally crashes the render process.
    SetIgnoreRenderProcessCrashesDuringTesting(true);
  }
  WKWebView* webView_;
};

// Tests that WebStateDelegate::RenderProcessGone is called when WKWebView web
// process has crashed.
TEST_F(CRWWebControllerWebProcessTest, Crash) {
  ASSERT_TRUE([web_controller() isViewAlive]);
  ASSERT_FALSE([web_controller() isWebProcessCrashed]);
  ASSERT_FALSE(web_state()->IsCrashed());
  ASSERT_FALSE(web_state()->IsEvicted());

  TestWebStateObserver observer(web_state());
  TestWebStateObserver* observer_ptr = &observer;
  SimulateWKWebViewCrash(webView_);
  base::test::ios::WaitUntilCondition(^bool() {
    return observer_ptr->render_process_gone_info();
  });
  EXPECT_EQ(web_state(), observer.render_process_gone_info()->web_state);
  EXPECT_FALSE([web_controller() isViewAlive]);
  EXPECT_TRUE([web_controller() isWebProcessCrashed]);
  EXPECT_TRUE(web_state()->IsCrashed());
  EXPECT_TRUE(web_state()->IsEvicted());
};

// Tests that WebState is considered as evicted but not crashed when calling
// SetWebUsageEnabled(false).
TEST_F(CRWWebControllerWebProcessTest, Eviction) {
  ASSERT_TRUE([web_controller() isViewAlive]);
  ASSERT_FALSE([web_controller() isWebProcessCrashed]);
  ASSERT_FALSE(web_state()->IsCrashed());
  ASSERT_FALSE(web_state()->IsEvicted());

  web_state()->SetWebUsageEnabled(false);
  EXPECT_FALSE([web_controller() isViewAlive]);
  EXPECT_FALSE([web_controller() isWebProcessCrashed]);
  EXPECT_FALSE(web_state()->IsCrashed());
  EXPECT_TRUE(web_state()->IsEvicted());
};

// Test fixture for -[CRWWebController loadCurrentURLIfNecessary] method.
class LoadIfNecessaryTest : public WebTest {
 protected:
  void SetUp() override {
    WebTest::SetUp();
    web_state_ = std::make_unique<WebStateImpl>(
        WebState::CreateParams(GetBrowserState()), GetTestSessionStorage());
  }

  void TearDown() override {
    WebTest::TearDown();
    web_state_.reset();
  }

  std::unique_ptr<WebStateImpl> web_state_;

 private:
  // Returns file:// URL which point to a test html file with "pony" text.
  GURL GetTestFileUrl() {
    base::FilePath path;
    base::PathService::Get(base::DIR_MODULE, &path);
    path = path.Append(
        FILE_PATH_LITERAL("ios/testing/data/http_server_files/pony.html"));
    return GURL(base::StringPrintf("file://%s", path.value().c_str()));
  }
  // Returns session storage with a single committed entry with |GetTestFileUrl|
  // url.
  CRWSessionStorage* GetTestSessionStorage() {
    CRWSessionStorage* result = [[CRWSessionStorage alloc] init];
    result.lastCommittedItemIndex = 0;
    CRWNavigationItemStorage* item = [[CRWNavigationItemStorage alloc] init];
    [item setVirtualURL:GetTestFileUrl()];
    [result setItemStorages:@[ item ]];
    return result;
  }
};

// Tests that |loadCurrentURLIfNecessary| restores the page after disabling and
// re-enabling web usage.
TEST_F(LoadIfNecessaryTest, RestoredFromHistory) {
  ASSERT_FALSE(test::IsWebViewContainingText(web_state_.get(), "pony"));
  [web_state_->GetWebController() loadCurrentURLIfNecessary];
  EXPECT_TRUE(test::WaitForWebViewContainingText(web_state_.get(), "pony"));
};

// Tests that |loadCurrentURLIfNecessary| restores the page after disabling and
// re-enabling web usage.
TEST_F(LoadIfNecessaryTest, DisableAndReenableWebUsage) {
  [web_state_->GetWebController() loadCurrentURLIfNecessary];
  EXPECT_TRUE(test::WaitForWebViewContainingText(web_state_.get(), "pony"));

  // Disable and re-enable web usage.
  web_state_->SetWebUsageEnabled(false);
  web_state_->SetWebUsageEnabled(true);

  // |loadCurrentURLIfNecessary| should restore the page.
  ASSERT_FALSE(test::IsWebViewContainingText(web_state_.get(), "pony"));
  [web_state_->GetWebController() loadCurrentURLIfNecessary];
  EXPECT_TRUE(test::WaitForWebViewContainingText(web_state_.get(), "pony"));
};
}  // namespace web
