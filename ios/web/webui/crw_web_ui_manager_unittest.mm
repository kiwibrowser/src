// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/web/webui/crw_web_ui_manager.h"

#include <memory>

#include "base/files/file_path.h"
#include "base/files/file_util.h"
#include "base/logging.h"
#include "base/memory/ptr_util.h"
#include "base/path_service.h"
#include "base/strings/stringprintf.h"
#import "base/strings/sys_string_conversions.h"
#include "base/strings/utf_string_conversions.h"
#include "base/values.h"
#include "ios/web/public/test/fakes/test_browser_state.h"
#import "ios/web/public/test/fakes/test_web_client.h"
#include "ios/web/public/test/scoped_testing_web_client.h"
#include "ios/web/public/test/web_test.h"
#import "ios/web/web_state/web_state_impl.h"
#import "ios/web/webui/crw_web_ui_page_builder.h"
#import "ios/web/webui/url_fetcher_block_adapter.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"
#import "testing/gtest_mac.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

namespace web {

// Path for test favicon file.
const char kFaviconPath[] = "ios/web/test/data/testfavicon.png";
// URL for mock WebUI page.
const char kTestWebUIUrl[] = "testwebui://test/";
// URL for mock favicon.
const char kFaviconUrl[] = "testwebui://favicon/";
// HTML for mock WebUI page.
NSString* kHtml = @"<html>Hello World</html>";

// Mock of WebStateImpl to check that LoadHtml and ExecuteJavaScript are called
// as expected.
class MockWebStateImpl : public WebStateImpl {
 public:
  MockWebStateImpl(const WebState::CreateParams& params)
      : WebStateImpl(params), last_committed_url_(kTestWebUIUrl) {}
  MOCK_METHOD2(LoadWebUIHtml,
               void(const base::string16& html, const GURL& url));
  MOCK_METHOD1(ExecuteJavaScript, void(const base::string16& javascript));
  const GURL& GetLastCommittedURL() const override {
    return last_committed_url_;
  }

 private:
  GURL last_committed_url_;
};

// Mock of URLFetcherBlockAdapter to provide mock resources.
class MockURLFetcherBlockAdapter : public URLFetcherBlockAdapter {
 public:
  MockURLFetcherBlockAdapter(
      const GURL& url,
      net::URLRequestContextGetter* request_context,
      URLFetcherBlockAdapterCompletion completion_handler)
      : URLFetcherBlockAdapter(url, request_context, completion_handler),
        url_(url),
        completion_handler_([completion_handler copy]) {}

  void Start() override {
    if (url_.spec() == kFaviconUrl) {
      base::FilePath favicon_path;
      ASSERT_TRUE(base::PathService::Get(base::DIR_SOURCE_ROOT, &favicon_path));
      favicon_path = favicon_path.AppendASCII(kFaviconPath);
      NSData* favicon = [NSData
          dataWithContentsOfFile:base::SysUTF8ToNSString(favicon_path.value())];
      completion_handler_(favicon, this);
    } else if (url_.scheme().find("test") != std::string::npos) {
      completion_handler_([kHtml dataUsingEncoding:NSUTF8StringEncoding], this);
    } else {
      NOTREACHED();
    }
  }

 private:
  // The URL to fetch.
  const GURL url_;
  // Callback for resource load.
  URLFetcherBlockAdapterCompletion completion_handler_;
};

}  // namespace web

// Subclass of CRWWebUIManager for testing.
@interface CRWTestWebUIManager : CRWWebUIManager
// Use mock URLFetcherBlockAdapter.
- (std::unique_ptr<web::URLFetcherBlockAdapter>)
    fetcherForURL:(const GURL&)URL
completionHandler:(web::URLFetcherBlockAdapterCompletion)handler;
@end

@implementation CRWTestWebUIManager
- (std::unique_ptr<web::URLFetcherBlockAdapter>)
    fetcherForURL:(const GURL&)URL
completionHandler:(web::URLFetcherBlockAdapterCompletion)handler {
  return std::unique_ptr<web::URLFetcherBlockAdapter>(
      new web::MockURLFetcherBlockAdapter(URL, nil, handler));
}
@end

namespace web {

// Test fixture for testing CRWWebUIManager
class CRWWebUIManagerTest : public web::WebTest {
 protected:
  void SetUp() override {
    PlatformTest::SetUp();
    test_browser_state_.reset(new TestBrowserState());
    WebState::CreateParams params(test_browser_state_.get());
    web_state_impl_.reset(new MockWebStateImpl(params));
    web_ui_manager_ =
        [[CRWTestWebUIManager alloc] initWithWebState:web_state_impl_.get()];
  }

  // TestBrowserState for creation of WebStateImpl.
  std::unique_ptr<TestBrowserState> test_browser_state_;
  // MockWebStateImpl for detection of LoadHtml and EvaluateJavaScriptAync
  // calls.
  std::unique_ptr<MockWebStateImpl> web_state_impl_;
  // WebUIManager for testing.
  CRWTestWebUIManager* web_ui_manager_;
};

// Tests that CRWWebUIManager observes provisional navigation and invokes an
// HTML load in web state.
TEST_F(CRWWebUIManagerTest, LoadWebUI) {
  base::string16 html(base::SysNSStringToUTF16(kHtml));
  GURL url(kTestWebUIUrl);
  EXPECT_CALL(*web_state_impl_, LoadWebUIHtml(html, url));
  [web_ui_manager_ loadWebUIForURL:url];
}

}  // namespace web
