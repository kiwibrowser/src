// Copyright (c) 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UI_MESSAGE_CENTER_COCOA_NOTIFICATION_CONTROLLER_H_
#define UI_MESSAGE_CENTER_COCOA_NOTIFICATION_CONTROLLER_H_

#import <Cocoa/Cocoa.h>

#include <string>

#import "base/mac/scoped_nsobject.h"
#include "ui/message_center/message_center_export.h"

namespace message_center {
class MessageCenter;
class Notification;
}

@class HoverImageButton;

namespace message_center {
// A struct that can hold all the temporary frames
// created when adjusting a view
struct NotificationLayoutParams {
  NSRect rootFrame;
  NSRect titleFrame;
  NSRect messageFrame;
  NSRect contextMessageFrame;
  NSRect settingsButtonFrame;
  NSRect listFrame;
  NSRect progressBarFrame;
};
}

// The base view controller class for notifications. A notification at minimum
// has an image, title, body, and close button. This controller can be used as
// the content for both a popup bubble and a view in the notification tray.
MESSAGE_CENTER_EXPORT
@interface MCNotificationController : NSViewController {
 @protected
  // The message object. Weak.
  const message_center::Notification* notification_;

  // A copy of the notification ID.
  std::string notificationID_;

  // Controller of the notifications, where action messages are forwarded. Weak.
  message_center::MessageCenter* messageCenter_;

  // The button that invokes |-close:|, in the upper-right corner.
  base::scoped_nsobject<HoverImageButton> closeButton_;

  // The button that invokes |-settingsClicked:|, in the bottom right corner of
  // the context message.
  base::scoped_nsobject<HoverImageButton> settingsButton_;

  // The small icon associated with the notification, on the bottom right.
  base::scoped_nsobject<NSImageView> smallImage_;

  // The large icon associated with the notification, on the left side.
  base::scoped_nsobject<NSImageView> icon_;

  // The title of the message.
  base::scoped_nsobject<NSTextView> title_;

  // Body text of the message. Hidden for list notifications.
  base::scoped_nsobject<NSTextView> message_;

  // Context-giving text of the message.  Alternate font used to distinguish it.
  base::scoped_nsobject<NSTextView> contextMessage_;

  // Container for optional list view that contains multiple items.
  base::scoped_nsobject<NSView> listView_;

  // Container for optional progress bar view.
  base::scoped_nsobject<NSProgressIndicator> progressBarView_;

  // Container for optional items at the bottom of the notification.
  base::scoped_nsobject<NSView> bottomView_;
}

// Creates a new controller for a given notification.
- (id)initWithNotification:(const message_center::Notification*)notification
    messageCenter:(message_center::MessageCenter*)messageCenter;

// If the model object changes, this method will update the views to reflect
// the new model object. Returns the updated frame of the notification.
- (NSRect)updateNotification:(const message_center::Notification*)notification;

// Action for clicking on the notification's |closeButton_|.
- (void)close:(id)sender;

// Action for clicking on the notification's |settingsButton_|.
- (void)settingsClicked:(id)sender;

// Accessor for the notification.
- (const message_center::Notification*)notification;

// Gets the notification ID. This string is owned by the NotificationController
// rather than the model object, so it's safe to use after the Notification has
// been deleted.
- (const std::string&)notificationID;

// Called when the user clicks within the notification view.
- (void)notificationClicked;

// Adjust the position and height of all the internal frames by |delta|.
- (void)adjustFrameHeight:(message_center::NotificationLayoutParams*)frames
                    delta:(CGFloat)delta;

@end

@interface MCNotificationController (TestingInterface)
- (NSImageView*)smallImageView;
- (NSImageView*)iconView;
@end

#endif  // UI_MESSAGE_CENTER_COCOA_NOTIFICATION_CONTROLLER_H_
