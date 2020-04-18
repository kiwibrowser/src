// Copyright (c) 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UI_MESSAGE_CENTER_COCOA_POPUP_COLLECTION_H_
#define UI_MESSAGE_CENTER_COCOA_POPUP_COLLECTION_H_

#import <Cocoa/Cocoa.h>

#include <memory>
#include <set>

#include "base/mac/scoped_block.h"
#import "base/mac/scoped_nsobject.h"
#include "ui/message_center/message_center_export.h"

namespace message_center {
class MessageCenter;
class MessageCenterObserver;
}

namespace message_center {
typedef void(^AnimationEndedCallback)();
}

// A popup collection interfaces with the MessageCenter as an observer. It will
// arrange notifications on the screen as popups, starting in the upper-right
// corner, going to the bottom of the screen. This class maintains ownership of
// the Cocoa controllers and windows of the notifications.
MESSAGE_CENTER_EXPORT
@interface MCPopupCollection : NSObject {
 @private
  // The message center that is responsible for the notifications. Weak, global.
  message_center::MessageCenter* messageCenter_;

  // MessageCenterObserver implementation.
  std::unique_ptr<message_center::MessageCenterObserver> observer_;

  // Array of all on-screen popup notifications.
  base::scoped_nsobject<NSMutableArray> popups_;

  // Array of all on-screen popup notifications that are being faded out
  // for removal.
  base::scoped_nsobject<NSMutableArray> popupsBeingRemoved_;

  // For testing only. If not a zero rect, this is the screen size to use
  // for laying out popups.
  NSRect testingScreenFrame_;

  // The duration of the popup animation, in the number of seconds.
  NSTimeInterval popupAnimationDuration_;

  // Set of notification IDs for those popups to be updated when all existing
  // animations end.
  std::set<std::string> pendingUpdateNotificationIDs_;

  // Set of notification IDs for those popups to be closed when all existing
  // animations end.
  std::set<std::string> pendingRemoveNotificationIDs_;

  // Set of notification IDs for those popups that are being animated due to
  // showing, bounds change or closing.
  std::set<std::string> animatingNotificationIDs_;

  // For testing only. If set, the callback will be called when the animation
  // ends.
  base::mac::ScopedBlock<message_center::AnimationEndedCallback>
      testingAnimationEndedCallback_;
}

// Designated initializer that construct an instance to observe |messageCenter|.
- (id)initWithMessageCenter:(message_center::MessageCenter*)messageCenter;

// Returns true if an animation is being played.
- (BOOL)isAnimating;

// Returns the duration of the popup animation.
- (NSTimeInterval)popupAnimationDuration;

// Called when the animation of a popup ends.
- (void)onPopupAnimationEnded:(const std::string&)notificationID;

@end

@interface MCPopupCollection (ExposedForTesting)
- (NSArray*)popups;

// Setter for the testingScreenFrame_.
- (void)setScreenFrame:(NSRect)frame;

// Setter for changing the animation duration. The testing code could set it
// to a very small value to expedite the test running.
- (void)setAnimationDuration:(NSTimeInterval)duration;

// Setter for testingAnimationEndedCallback_. The testing code could set it
// to get called back when the animation ends.
- (void)setAnimationEndedCallback:
    (message_center::AnimationEndedCallback)callback;
@end

#endif  // UI_MESSAGE_CENTER_COCOA_POPUP_COLLECTION_H_
