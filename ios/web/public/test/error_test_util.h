// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_WEB_PUBLIC_TEST_ERROR_TEST_UTIL_H_
#define IOS_WEB_PUBLIC_TEST_ERROR_TEST_UTIL_H_

@class NSError;

namespace web {
namespace testing {

// Creates Chrome specific error from a regular NSError. Returned error has the
// same format and structure as errors provided in ios/web callbacks.
NSError* CreateTestNetError(NSError* error);

}  // namespace testing
}  // namespace web

#endif  // IOS_WEB_PUBLIC_TEST_ERROR_TEST_UTIL_H_
