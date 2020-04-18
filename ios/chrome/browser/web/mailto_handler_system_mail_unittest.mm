// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ios/chrome/browser/web/mailto_handler_system_mail.h"

#include "testing/gtest/include/gtest/gtest.h"
#include "testing/gtest_mac.h"
#include "testing/platform_test.h"
#include "url/gurl.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

using MailtoHandlerSystemMailTest = PlatformTest;

TEST_F(MailtoHandlerSystemMailTest, TestConstructor) {
  MailtoHandler* handler = [[MailtoHandlerSystemMail alloc] init];
  EXPECT_TRUE(handler);
  EXPECT_NSEQ(@"Mail", [handler appName]);
  EXPECT_GT([[handler appStoreID] length], 0U);
  EXPECT_TRUE([handler isAvailable]);
}

TEST_F(MailtoHandlerSystemMailTest, TestRewrite) {
  MailtoHandler* handler = [[MailtoHandlerSystemMail alloc] init];
  NSString* result = [handler rewriteMailtoURL:GURL("mailto:user@domain.com")];
  EXPECT_NSEQ(@"mailto:user@domain.com", result);
  result = [handler rewriteMailtoURL:GURL("http://www.google.com")];
  EXPECT_FALSE(result);
}
