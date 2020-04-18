// Copyright (c) 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UI_MESSAGE_CENTER_COCOA_POPUP_CONTROLLER_H_
#define UI_MESSAGE_CENTER_COCOA_POPUP_CONTROLLER_H_

#import <Cocoa/Cocoa.h>

#include <string>

#import "base/mac/scoped_nsobject.h"
#import "ui/base/cocoa/tracking_area.h"
#include "ui/message_center/message_center_export.h"

namespace message_center {
class MessageCenter;
class Notification;
}

@class MCNotificationController;
@class MCPopupCollection;

// A window controller that hosts a notification as a popup balloon on the
// user's desktop. The window controller manages its lifetime because the
// popup collection will be destructed when the last popup is closed.
MESSAGE_CENTER_EXPORT
@interface MCPopupController : NSWindowController<NSAnimationDelegate>  {
 @private
  // Global message center. Weak.
  message_center::MessageCenter* messageCenter_;

  // The collection that contains the popup. Weak.
  MCPopupCollection* popupCollection_;

  // The view controller that provide's the popup content view.
  base::scoped_nsobject<MCNotificationController> notificationController_;

  // If the swipe-away gesture received NSEventPhaseEnded.
  BOOL swipeGestureEnded_;

  // The frame of the popup before any swipe animation started. Used to
  // calculate the animating position of the window when swiping away.
  NSRect originalFrame_;

  // Is the popup currently being closed?
  BOOL isClosing_;

#ifndef NDEBUG
  // Has the popup been closed before being dealloc-ed.
  BOOL hasBeenClosed_;
#endif

  // The current bounds of the popup frame if no animation is playing.
  // Otherwise, it is the target bounds of the popup frame.
  NSRect bounds_;

  // Used to play animation when the popup shows, changes bounds and closes.
  base::scoped_nsobject<NSViewAnimation> boundsAnimation_;

  // Used to track the popup for mouse entered and exited events.
  ui::ScopedCrTrackingArea trackingArea_;
}

// Designated initializer.
- (id)initWithNotification:(const message_center::Notification*)notification
             messageCenter:(message_center::MessageCenter*)messageCenter
           popupCollection:(MCPopupCollection*)popupCollection;

// Accessor for the view controller.
- (MCNotificationController*)notificationController;

// Accessor for the notification model object.
- (const message_center::Notification*)notification;

// Gets the notification ID. This string is owned by the NotificationController
// rather than the model object, so it's safe to use after the Notification has
// been deleted.
- (const std::string&)notificationID;

// Shows the window with the sliding animation.
- (void)showWithAnimation:(NSRect)newBounds;

// Closes the window with the fade-out animation.
- (void)closeWithAnimation;

// Tells that the popupCollection_ is gone.
- (void)markPopupCollectionGone;

// Returns the window bounds. This is the target bounds to go to if the bounds
// animation is playing.
- (NSRect)bounds;

// Changes the window bounds with animation.
- (void)setBounds:(NSRect)newBounds;

@end

#endif  // UI_MESSAGE_CENTER_COCOA_POPUP_CONTROLLER_H_
