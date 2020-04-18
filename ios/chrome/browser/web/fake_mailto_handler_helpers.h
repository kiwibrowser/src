// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CHROME_BROWSER_WEB_FAKE_MAILTO_HANLDERS_H_
#define IOS_CHROME_BROWSER_WEB_FAKE_MAILTO_HANLDERS_H_

#import "ios/chrome/browser/web/mailto_handler.h"
#import "ios/chrome/browser/web/mailto_handler_gmail.h"
#import "ios/chrome/browser/web/mailto_handler_manager.h"

// Test fixtures for faking MailtoHandlerGmail objects that reports whether
// Gmail app as installed or not.

// Fakes Gmail handler where Gmail app is not installed.
@interface FakeMailtoHandlerGmailNotInstalled : MailtoHandlerGmail
@end

// Fakes Gmail handler where Gmail app is installed.
@interface FakeMailtoHandlerGmailInstalled : MailtoHandlerGmail
@end

// Fake mailto: handler
@interface FakeMailtoHandlerForTesting : MailtoHandler
@end

// An observer object that counts and reports the number of times it has been
// called by the MailtoHandlerManager object.
@interface CountingMailtoHandlerManagerObserver
    : NSObject<MailtoHandlerManagerObserver>
// Returns the number of times that observer has been called.
@property(nonatomic, readonly) int changeCount;
@end

#endif  // IOS_CHROME_BROWSER_WEB_FAKE_MAILTO_HANLDERS_H_
