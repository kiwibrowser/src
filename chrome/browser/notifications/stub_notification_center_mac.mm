// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "chrome/browser/notifications/stub_notification_center_mac.h"

#include "base/logging.h"
#include "base/mac/scoped_nsobject.h"
#include "chrome/browser/ui/cocoa/notifications/notification_constants_mac.h"

@implementation StubNotificationCenter {
  base::scoped_nsobject<NSMutableArray> banners_;
}

- (instancetype)init {
  if ((self = [super init])) {
    banners_.reset([[NSMutableArray alloc] init]);
  }
  return self;
}

// The default implementation adds some extra checks on what constructors can
// be used. isKindOfClass bypasses all of that.
- (BOOL)isKindOfClass:(Class)cls {
  if ([cls isEqual:NSClassFromString(@"_NSConcreteUserNotificationCenter")]) {
    return YES;
  }
  return [super isKindOfClass:cls];
}

- (void)deliverNotification:(NSUserNotification*)notification {
  [banners_ addObject:notification];
}

- (NSArray*)deliveredNotifications {
  return [[banners_ copy] autorelease];
}

- (void)removeDeliveredNotification:(NSUserNotification*)notification {
  NSString* notificationId = [notification.userInfo
      objectForKey:notification_constants::kNotificationId];
  NSString* profileId = [notification.userInfo
      objectForKey:notification_constants::kNotificationProfileId];
  DCHECK(profileId);
  DCHECK(notificationId);
  for (NSUserNotification* toast in banners_.get()) {
    NSString* toastId =
        [toast.userInfo objectForKey:notification_constants::kNotificationId];
    NSString* persistentProfileId = [toast.userInfo
        objectForKey:notification_constants::kNotificationProfileId];
    if ([toastId isEqualToString:notificationId] &&
        [persistentProfileId isEqualToString:profileId]) {
      [banners_ removeObject:toast];
      break;
    }
  }
}

- (void)removeAllDeliveredNotifications {
  [banners_ removeAllObjects];
}

// Need to provide a nop implementation of setDelegate as it is
// used during the setup of the bridge.
- (void)setDelegate:(id<NSUserNotificationCenterDelegate>)delegate {
}

@end
