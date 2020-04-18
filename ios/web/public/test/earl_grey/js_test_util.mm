// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/web/public/test/earl_grey/js_test_util.h"

#import <EarlGrey/EarlGrey.h>
#import <WebKit/WebKit.h>

#include "base/timer/elapsed_timer.h"
#import "ios/testing/wait_util.h"
#import "ios/web/interstitials/web_interstitial_impl.h"
#import "ios/web/public/web_state/js/crw_js_injection_receiver.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

using testing::kWaitForJSCompletionTimeout;
using testing::WaitUntilConditionOrTimeout;

namespace web {

// Evaluates the given |script| on |interstitial|.
void ExecuteScriptForTesting(web::WebInterstitialImpl* interstitial,
                             NSString* script,
                             web::JavaScriptResultBlock handler) {
  DCHECK(interstitial);
  interstitial->ExecuteJavaScript(script, handler);
}

void WaitUntilWindowIdInjected(WebState* web_state) {
  bool is_window_id_injected = false;
  bool is_timeout = false;
  bool is_unrecoverable_error = false;

  base::ElapsedTimer timer;
  base::TimeDelta timeout =
      base::TimeDelta::FromSeconds(kWaitForJSCompletionTimeout);

  // Keep polling until either the JavaScript execution returns with expected
  // value (indicating that Window ID is set), the timeout occurs, or an
  // unrecoverable error occurs.
  while (!is_window_id_injected && !is_timeout && !is_unrecoverable_error) {
    NSError* error = nil;
    id result = ExecuteJavaScript(web_state, @"0", &error);
    if (error) {
      is_unrecoverable_error = ![error.domain isEqual:WKErrorDomain] ||
                               error.code != WKErrorJavaScriptExceptionOccurred;
    } else {
      is_window_id_injected = [result isEqual:@0];
    }
    is_timeout = timeout < timer.Elapsed();
  }
  GREYAssertFalse(is_timeout, @"windowID injection timed out");
  GREYAssertFalse(is_unrecoverable_error, @"script execution error");
}

id ExecuteJavaScript(WebState* web_state,
                     NSString* javascript,
                     NSError* __autoreleasing* out_error) {
  __block bool did_complete = false;
  __block id result = nil;
  CRWJSInjectionReceiver* receiver = web_state->GetJSInjectionReceiver();
  [receiver executeJavaScript:javascript
            completionHandler:^(id value, NSError* error) {
              did_complete = true;
              result = [value copy];
              if (out_error)
                *out_error = [error copy];
            }];

  // Wait for completion.
  BOOL succeeded = WaitUntilConditionOrTimeout(kWaitForJSCompletionTimeout, ^{
    return did_complete;
  });
  GREYAssert(succeeded, @"Script execution timed out");

  return result;
}

id ExecuteScriptOnInterstitial(WebState* web_state, NSString* script) {
  web::WebInterstitialImpl* interstitial =
      static_cast<web::WebInterstitialImpl*>(web_state->GetWebInterstitial());

  __block id script_result = nil;
  __block bool did_finish = false;
  web::ExecuteScriptForTesting(interstitial, script, ^(id result, NSError*) {
    script_result = [result copy];
    did_finish = true;
  });
  BOOL succeeded = WaitUntilConditionOrTimeout(kWaitForJSCompletionTimeout, ^{
    return did_finish;
  });
  GREYAssert(succeeded, @"Script execution timed out");
  return script_result;
}

}  // namespace web
