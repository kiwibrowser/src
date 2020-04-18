// Copyright (c) 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ui/message_center/cocoa/popup_controller.h"

#include <cmath>

#import "base/mac/foundation_util.h"
#import "base/mac/sdk_forward_declarations.h"
#import "ui/base/cocoa/window_size_constants.h"
#import "ui/message_center/cocoa/notification_controller.h"
#import "ui/message_center/cocoa/popup_collection.h"
#include "ui/message_center/message_center.h"

////////////////////////////////////////////////////////////////////////////////

@interface MCPopupController (Private)
- (void)notificationSwipeStarted;
- (void)notificationSwipeMoved:(CGFloat)amount;
- (void)notificationSwipeEnded:(BOOL)ended complete:(BOOL)isComplete;

// This setter for |boundsAnimation_| also cleans up the state of the previous
// |boundsAnimation_|.
- (void)setBoundsAnimation:(NSViewAnimation*)animation;

// Constructs an NSViewAnimation from |dictionary|, which should be a view
// animation dictionary.
- (NSViewAnimation*)animationWithDictionary:(NSDictionary*)dictionary;
@end

// Window Subclass /////////////////////////////////////////////////////////////

@interface MCPopupWindow : NSPanel {
  // The cumulative X and Y scrollingDeltas since the -scrollWheel: event began.
  NSPoint totalScrollDelta_;
}
@end

@implementation MCPopupWindow

- (void)scrollWheel:(NSEvent*)event {
  // Gesture swiping only exists on 10.7+.
  if (![event respondsToSelector:@selector(phase)])
    return;

  NSEventPhase phase = [event phase];
  BOOL shouldTrackSwipe = NO;

  if (phase == NSEventPhaseBegan) {
    totalScrollDelta_ = NSZeroPoint;
  } else if (phase == NSEventPhaseChanged) {
    shouldTrackSwipe = YES;
    totalScrollDelta_.x += [event scrollingDeltaX];
    totalScrollDelta_.y += [event scrollingDeltaY];
  }

  // Only allow horizontal scrolling.
  if (std::abs(totalScrollDelta_.x) < std::abs(totalScrollDelta_.y))
    return;

  if (shouldTrackSwipe) {
    MCPopupController* controller =
        base::mac::ObjCCastStrict<MCPopupController>([self windowController]);
    BOOL directionInverted = [event isDirectionInvertedFromDevice];

    auto handler = ^(CGFloat gestureAmount, NSEventPhase phase,
                     BOOL isComplete, BOOL* stop) {
        // The swipe direction should match the direction the user's fingers
        // are moving, not the interpreted scroll direction.
        if (directionInverted)
          gestureAmount *= -1;

        if (phase == NSEventPhaseBegan) {
          [controller notificationSwipeStarted];
          return;
        }

        [controller notificationSwipeMoved:gestureAmount];

        BOOL ended = phase == NSEventPhaseEnded;
        if (ended || isComplete)
          [controller notificationSwipeEnded:ended complete:isComplete];
    };
    [event trackSwipeEventWithOptions:NSEventSwipeTrackingLockDirection
             dampenAmountThresholdMin:-1
                                  max:1
                         usingHandler:handler];
  }
}

@end

////////////////////////////////////////////////////////////////////////////////

@implementation MCPopupController

- (id)initWithNotification:(const message_center::Notification*)notification
             messageCenter:(message_center::MessageCenter*)messageCenter
           popupCollection:(MCPopupCollection*)popupCollection {
  base::scoped_nsobject<MCPopupWindow> window([[MCPopupWindow alloc]
      initWithContentRect:ui::kWindowSizeDeterminedLater
                styleMask:NSNonactivatingPanelMask
                  backing:NSBackingStoreBuffered
                    defer:NO]);
  if ((self = [super initWithWindow:window])) {
    messageCenter_ = messageCenter;
    popupCollection_ = popupCollection;
    notificationController_.reset(
        [[MCNotificationController alloc] initWithNotification:notification
                                                 messageCenter:messageCenter_]);
    bounds_ = [[notificationController_ view] frame];

    [window setFloatingPanel:YES];
    [window setBecomesKeyOnlyIfNeeded:YES];
    [window
        setCollectionBehavior:NSWindowCollectionBehaviorCanJoinAllSpaces |
                              NSWindowCollectionBehaviorFullScreenAuxiliary];

    [window setHasShadow:YES];
    [window setContentView:[notificationController_ view]];

    trackingArea_.reset(
        [[CrTrackingArea alloc] initWithRect:NSZeroRect
                                     options:NSTrackingInVisibleRect |
                                             NSTrackingMouseEnteredAndExited |
                                             NSTrackingActiveAlways
                                       owner:self
                                    userInfo:nil]);
    [[window contentView] addTrackingArea:trackingArea_.get()];
  }
  return self;
}

#ifndef NDEBUG
- (void)dealloc {
  DCHECK(hasBeenClosed_);
  [super dealloc];
}
#endif

