// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "chrome/browser/ui/cocoa/info_bubble_window.h"

#include <Carbon/Carbon.h>

#include "base/logging.h"
#import "base/mac/foundation_util.h"
#import "base/mac/scoped_nsobject.h"
#import "base/mac/sdk_forward_declarations.h"
#include "base/macros.h"
#include "chrome/browser/chrome_notification_types.h"
#import "chrome/browser/ui/cocoa/base_bubble_controller.h"
#include "content/public/browser/notification_observer.h"
#include "content/public/browser/notification_registrar.h"
#include "content/public/browser/notification_service.h"
#import "third_party/google_toolbox_for_mac/src/AppKit/GTMNSAnimation+Duration.h"

namespace {
const CGFloat kOrderInSlideOffset = 10;
const NSTimeInterval kOrderInAnimationDuration = 0.075;
const NSTimeInterval kOrderOutAnimationDuration = 0.15;
// The minimum representable time interval.  This can be used as the value
// passed to +[NSAnimationContext setDuration:] to stop an in-progress
// animation as quickly as possible.
const NSTimeInterval kMinimumTimeInterval =
    std::numeric_limits<NSTimeInterval>::min();
}  // namespace

@interface InfoBubbleWindow (Private)
- (void)appIsTerminating;
- (void)finishCloseAfterAnimation;
@end

// A helper class to proxy app notifications to the window.
class AppNotificationBridge : public content::NotificationObserver {
 public:
  explicit AppNotificationBridge(InfoBubbleWindow* owner) : owner_(owner) {
    registrar_.Add(this, chrome::NOTIFICATION_APP_TERMINATING,
                   content::NotificationService::AllSources());
  }

  // Overridden from content::NotificationObserver.
  void Observe(int type,
               const content::NotificationSource& source,
               const content::NotificationDetails& details) override {
    switch (type) {
      case chrome::NOTIFICATION_APP_TERMINATING:
        [owner_ appIsTerminating];
        break;
      default:
        NOTREACHED() << "Unexpected notification";
    }
  }

 private:
  // The object we need to inform when we get a notification. Weak. Owns us.
  InfoBubbleWindow* owner_;

  // Used for registering to receive notifications and automatic clean up.
  content::NotificationRegistrar registrar_;

  DISALLOW_COPY_AND_ASSIGN(AppNotificationBridge);
};

// A delegate object for watching the alphaValue animation on InfoBubbleWindows.
// An InfoBubbleWindow instance cannot be the delegate for its own animation
// because CAAnimations retain their delegates, and since the InfoBubbleWindow
// retains its animations a retain loop would be formed.
@interface InfoBubbleWindowCloser : NSObject <CAAnimationDelegate> {
 @private
  InfoBubbleWindow* window_;  // Weak. Window to close.
}
- (id)initWithWindow:(InfoBubbleWindow*)window;
@end

@implementation InfoBubbleWindowCloser

- (id)initWithWindow:(InfoBubbleWindow*)window {
  if ((self = [super init])) {
    window_ = window;
  }
  return self;
}

- (void)animationDidStart:(CAAnimation*)theAnimation {
  // CAAnimationDelegate method added on OSX 10.12.
}

// Callback for the alpha animation. Closes window_ if appropriate.
- (void)animationDidStop:(CAAnimation*)anim finished:(BOOL)flag {
  // When alpha reaches zero, close window_.
  if ([window_ alphaValue] == 0.0) {
    [window_ finishCloseAfterAnimation];
  }
}

@end


@implementation InfoBubbleWindow

@synthesize allowedAnimations = allowedAnimations_;
@synthesize infoBubbleCanBecomeKeyWindow = infoBubbleCanBecomeKeyWindow_;
@synthesize allowShareParentKeyState = allowShareParentKeyState_;

- (id)initWithContentRect:(NSRect)contentRect
                styleMask:(NSUInteger)aStyle
                  backing:(NSBackingStoreType)bufferingType
                    defer:(BOOL)flag {
  if ((self = [super initWithContentRect:contentRect
                               styleMask:NSBorderlessWindowMask
                                 backing:bufferingType
                                   defer:flag])) {
    [self setBackgroundColor:[NSColor clearColor]];
    [self setExcludedFromWindowsMenu:YES];
    [self setAllowShareParentKeyState:YES];
    [self setOpaque:NO];
    [self setHasShadow:YES];
    infoBubbleCanBecomeKeyWindow_ = YES;
    allowedAnimations_ = info_bubble::kAnimateOrderIn |
                         info_bubble::kAnimateOrderOut;
    notificationBridge_.reset(new AppNotificationBridge(self));

    // Start invisible. Will be made visible when ordered front.
    [self setAlphaValue:0.0];

    // Set up alphaValue animation so that self is delegate for the animation.
    // Setting up the delegate is required so that the
    // animationDidStop:finished: callback can be handled.
    // Notice that only the alphaValue Animation is replaced in case
    // superclasses set up animations.
    CAAnimation* alphaAnimation = [CABasicAnimation animation];
    base::scoped_nsobject<InfoBubbleWindowCloser> delegate(
        [[InfoBubbleWindowCloser alloc] initWithWindow:self]);
    [alphaAnimation setDelegate:delegate];
    NSMutableDictionary* animations =
        [NSMutableDictionary dictionaryWithDictionary:[self animations]];
    [animations setObject:alphaAnimation forKey:@"alphaValue"];
    [self setAnimations:animations];
  }
  return self;
}

