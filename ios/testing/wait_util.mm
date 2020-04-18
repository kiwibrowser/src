// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/testing/wait_util.h"

#import "base/test/ios/wait_util.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

namespace testing {

const NSTimeInterval kSpinDelaySeconds = 0.01;
const NSTimeInterval kWaitForJSCompletionTimeout = 4.0;
const NSTimeInterval kWaitForUIElementTimeout = 4.0;
const NSTimeInterval kWaitForDownloadTimeout = 10.0;
const NSTimeInterval kWaitForPageLoadTimeout = 10.0;
const NSTimeInterval kWaitForActionTimeout = 10.0;
const NSTimeInterval kWaitForCookiesTimeout = 4.0;
const NSTimeInterval kWaitForFileOperationTimeout = 2.0;

bool WaitUntilConditionOrTimeout(NSTimeInterval timeout,
                                 ConditionBlock condition) {
  NSDate* deadline = [NSDate dateWithTimeIntervalSinceNow:timeout];
  bool success = condition();
  while (!success && [[NSDate date] compare:deadline] != NSOrderedDescending) {
    base::test::ios::SpinRunLoopWithMaxDelay(
        base::TimeDelta::FromSecondsD(testing::kSpinDelaySeconds));
    success = condition();
  }
  return success;
}

}  // namespace testing