- (void)close {
#ifndef NDEBUG
  hasBeenClosed_ = YES;
#endif
  [self setBoundsAnimation:nil];
  if (trackingArea_.get())
    [[[self window] contentView] removeTrackingArea:trackingArea_.get()];
  [super close];
  [self performSelectorOnMainThread:@selector(release)
                         withObject:nil
                      waitUntilDone:NO
                              modes:@[ NSDefaultRunLoopMode ]];
}

- (MCNotificationController*)notificationController {
  return notificationController_.get();
}

- (const message_center::Notification*)notification {
  return [notificationController_ notification];
}

- (const std::string&)notificationID {
  return [notificationController_ notificationID];
}

// Private /////////////////////////////////////////////////////////////////////

- (void)notificationSwipeStarted {
  originalFrame_ = [[self window] frame];
  swipeGestureEnded_ = NO;
}

- (void)notificationSwipeMoved:(CGFloat)amount {
  NSWindow* window = [self window];

  [window setAlphaValue:1.0 - std::abs(amount)];
  NSRect frame = [window frame];
  CGFloat originalMin = NSMinX(originalFrame_);
  frame.origin.x = originalMin + (NSMidX(originalFrame_) - originalMin) *
                   -amount;
  [window setFrame:frame display:YES];
}

- (void)notificationSwipeEnded:(BOOL)ended complete:(BOOL)isComplete {
  swipeGestureEnded_ |= ended;
  if (swipeGestureEnded_ && isComplete) {
    messageCenter_->RemoveNotification([self notificationID], /*by_user=*/true);
    [popupCollection_ onPopupAnimationEnded:[self notificationID]];
  }
}

- (void)setBoundsAnimation:(NSViewAnimation*)animation {
  [boundsAnimation_ stopAnimation];
  [boundsAnimation_ setDelegate:nil];
  boundsAnimation_.reset([animation retain]);
}

- (NSViewAnimation*)animationWithDictionary:(NSDictionary*)dictionary {
  return [[[NSViewAnimation alloc]
      initWithViewAnimations:@[ dictionary ]] autorelease];
}

- (void)animationDidEnd:(NSAnimation*)animation {
  DCHECK_EQ(animation, boundsAnimation_.get());
  [self setBoundsAnimation:nil];

  [popupCollection_ onPopupAnimationEnded:[self notificationID]];

  if (isClosing_)
    [self close];
}

- (void)showWithAnimation:(NSRect)newBounds {
  bounds_ = newBounds;
  NSRect startBounds = newBounds;
  startBounds.origin.x += startBounds.size.width;
  [[self window] setFrame:startBounds display:NO];
  [[self window] setAlphaValue:0];
  [[self window] setCanHide:NO];
  [self showWindow:nil];

  // Slide-in and fade-in simultaneously.
  NSDictionary* animationDict = @{
    NSViewAnimationTargetKey : [self window],
    NSViewAnimationEndFrameKey : [NSValue valueWithRect:newBounds],
    NSViewAnimationEffectKey : NSViewAnimationFadeInEffect
  };
  NSViewAnimation* animation = [self animationWithDictionary:animationDict];
  [self setBoundsAnimation:animation];
  [boundsAnimation_ setDuration:[popupCollection_ popupAnimationDuration]];
  [boundsAnimation_ setDelegate:self];
  [boundsAnimation_ startAnimation];
}

- (void)closeWithAnimation {
  if (isClosing_)
    return;

#ifndef NDEBUG
  hasBeenClosed_ = YES;
#endif
  isClosing_ = YES;

  // If the notification was swiped closed, do not animate it as the
  // notification has already faded out.
  if (swipeGestureEnded_) {
    [self close];
    return;
  }

  NSDictionary* animationDict = @{
    NSViewAnimationTargetKey : [self window],
    NSViewAnimationEffectKey : NSViewAnimationFadeOutEffect
  };
  NSViewAnimation* animation = [self animationWithDictionary:animationDict];
  [self setBoundsAnimation:animation];
  [boundsAnimation_ setDuration:[popupCollection_ popupAnimationDuration]];
  [boundsAnimation_ setDelegate:self];
  [boundsAnimation_ startAnimation];
}

- (void)markPopupCollectionGone {
  popupCollection_ = nil;
}

- (NSRect)bounds {
  return bounds_;
}

- (void)setBounds:(NSRect)newBounds {
  if (isClosing_ || NSEqualRects(bounds_ , newBounds))
    return;
  bounds_ = newBounds;

  NSDictionary* animationDict = @{
    NSViewAnimationTargetKey :   [self window],
    NSViewAnimationEndFrameKey : [NSValue valueWithRect:newBounds]
  };
  NSViewAnimation* animation = [self animationWithDictionary:animationDict];
  [self setBoundsAnimation:animation];
  [boundsAnimation_ setDuration:[popupCollection_ popupAnimationDuration]];
  [boundsAnimation_ setDelegate:self];
  [boundsAnimation_ startAnimation];
}

- (void)mouseEntered:(NSEvent*)event {
  messageCenter_->PausePopupTimers();
}

- (void)mouseExited:(NSEvent*)event {
  messageCenter_->RestartPopupTimers();
}

@end
