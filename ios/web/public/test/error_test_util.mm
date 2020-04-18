// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/web/public/test/error_test_util.h"

#import "ios/web/web_state/error_translation_util.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

namespace web {
namespace testing {

NSError* CreateTestNetError(NSError* error) {
  return NetErrorFromError(error);
}

}  // namespace testing
}  // namespace web
