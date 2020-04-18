// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/chrome/browser/web/mailto_handler_inbox.h"

#include "testing/gtest/include/gtest/gtest.h"
#include "testing/gtest_mac.h"
#include "testing/platform_test.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

using MailtoHandlerInboxTest = PlatformTest;

TEST_F(MailtoHandlerInboxTest, TestConstructor) {
  MailtoHandlerInbox* handler = [[MailtoHandlerInbox alloc] init];
  EXPECT_NSEQ(@"Inbox by Gmail", [handler appName]);
  EXPECT_NSEQ(@"905060486", [handler appStoreID]);
  EXPECT_NSEQ(@"inbox-gmail://co?", [handler beginningScheme]);
}
