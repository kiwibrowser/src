// Copyright (c) 2009 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "chrome/browser/ui/cocoa/clickhold_button_cell.h"

#include "base/logging.h"

// Minimum and maximum click-hold timeout.
static const NSTimeInterval kMinTimeout = 0.0;
static const NSTimeInterval kMaxTimeout = 5.0;

// Drag distance threshold to activate click-hold; should be >= 0.
static const CGFloat kDragDistThreshold = 2.5;

// See |-resetToDefaults| (and header file) for other default values.

@interface ClickHoldButtonCell (Private)
- (void)resetToDefaults;
- (BOOL)shouldExposeAccessibilityShowMenu;
- (id)accessibilityAttributeValue:(NSString*)attribute;
@end  // @interface ClickHoldButtonCell (Private)

@implementation ClickHoldButtonCell

@synthesize enableClickHold = enableClickHold_;
@synthesize enableRightClick = enableRightClick_;
@synthesize clickHoldTimeout = clickHoldTimeout_;
@synthesize trackOnlyInRect = trackOnlyInRect_;
@synthesize activateOnDrag = activateOnDrag_;
@synthesize clickHoldTarget = clickHoldTarget_;
@synthesize clickHoldAction = clickHoldAction_;
@synthesize accessibilityShowMenuTarget = accessibilityShowMenuTarget_;
@synthesize accessibilityShowMenuAction = accessibilityShowMenuAction_;

// Overrides:

+ (BOOL)prefersTrackingUntilMouseUp {
  return NO;
}

- (id)init {
  if ((self = [super init]))
    [self resetToDefaults];
  return self;
}

- (id)initWithCoder:(NSCoder*)decoder {
  if ((self = [super initWithCoder:decoder]))
    [self resetToDefaults];
  return self;
}

- (id)initImageCell:(NSImage*)image {
  if ((self = [super initImageCell:image]))
    [self resetToDefaults];
  return self;
}

- (id)initTextCell:(NSString*)string {
  if ((self = [super initTextCell:string]))
    [self resetToDefaults];
  return self;
}

- (void)accessibilityPerformAction:(NSString*)action {
  if ([action isEqualToString:NSAccessibilityShowMenuAction] &&
      [self shouldExposeAccessibilityShowMenu]) {
    NSControl* controlView = static_cast<NSControl*>([self controlView]);
    if (controlView)
      [controlView sendAction:accessibilityShowMenuAction_
                           to:accessibilityShowMenuTarget_];
    return;
  }

  [super accessibilityPerformAction:action];
}

- (BOOL)startTrackingAt:(NSPoint)startPoint
                 inView:(NSView*)controlView {
  return enableClickHold_ ? YES :
                            [super startTrackingAt:startPoint
                                            inView:controlView];
}

- (BOOL)continueTracking:(NSPoint)lastPoint
                      at:(NSPoint)currentPoint
                  inView:(NSView*)controlView {
  return enableClickHold_ ? YES :
                            [super continueTracking:lastPoint
                                                 at:currentPoint
                                             inView:controlView];
}

