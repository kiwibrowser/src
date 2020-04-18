// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import <ChromeWebView/ChromeWebView.h>

#import "ios/web_view/test/web_view_int_test.h"
#import "ios/web_view/test/web_view_test_util.h"
#include "testing/gtest_mac.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

namespace ios_web_view {

// Tests encodeRestorableStateWithCoder: and decodeRestorableStateWithCoder:
// methods.
typedef ios_web_view::WebViewIntTest WebViewRestorableStateTest;
TEST_F(WebViewRestorableStateTest, EncodeDecode) {
  // Load 2 URLs to create non-default state.
  ASSERT_FALSE([web_view_ lastCommittedURL]);
  ASSERT_FALSE([web_view_ visibleURL]);
  ASSERT_FALSE([web_view_ canGoBack]);
  ASSERT_FALSE([web_view_ canGoForward]);
  ASSERT_TRUE(test::LoadUrl(web_view_, [NSURL URLWithString:@"about:newtab"]));
  ASSERT_NSEQ(@"about:newtab", [web_view_ lastCommittedURL].absoluteString);
  ASSERT_NSEQ(@"about:newtab", [web_view_ visibleURL].absoluteString);
  ASSERT_TRUE(test::LoadUrl(web_view_, [NSURL URLWithString:@"about:blank"]));
  ASSERT_NSEQ(@"about:blank", [web_view_ lastCommittedURL].absoluteString);
  ASSERT_NSEQ(@"about:blank", [web_view_ visibleURL].absoluteString);
  ASSERT_TRUE([web_view_ canGoBack]);
  ASSERT_FALSE([web_view_ canGoForward]);

  // Create second web view and restore its state from the first web view.
  CWVWebView* restored_web_view = test::CreateWebView();
  test::CopyWebViewState(web_view_, restored_web_view);

  // Verify that the state has been restored correctly.
  EXPECT_NSEQ(@"about:blank",
              [restored_web_view lastCommittedURL].absoluteString);
  EXPECT_NSEQ(@"about:blank", [restored_web_view visibleURL].absoluteString);
  EXPECT_TRUE([web_view_ canGoBack]);
  EXPECT_FALSE([web_view_ canGoForward]);
}

}  // namespace ios_web_view
