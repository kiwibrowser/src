// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/public/provider/chrome/browser/mailto/test_mailto_handler_provider.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

TestMailtoHandlerProvider::TestMailtoHandlerProvider() {}

TestMailtoHandlerProvider::~TestMailtoHandlerProvider() {}

NSString* TestMailtoHandlerProvider::MailtoHandlerSettingsTitle() const {
  // Return something other than nil.
  return @"Test Mailto Settings";
}
