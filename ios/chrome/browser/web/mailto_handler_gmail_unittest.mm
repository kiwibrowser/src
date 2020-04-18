// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ios/chrome/browser/web/mailto_handler_gmail.h"

#include "testing/gtest/include/gtest/gtest.h"
#include "testing/gtest_mac.h"
#include "testing/platform_test.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

using MailtoHandlerGmailTest = PlatformTest;

TEST_F(MailtoHandlerGmailTest, TestConstructor) {
  MailtoHandlerGmail* handler = [[MailtoHandlerGmail alloc] init];
  EXPECT_NSEQ(@"Gmail", [handler appName]);
  EXPECT_NSEQ(@"422689480", [handler appStoreID]);
  EXPECT_NSEQ(@"googlegmail:/co?", [handler beginningScheme]);
}
