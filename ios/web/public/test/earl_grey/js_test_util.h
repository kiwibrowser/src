// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_WEB_PUBLIC_TEST_EARL_GREY_JS_TEST_UTIL_H_
#define IOS_WEB_PUBLIC_TEST_EARL_GREY_JS_TEST_UTIL_H_

#import <Foundation/Foundation.h>

#import "ios/web/public/web_state/web_state.h"

namespace web {

// Waits until the Window ID has been injected and the page is thus ready to
// respond to JavaScript injection. Fails with a GREYAssert on timeout or if
// unrecoverable error (such as no web view) occurs.
void WaitUntilWindowIdInjected(WebState* web_state);

// Executes |javascript| on the given |web_state|, and waits until execution is
// completed. If |out_error| is not nil, it is set to the error resulting from
// the execution, if one occurs. The return value is the result of the
// JavaScript execution. If the script execution is timed out, then this method
// fails with a GREYAssert.
id ExecuteJavaScript(WebState* web_state,
                     NSString* javascript,
                     NSError* __autoreleasing* out_error);

// Synchronously returns the result of executed JavaScript on interstitial page
// displayed for |web_state|.
id ExecuteScriptOnInterstitial(WebState* web_state, NSString* script);

}  // namespace web

#endif  // IOS_WEB_PUBLIC_TEST_EARL_GREY_JS_TEST_UTIL_H_
