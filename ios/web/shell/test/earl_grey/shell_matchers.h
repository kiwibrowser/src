// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_WEB_SHELL_TEST_EARL_GREY_SHELL_MATCHERS_H_
#define IOS_WEB_SHELL_TEST_EARL_GREY_SHELL_MATCHERS_H_

#include <string>

#import <EarlGrey/EarlGrey.h>

namespace web {

// Matcher for the WKWebView.
id<GREYMatcher> WebView();

// Matcher for WKWebView's scroll view.
id<GREYMatcher> WebViewScrollView();

// Matcher for web shell address field text property equal to |text|.
id<GREYMatcher> AddressFieldText(std::string text);

// Matcher for back button in web shell.
id<GREYMatcher> BackButton();

// Matcher for forward button in web shell.
id<GREYMatcher> ForwardButton();

// Matcher for address field in web shell.
id<GREYMatcher> AddressField();

}  // namespace web

#endif  // IOS_WEB_SHELL_TEST_EARL_GREY_SHELL_MATCHERS_H_
