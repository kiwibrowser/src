// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/chrome/browser/web/mailto_handler_manager.h"

#import "ios/chrome/browser/web/fake_mailto_handler_helpers.h"
#import "ios/chrome/browser/web/mailto_handler_system_mail.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "testing/gtest_mac.h"
#include "testing/platform_test.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

class MailtoHandlerManagerTest : public PlatformTest {
 public:
  MailtoHandlerManagerTest() {
    [[NSUserDefaults standardUserDefaults]
        removeObjectForKey:kMailtoHandlerManagerUserDefaultsKey];
  }
};

// Tests that a new instance has expected properties and behaviors.
TEST_F(MailtoHandlerManagerTest, TestStandardInstance) {
  MailtoHandlerManager* manager =
      [MailtoHandlerManager mailtoHandlerManagerWithStandardHandlers];
  EXPECT_TRUE(manager);

  NSArray<MailtoHandler*>* handlers = [manager defaultHandlers];
  EXPECT_GE([handlers count], 1U);
  for (MailtoHandler* handler in handlers) {
    ASSERT_TRUE(handler);
    NSString* appStoreID = [handler appStoreID];
    NSString* expectedDefaultAppID = [handler isAvailable]
                                         ? appStoreID
                                         : [MailtoHandlerManager systemMailApp];
    [manager setDefaultHandlerID:appStoreID];
    EXPECT_NSEQ(expectedDefaultAppID, [manager defaultHandlerID]);
    MailtoHandler* foundHandler = [manager defaultHandlerByID:appStoreID];
    EXPECT_NSEQ(handler, foundHandler);
  }
}

// If Gmail is not installed, rewriter defaults to system Mail app.
TEST_F(MailtoHandlerManagerTest, TestNoGmailInstalled) {
  MailtoHandlerManager* manager = [[MailtoHandlerManager alloc] init];
  [manager setDefaultHandlers:@[
    [[MailtoHandlerSystemMail alloc] init],
    [[FakeMailtoHandlerGmailNotInstalled alloc] init]
  ]];
  EXPECT_NSEQ([MailtoHandlerManager systemMailApp], [manager defaultHandlerID]);
}

// If Gmail is installed but user has not made a choice, there is no default
// mail app.
TEST_F(MailtoHandlerManagerTest, TestWithGmailChoiceNotMade) {
  MailtoHandlerManager* manager = [[MailtoHandlerManager alloc] init];
  [manager setDefaultHandlers:@[
    [[MailtoHandlerSystemMail alloc] init],
    [[FakeMailtoHandlerGmailInstalled alloc] init]
  ]];
  EXPECT_FALSE([manager defaultHandlerID]);
}

// Tests that it is possible to unset the default handler.
TEST_F(MailtoHandlerManagerTest, TestUnsetDefaultHandler) {
  MailtoHandlerManager* manager = [[MailtoHandlerManager alloc] init];
  MailtoHandler* gmailInstalled =
      [[FakeMailtoHandlerGmailInstalled alloc] init];
  MailtoHandler* systemMail = [[MailtoHandlerSystemMail alloc] init];
  [manager setDefaultHandlers:@[ systemMail, gmailInstalled ]];
  [manager setDefaultHandlerID:[systemMail appStoreID]];
  EXPECT_NSEQ([systemMail appStoreID], [manager defaultHandlerID]);
  [manager setDefaultHandlerID:nil];
  EXPECT_FALSE([manager defaultHandlerID]);
}

// If Gmail was installed and user has made a choice, then Gmail is uninstalled.
// The default returns to system Mail app.
TEST_F(MailtoHandlerManagerTest, TestWithGmailUninstalled) {
  MailtoHandlerManager* manager = [[MailtoHandlerManager alloc] init];
  MailtoHandler* systemMailHandler = [[MailtoHandlerSystemMail alloc] init];
  MailtoHandler* fakeGmailHandler =
      [[FakeMailtoHandlerGmailInstalled alloc] init];
  [manager setDefaultHandlers:@[ systemMailHandler, fakeGmailHandler ]];
  [manager setDefaultHandlerID:[fakeGmailHandler appStoreID]];
  EXPECT_NSEQ([fakeGmailHandler appStoreID], [manager defaultHandlerID]);

  manager = [[MailtoHandlerManager alloc] init];
  fakeGmailHandler = [[FakeMailtoHandlerGmailNotInstalled alloc] init];
  [manager setDefaultHandlers:@[ systemMailHandler, fakeGmailHandler ]];
  EXPECT_NSEQ([MailtoHandlerManager systemMailApp], [manager defaultHandlerID]);
}

// If Gmail is installed but system Mail app has been chosen by user as the
// default mail handler app. Then Gmail is uninstalled. User's choice of system
// Mail app remains unchanged and will persist through a re-installation of
// Gmail.
TEST_F(MailtoHandlerManagerTest, TestSystemMailAppChosenSurviveGmailUninstall) {
  // Initial state of system Mail app explicitly chosen.
  MailtoHandlerManager* manager = [[MailtoHandlerManager alloc] init];
  MailtoHandler* systemMailHandler = [[MailtoHandlerSystemMail alloc] init];
  [manager setDefaultHandlers:@[
    systemMailHandler, [[FakeMailtoHandlerGmailInstalled alloc] init]
  ]];
  [manager setDefaultHandlerID:[systemMailHandler appStoreID]];
  EXPECT_NSEQ([systemMailHandler appStoreID], [manager defaultHandlerID]);

  // Gmail is installed.
  manager = [[MailtoHandlerManager alloc] init];
  [manager setDefaultHandlers:@[
    systemMailHandler, [[FakeMailtoHandlerGmailNotInstalled alloc] init]
  ]];
  EXPECT_NSEQ([systemMailHandler appStoreID], [manager defaultHandlerID]);

  // Gmail is installed again.
  manager = [[MailtoHandlerManager alloc] init];
  [manager setDefaultHandlers:@[
    systemMailHandler, [[FakeMailtoHandlerGmailInstalled alloc] init]
  ]];
  EXPECT_NSEQ([systemMailHandler appStoreID], [manager defaultHandlerID]);
}
