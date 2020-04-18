// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CHROME_BROWSER_UI_METRICS_METRICS_TEST_UTIL_H_
#define IOS_CHROME_BROWSER_UI_METRICS_METRICS_TEST_UTIL_H_

#import <Foundation/Foundation.h>

// Constructs an NSInvocation for an instance method defined by
// |selector| in |protocol|. |isRequiredMethod| indicates whether |selector|
// is a required method for |protocol|.
NSInvocation* GetInvocationForProtocolInstanceMethod(Protocol* protocol,
                                                     SEL selector,
                                                     BOOL isRequiredMethod);

#endif  // IOS_CHROME_BROWSER_UI_METRICS_METRICS_TEST_UTIL_H_
