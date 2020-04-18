// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_COCOA_NOTIFICATIONS_NOTIFICATION_RESPONSE_BUILDER_MAC_H_
#define CHROME_BROWSER_UI_COCOA_NOTIFICATIONS_NOTIFICATION_RESPONSE_BUILDER_MAC_H_

#import <Foundation/Foundation.h>

@class NSUserNotification;

// Make sure this Obj-C enum is kept in sync with the
// NotificationCommon::Operation enum.
// The latter cannot be reused because the XPC service is not aware of
// PlatformNotificationCenter.
enum NotificationOperation {
  NOTIFICATION_CLICK = 0,
  NOTIFICATION_CLOSE = 1,
  NOTIFICATION_DISABLE_PERMISSION = 2,
  NOTIFICATION_SETTINGS = 3,
  NOTIFICATION_OPERATION_MAX = NOTIFICATION_SETTINGS
};

// Provides a marshallable way for storing the information related to a
// notification response action, clicking on it, clicking on a button etc.
@interface NotificationResponseBuilder : NSObject

+ (NSDictionary*)buildDictionary:(NSUserNotification*)notification;

@end

#endif  // CHROME_BROWSER_UI_COCOA_NOTIFICATIONS_NOTIFICATION_RESPONSE_BUILDER_MAC_H_