- (BOOL)performKeyEquivalent:(NSEvent*)event {
  if (([event keyCode] == kVK_Escape) ||
      (([event keyCode] == kVK_ANSI_Period) &&
       (([event modifierFlags] & NSCommandKeyMask) != 0))) {
    BaseBubbleController* bubbleController =
        base::mac::ObjCCastStrict<BaseBubbleController>(
            [self windowController]);
    [bubbleController cancel:self];
    return YES;
  }
  return [super performKeyEquivalent:event];
}

// According to
// http://www.cocoabuilder.com/archive/message/cocoa/2006/6/19/165953,
// NSBorderlessWindowMask windows cannot become key or main. In this
// case, this is not necessarily a desired behavior. As an example, the
// bubble could have buttons.
- (BOOL)canBecomeKeyWindow {
  return infoBubbleCanBecomeKeyWindow_;
}

// Lets the traffic light buttons on the browser window keep their "active"
// state while an info bubble is open. Only has an effect on 10.7.
- (BOOL)_sharesParentKeyState {
  return allowShareParentKeyState_;
}

- (void)close {
  // Block the window from receiving events while it fades out.
  closing_ = YES;

  if ((allowedAnimations_ & info_bubble::kAnimateOrderOut) == 0) {
    [self finishCloseAfterAnimation];
  } else {
    // Apply animations to hide self.
    [NSAnimationContext beginGrouping];
    [[NSAnimationContext currentContext]
        gtm_setDuration:kOrderOutAnimationDuration
              eventMask:NSLeftMouseUpMask];
    [[self animator] setAlphaValue:0.0];
    [NSAnimationContext endGrouping];
  }
}

// If the app is terminating but the window is still fading out, cancel the
// animation and close the window to prevent it from leaking.
// See http://crbug.com/37717
- (void)appIsTerminating {
  if ((allowedAnimations_ & info_bubble::kAnimateOrderOut) == 0)
    return;  // The close has already happened with no Core Animation.

  // Cancel the current animation so that it closes immediately, triggering
  // |finishCloseAfterAnimation|.
  [NSAnimationContext beginGrouping];
  [[NSAnimationContext currentContext] setDuration:kMinimumTimeInterval];
  [[self animator] setAlphaValue:0.0];
  [NSAnimationContext endGrouping];
}

// Called by InfoBubbleWindowCloser when the window is to be really closed
// after the fading animation is complete.
- (void)finishCloseAfterAnimation {
  if (closing_) {
    [[self parentWindow] removeChildWindow:self];
    [super close];
  }
}

// Adds animation for info bubbles being ordered to the front.
- (void)orderWindow:(NSWindowOrderingMode)orderingMode
         relativeTo:(NSInteger)otherWindowNumber {
  // According to the documentation '0' is the otherWindowNumber when the window
  // is ordered front.
  if (orderingMode == NSWindowAbove && otherWindowNumber == 0) {
    // Order self appropriately assuming that its alpha is zero as set up
    // in the designated initializer.
    [super orderWindow:orderingMode relativeTo:otherWindowNumber];

    // Set up frame so it can be adjust down by a few pixels.
    NSRect frame = [self frame];
    NSPoint newOrigin = frame.origin;
    newOrigin.y += kOrderInSlideOffset;
    [self setFrameOrigin:newOrigin];

    // Apply animations to show and move self.
    [NSAnimationContext beginGrouping];
    // The star currently triggers on mouse down, not mouse up.
    NSTimeInterval duration =
        (allowedAnimations_ & info_bubble::kAnimateOrderIn)
            ? kOrderInAnimationDuration : kMinimumTimeInterval;
    [[NSAnimationContext currentContext]
        gtm_setDuration:duration
              eventMask:NSLeftMouseUpMask | NSLeftMouseDownMask];
    [[self animator] setAlphaValue:1.0];
    [[self animator] setFrame:frame display:YES];
    [NSAnimationContext endGrouping];
  } else {
    [super orderWindow:orderingMode relativeTo:otherWindowNumber];
  }
}

// If the window is currently animating a close, block all UI events to the
// window.
- (void)sendEvent:(NSEvent*)theEvent {
  if (!closing_)
    [super sendEvent:theEvent];
}

- (BOOL)isClosing {
  return closing_;
}

// Override -[NSWindow addChildWindow] to prevent ShareKit bugs propagating
// to the browser window. See http://crbug.com/475855.
- (void)addChildWindow:(NSWindow*)childWindow
               ordered:(NSWindowOrderingMode)orderingMode {
  [[self parentWindow] removeChildWindow:self];
  [super addChildWindow:childWindow ordered:orderingMode];
}

@end
