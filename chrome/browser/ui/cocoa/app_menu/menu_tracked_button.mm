// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "chrome/browser/ui/cocoa/app_menu/menu_tracked_button.h"

#include "ui/base/cocoa/cocoa_base_utils.h"

@interface MenuTrackedButton (Private)
- (void)doHighlight:(BOOL)highlight;
- (void)checkMouseInRect;
- (NSRect)insetBounds;
@end

@implementation MenuTrackedButton

@synthesize tracking = tracking_;

- (void)updateTrackingAreas {
  [super updateTrackingAreas];
  [self removeTrackingRect:trackingTag_];
  trackingTag_ = [self addTrackingRect:NSInsetRect([self bounds], 1, 1)
                                 owner:self
                              userData:NULL
                          assumeInside:NO];
}

- (void)viewDidMoveToWindow {
  [self updateTrackingAreas];
  [self doHighlight:NO];
}

- (void)mouseEntered:(NSEvent*)theEvent {
  if (!tracking_) {
    didEnter_ = YES;
  }
  [self doHighlight:YES];
  [super mouseEntered:theEvent];
}

- (void)mouseExited:(NSEvent*)theEvent {
  didEnter_ = NO;
  tracking_ = NO;
  [self doHighlight:NO];
  [super mouseExited:theEvent];
}

- (void)mouseDragged:(NSEvent*)theEvent {
  tracking_ = !didEnter_;

  NSPoint point = [self convertPoint:[theEvent locationInWindow] fromView:nil];
  BOOL highlight = NSPointInRect(point, [self insetBounds]);
  [self doHighlight:highlight];

  // If tracking in non-sticky mode, poll the mouse cursor to see if it is still
  // over the button and thus needs to be highlighted.  The delay is the
  // smallest that still produces the effect while minimizing jank. Smaller
  // values make the selector fire too close to immediately/now for the mouse to
  // have moved off the receiver, and larger values produce lag.
  if (tracking_) {
    [self performSelector:@selector(checkMouseInRect)
               withObject:nil
               afterDelay:0.05
                  inModes:[NSArray arrayWithObject:NSEventTrackingRunLoopMode]];
  }
  [super mouseDragged:theEvent];
}

- (void)mouseUp:(NSEvent*)theEvent {
  [self doHighlight:NO];
  if (!tracking_) {
    return [super mouseUp:theEvent];
  }
  [self performClick:self];
  tracking_ = NO;
}

- (void)doHighlight:(BOOL)highlight {
  [[self cell] setHighlighted:highlight];
  [self setNeedsDisplay];
}

// Checks if the user's current mouse location is over this button.  If it is,
// the user is merely hovering here.  If it is not, then disable the highlight.
// If the menu is opened in non-sticky mode, the button does not receive enter/
// exit mouse events and thus polling is necessary.
- (void)checkMouseInRect {
  NSPoint point = [NSEvent mouseLocation];
  point = ui::ConvertPointFromScreenToWindow([self window], point);
  point = [self convertPoint:point fromView:nil];
  if (!NSPointInRect(point, [self insetBounds])) {
    [self doHighlight:NO];
  }
}

// Returns the bounds of the receiver slightly inset to avoid highlighting both
// buttons in a pair that overlap.
- (NSRect)insetBounds {
  return NSInsetRect([self bounds], 2, 1);
}

@end
