// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_WEB_PUBLIC_TEST_FAKES_CRW_TEST_JS_INJECTION_RECEIVER_H_
#define IOS_WEB_PUBLIC_TEST_FAKES_CRW_TEST_JS_INJECTION_RECEIVER_H_

#import "ios/web/public/web_state/js/crw_js_injection_receiver.h"

// TestInjectionReceiver is used for tests.
// It uses a bare UIWebView as backend for javascript evaluation.
@interface CRWTestJSInjectionReceiver : CRWJSInjectionReceiver
@end

#endif  // IOS_WEB_PUBLIC_TEST_FAKES_CRW_TEST_JS_INJECTION_RECEIVER_H_
