// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "chrome/browser/ui/cocoa/notifications/notification_service_delegate.h"

#import <AppKit/AppKit.h>

#include "base/mac/scoped_nsobject.h"
#import "chrome/browser/ui/cocoa/notifications/alert_notification_service.h"
#import "chrome/browser/ui/cocoa/notifications/notification_delivery.h"
#import "chrome/browser/ui/cocoa/notifications/notification_response_builder_mac.h"
#import "chrome/browser/ui/cocoa/notifications/xpc_transaction_handler.h"

@class NSUserNotificationCenter;

@implementation ServiceDelegate {
  // Helper to manage the XPC transaction reference count with respect to
  // still-visible notifications.
  base::scoped_nsobject<XPCTransactionHandler> transactionHandler_;

  // Client connection accepted from the browser process, to which messages
  // are sent in response to notification actions.
  base::scoped_nsobject<NSXPCConnection> connection_;
}

- (instancetype)init {
  if ((self = [super init])) {
    [[NSUserNotificationCenter defaultUserNotificationCenter] setDelegate:self];
    transactionHandler_.reset([[XPCTransactionHandler alloc] init]);
  }
  return self;
}

- (void)dealloc {
  [[NSUserNotificationCenter defaultUserNotificationCenter] setDelegate:nil];
  [super dealloc];
}

- (BOOL)listener:(NSXPCListener*)listener
    shouldAcceptNewConnection:(NSXPCConnection*)newConnection {
  newConnection.exportedInterface =
      [NSXPCInterface interfaceWithProtocol:@protocol(NotificationDelivery)];
  [newConnection.exportedInterface
         setClasses:[NSSet setWithObjects:[NSArray class], [NSData class],
                                          [NSDictionary class], [NSImage class],
                                          [NSNumber class], [NSString class],
                                          nil]
        forSelector:@selector(deliverNotification:)
      argumentIndex:0
            ofReply:NO];

  base::scoped_nsobject<AlertNotificationService> object(
      [[AlertNotificationService alloc]
          initWithTransactionHandler:transactionHandler_]);
  newConnection.exportedObject = object.get();
  newConnection.remoteObjectInterface =
      [NSXPCInterface interfaceWithProtocol:@protocol(NotificationReply)];
  connection_.reset(newConnection, base::scoped_policy::RETAIN);
  [newConnection resume];

  return YES;
}

// NSUserNotificationCenterDelegate:
- (void)userNotificationCenter:(NSUserNotificationCenter*)center
       didActivateNotification:(NSUserNotification*)notification {
  NSDictionary* response =
      [NotificationResponseBuilder buildDictionary:notification];
  [[connection_ remoteObjectProxy] notificationClick:response];
}

// _NSUserNotificationCenterDelegatePrivate:
- (void)userNotificationCenter:(NSUserNotificationCenter*)center
               didDismissAlert:(NSUserNotification*)notification {
  NSDictionary* response =
      [NotificationResponseBuilder buildDictionary:notification];
  [[connection_ remoteObjectProxy] notificationClick:response];
  [transactionHandler_ closeTransactionIfNeeded];
}

@end