- (BOOL)trackMouse:(NSEvent*)originalEvent
            inRect:(NSRect)cellFrame
            ofView:(NSView*)controlView
      untilMouseUp:(BOOL)untilMouseUp {
  if (!enableClickHold_) {
    return [super trackMouse:originalEvent
                      inRect:cellFrame
                      ofView:controlView
                untilMouseUp:untilMouseUp];
  }

  // If doing click-hold, track the mouse ourselves.
  NSPoint currPoint = [controlView convertPoint:[originalEvent locationInWindow]
                                       fromView:nil];
  NSPoint lastPoint = currPoint;
  NSPoint firstPoint = currPoint;
  NSTimeInterval timeout =
      MAX(MIN(clickHoldTimeout_, kMaxTimeout), kMinTimeout);
  NSDate* clickHoldBailTime = [NSDate dateWithTimeIntervalSinceNow:timeout];

  if (![self startTrackingAt:currPoint inView:controlView])
    return NO;

  enum {
    kContinueTrack, kStopClickHold, kStopMouseUp, kStopLeftRect, kStopNoContinue
  } state = kContinueTrack;
  do {
    NSEvent* event = [NSApp nextEventMatchingMask:(NSLeftMouseDraggedMask |
                                                   NSLeftMouseUpMask)
                                        untilDate:clickHoldBailTime
                                           inMode:NSEventTrackingRunLoopMode
                                          dequeue:YES];
    currPoint = [controlView convertPoint:[event locationInWindow]
                                 fromView:nil];

    // Time-out.
    if (!event) {
      state = kStopClickHold;

    // Drag? (If distance meets threshold.)
    } else if (activateOnDrag_ && ([event type] == NSLeftMouseDragged)) {
      CGFloat dx = currPoint.x - firstPoint.x;
      CGFloat dy = currPoint.y - firstPoint.y;
      if ((dx*dx + dy*dy) >= (kDragDistThreshold*kDragDistThreshold))
        state = kStopClickHold;

    // Mouse up.
    } else if ([event type] == NSLeftMouseUp) {
      state = kStopMouseUp;

    // Stop tracking if mouse left frame rectangle (if requested to do so).
    } else if (trackOnlyInRect_ && ![controlView mouse:currPoint
                                                inRect:cellFrame]) {
      state = kStopLeftRect;

    // Stop tracking if instructed to.
    } else if (![self continueTracking:lastPoint
                                    at:currPoint
                                inView:controlView]) {
      state = kStopNoContinue;
    }

    lastPoint = currPoint;
  } while (state == kContinueTrack);

  [self stopTracking:lastPoint
                  at:lastPoint
              inView:controlView
           mouseIsUp:NO];

  switch (state) {
    case kStopClickHold:
      if (clickHoldAction_) {
        [static_cast<NSControl*>(controlView) sendAction:clickHoldAction_
                                                      to:clickHoldTarget_];
      }
      return YES;

    case kStopMouseUp:
      if ([self action]) {
        [static_cast<NSControl*>(controlView) sendAction:[self action]
                                                      to:[self target]];
      }
      return YES;

    case kStopLeftRect:
    case kStopNoContinue:
      return NO;

    default:
      NOTREACHED() << "Unknown terminating state!";
  }

  return NO;
}

// Accessors and mutators:

- (NSArray*)accessibilityActionNames {
  NSArray* actionNames = [super accessibilityActionNames];
  if ([self shouldExposeAccessibilityShowMenu] &&
      ![actionNames containsObject:NSAccessibilityShowMenuAction]) {
    return [actionNames arrayByAddingObject:NSAccessibilityShowMenuAction];
  }

  return actionNames;
}

@end  // @implementation ClickHoldButtonCell

@implementation ClickHoldButtonCell (Private)

// Resets various members to defaults indicated in the header file. (Those
// without indicated defaults are *not* touched.) Please keep the values below
// in sync with the header file, and please be aware of side-effects on code
// which relies on the "published" defaults.
- (void)resetToDefaults {
  [self setEnableClickHold:NO];
  [self setClickHoldTimeout:0.25];
  [self setTrackOnlyInRect:NO];
  [self setActivateOnDrag:YES];
}

- (BOOL)shouldExposeAccessibilityShowMenu {
  return (enableRightClick_ ||
      (enableClickHold_ && clickHoldTimeout_ > kMinTimeout)) &&
      accessibilityShowMenuAction_ && accessibilityShowMenuTarget_;
}

- (id)accessibilityAttributeValue:(NSString*)attribute {
  if ([attribute isEqual:NSAccessibilityRoleAttribute] &&
      [self clickHoldTimeout] == 0.0) {
    // When the delay is set to zero, this button operates like a popup button
    // from a user perspective.
    return NSAccessibilityPopUpButtonRole;
  }
  return [super accessibilityAttributeValue:attribute];
}

@end  // @implementation ClickHoldButtonCell (Private)
