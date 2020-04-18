// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_NOTIFICATIONS_STUB_ALERT_DISPATCHER_MAC_H_
#define CHROME_BROWSER_NOTIFICATIONS_STUB_ALERT_DISPATCHER_MAC_H_

#import <Foundation/Foundation.h>

#include <string>

#include "chrome/browser/notifications/alert_dispatcher_mac.h"

@interface StubAlertDispatcher : NSObject<AlertDispatcher>

- (void)dispatchNotification:(NSDictionary*)data;

- (void)closeNotificationWithId:(NSString*)notificationId
                  withProfileId:(NSString*)profileId;

- (void)closeAllNotifications;

- (void)
getDisplayedAlertsForProfileId:(NSString*)profileId
                     incognito:(BOOL)incognito
            notificationCenter:(NSUserNotificationCenter*)notificationCenter
                      callback:(GetDisplayedNotificationsCallback)callback;

// Stub specific methods.
- (NSArray*)alerts;

@end

#endif  // CHROME_BROWSER_NOTIFICATIONS_STUB_ALERT_DISPATCHER_MAC_H_
