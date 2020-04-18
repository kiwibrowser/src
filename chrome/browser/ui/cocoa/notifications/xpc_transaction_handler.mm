// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import <AppKit/AppKit.h>
#import <Foundation/Foundation.h>

#import "chrome/browser/ui/cocoa/notifications/xpc_transaction_handler.h"

@class NSUserNotificationCenter;

@implementation XPCTransactionHandler {
  bool transactionOpen_;
}

- (instancetype)init {
  if ((self = [super init])) {
    transactionOpen_ = false;
  }
  return self;
}

- (void)openTransactionIfNeeded {
  @synchronized(self) {
    if (transactionOpen_) {
      return;
    }
    xpc_transaction_begin();
    transactionOpen_ = true;
  }
}

- (void)closeTransactionIfNeeded {
  @synchronized(self) {
    NSUserNotificationCenter* notificationCenter =
        [NSUserNotificationCenter defaultUserNotificationCenter];
    NSUInteger showing = [[notificationCenter deliveredNotifications] count];
    if (showing == 0 && transactionOpen_) {
      xpc_transaction_end();
      transactionOpen_ = false;
    }
  }
}
@end
