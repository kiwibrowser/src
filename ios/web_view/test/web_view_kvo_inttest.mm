// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import <ChromeWebView/ChromeWebView.h>
#import <Foundation/Foundation.h>

#include "base/strings/stringprintf.h"
#import "base/strings/sys_string_conversions.h"
#import "ios/testing/wait_util.h"
#import "ios/web_view/test/observer.h"
#import "ios/web_view/test/web_view_int_test.h"
#import "ios/web_view/test/web_view_test_util.h"
#import "net/base/mac/url_conversions.h"
#include "testing/gtest_mac.h"
#include "url/gurl.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

namespace ios_web_view {

// Tests that the KVO compliant properties of CWVWebView correctly report
// changes.
typedef ios_web_view::WebViewIntTest WebViewKvoTest;

// Tests that CWVWebView correctly reports |canGoBack| and |canGoForward| state.
TEST_F(WebViewKvoTest, CanGoBackForward) {
  Observer* back_observer = [[Observer alloc] init];
  [back_observer setObservedObject:web_view_ keyPath:@"canGoBack"];

  Observer* forward_observer = [[Observer alloc] init];
  [forward_observer setObservedObject:web_view_ keyPath:@"canGoForward"];

  ASSERT_FALSE(back_observer.lastValue);
  ASSERT_FALSE(forward_observer.lastValue);

  // Define pages in reverse order so the links can reference the "next" page.
  GURL page_3_url = GetUrlForPageWithTitle("Page 3");

  std::string page_2_html =
      "<a id='link_2' href='" + page_3_url.spec() + "'>Link 2</a>";
  GURL page_2_url = GetUrlForPageWithHtmlBody(page_2_html);

  std::string page_1_html =
      "<a id='link_1' href='" + page_2_url.spec() + "'>Link 1</a>";
  GURL page_1_url = GetUrlForPageWithHtmlBody(page_1_html);

  ASSERT_TRUE(test::LoadUrl(web_view_, net::NSURLWithGURL(page_1_url)));
  // Loading initial URL should not affect back/forward navigation state.
  EXPECT_FALSE([back_observer.lastValue boolValue]);
  EXPECT_FALSE([forward_observer.lastValue boolValue]);

  // Navigate to page 2.
  EXPECT_TRUE(test::TapWebViewElementWithId(web_view_, @"link_1"));
  ASSERT_TRUE(test::WaitForWebViewLoadCompletionOrTimeout(web_view_));
  EXPECT_TRUE([back_observer.lastValue boolValue]);
  EXPECT_FALSE([forward_observer.lastValue boolValue]);

  // Navigate back to page 1.
  [web_view_ goBack];
  ASSERT_TRUE(test::WaitForWebViewLoadCompletionOrTimeout(web_view_));
  EXPECT_FALSE([back_observer.lastValue boolValue]);
  EXPECT_TRUE([forward_observer.lastValue boolValue]);

  // Navigate forward to page 2.
  [web_view_ goForward];
  ASSERT_TRUE(test::WaitForWebViewLoadCompletionOrTimeout(web_view_));
  EXPECT_TRUE([back_observer.lastValue boolValue]);
  EXPECT_FALSE([forward_observer.lastValue boolValue]);

  // Navigate to page 3.
  EXPECT_TRUE(test::TapWebViewElementWithId(web_view_, @"link_2"));
  ASSERT_TRUE(test::WaitForWebViewLoadCompletionOrTimeout(web_view_));
  EXPECT_TRUE([back_observer.lastValue boolValue]);
  EXPECT_FALSE([forward_observer.lastValue boolValue]);

  // Navigate back to page 2.
  [web_view_ goBack];
  ASSERT_TRUE(test::WaitForWebViewLoadCompletionOrTimeout(web_view_));
  EXPECT_TRUE([back_observer.lastValue boolValue]);
  EXPECT_TRUE([forward_observer.lastValue boolValue]);
}

// Tests that CWVWebView correctly reports current |title|.
TEST_F(WebViewKvoTest, Title) {
  Observer* observer = [[Observer alloc] init];
  [observer setObservedObject:web_view_ keyPath:@"title"];

  NSString* page_2_title = @"Page 2";
  GURL page_2_url =
      GetUrlForPageWithTitle(base::SysNSStringToUTF8(page_2_title));

  NSString* page_1_title = @"Page 1";
  std::string page_1_html = base::StringPrintf(
      "<a id='link_1' href='%s'>Link 1</a>", page_2_url.spec().c_str());
  GURL page_1_url = GetUrlForPageWithTitleAndBody(
      base::SysNSStringToUTF8(page_1_title), page_1_html);

  ASSERT_TRUE(test::LoadUrl(web_view_, net::NSURLWithGURL(page_1_url)));
  EXPECT_NSEQ(page_1_title, observer.lastValue);

  // Navigate to page 2.
  EXPECT_TRUE(test::TapWebViewElementWithId(web_view_, @"link_1"));
  ASSERT_TRUE(test::WaitForWebViewLoadCompletionOrTimeout(web_view_));
  EXPECT_NSEQ(page_2_title, observer.lastValue);

  // Navigate back to page 1.
  [web_view_ goBack];
  ASSERT_TRUE(test::WaitForWebViewLoadCompletionOrTimeout(web_view_));
  EXPECT_NSEQ(page_1_title, observer.lastValue);
}

// Tests that CWVWebView correctly reports |isLoading| value.
TEST_F(WebViewKvoTest, Loading) {
  Observer* observer = [[Observer alloc] init];
  [observer setObservedObject:web_view_ keyPath:@"loading"];

  GURL page_2_url = GetUrlForPageWithTitle("Page 2");

  std::string page_1_html = base::StringPrintf(
      "<a id='link_1' href='%s'>Link 1</a>", page_2_url.spec().c_str());
  GURL page_1_url = GetUrlForPageWithTitleAndBody("Page 1", page_1_html);

  ASSERT_TRUE(test::LoadUrl(web_view_, net::NSURLWithGURL(page_1_url)));
  EXPECT_TRUE([observer.previousValue boolValue]);
  EXPECT_FALSE([observer.lastValue boolValue]);

  // Navigate to page 2.
  EXPECT_TRUE(test::TapWebViewElementWithId(web_view_, @"link_1"));
  ASSERT_TRUE(test::WaitForWebViewLoadCompletionOrTimeout(web_view_));
  EXPECT_TRUE([observer.previousValue boolValue]);
  EXPECT_FALSE([observer.lastValue boolValue]);

  // Navigate back to page 1.
  [web_view_ goBack];
  ASSERT_TRUE(test::WaitForWebViewLoadCompletionOrTimeout(web_view_));
  EXPECT_TRUE([observer.previousValue boolValue]);
  EXPECT_FALSE([observer.lastValue boolValue]);
}

// Tests that CWVWebView correctly reports |visibleURL| and |lastCommittedURL|.
TEST_F(WebViewKvoTest, URLs) {
  Observer* last_committed_url_observer = [[Observer alloc] init];
  [last_committed_url_observer setObservedObject:web_view_
                                         keyPath:@"lastCommittedURL"];

  Observer* visible_url_observer = [[Observer alloc] init];
  [visible_url_observer setObservedObject:web_view_ keyPath:@"visibleURL"];

  GURL page_2 = GetUrlForPageWithTitle("Page 2");
  NSURL* page_2_url = net::NSURLWithGURL(page_2);

  std::string page_1_html = base::StringPrintf(
      "<a id='link_1' href='%s'>Link 1</a>", page_2.spec().c_str());
  NSURL* page_1_url =
      net::NSURLWithGURL(GetUrlForPageWithTitleAndBody("Page 1", page_1_html));

  [web_view_ loadRequest:[NSURLRequest requestWithURL:page_1_url]];

  // |visibleURL| will update immediately
  EXPECT_NSEQ(page_1_url, visible_url_observer.lastValue);

  ASSERT_TRUE(test::WaitForWebViewLoadCompletionOrTimeout(web_view_));
  EXPECT_NSEQ(page_1_url, last_committed_url_observer.lastValue);
  EXPECT_NSEQ(page_1_url, visible_url_observer.lastValue);

  // Navigate to page 2.
  EXPECT_TRUE(test::TapWebViewElementWithId(web_view_, @"link_1"));
  ASSERT_TRUE(test::WaitForWebViewLoadCompletionOrTimeout(web_view_));
  EXPECT_NSEQ(page_2_url, last_committed_url_observer.lastValue);
  EXPECT_NSEQ(page_2_url, visible_url_observer.lastValue);

  // Navigate back to page 1.
  [web_view_ goBack];
  ASSERT_TRUE(test::WaitForWebViewLoadCompletionOrTimeout(web_view_));
  EXPECT_NSEQ(page_1_url, last_committed_url_observer.lastValue);
  EXPECT_NSEQ(page_1_url, visible_url_observer.lastValue);
}

}  // namespace ios_web_view
