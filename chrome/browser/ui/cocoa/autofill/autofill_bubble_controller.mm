// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "chrome/browser/ui/cocoa/autofill/autofill_bubble_controller.h"

#import "chrome/browser/ui/cocoa/autofill/autofill_dialog_constants.h"
#import "chrome/browser/ui/cocoa/info_bubble_window.h"
#include "skia/ext/skia_utils_mac.h"

namespace {

// Border inset for error label.
const CGFloat kLabelInset = 3.0;

}  // namespace

@interface AutofillBubbleController () {
  CGFloat maxLabelWidth_;
}

// Update the current message, keeping arrow location in place.
- (void)updateMessage:(NSString*)message display:(BOOL)display;

@end

@implementation AutofillBubbleController

- (id)initWithParentWindow:(NSWindow*)parentWindow
                   message:(NSString*)message {
  return [self initWithParentWindow:parentWindow
                            message:message
                              inset:NSMakeSize(kLabelInset, kLabelInset)
                      maxLabelWidth:CGFLOAT_MAX
                      arrowLocation:info_bubble::kTopCenter];
}

- (id)initWithParentWindow:(NSWindow*)parentWindow
                   message:(NSString*)message
                     inset:(NSSize)inset
             maxLabelWidth:(CGFloat)maxLabelWidth
             arrowLocation:(info_bubble::BubbleArrowLocation)arrowLocation {
  base::scoped_nsobject<InfoBubbleWindow> window(
      [[InfoBubbleWindow alloc] initWithContentRect:NSMakeRect(0, 0, 200, 100)
                                          styleMask:NSBorderlessWindowMask
                                            backing:NSBackingStoreBuffered
                                              defer:NO]);
  [window setAllowedAnimations:info_bubble::kAnimateNone];
  if ((self = [super initWithWindow:window
                       parentWindow:parentWindow
                         anchoredAt:NSZeroPoint])) {
    inset_ = inset;
    maxLabelWidth_ = maxLabelWidth;
    [self setShouldOpenAsKeyWindow:NO];
    [[self bubble] setArrowLocation:arrowLocation];
    [[self bubble] setAlignment:info_bubble::kAlignArrowToAnchor];

    label_.reset([[NSTextField alloc] init]);
    [label_ setEditable:NO];
    [label_ setBordered:NO];
    [label_ setDrawsBackground:NO];
    [[self bubble] addSubview:label_];

    [self updateMessage:message display:NO];
  }
  return self;
}

- (void)setMessage:(NSString*)message {
  if ([[label_ stringValue] isEqualToString:message])
    return;
  [self updateMessage:message display:YES];
}

- (void)updateMessage:(NSString*)message display:(BOOL)display {
  [label_ setStringValue:message];

  NSRect labelFrame = NSMakeRect(inset_.width, inset_.height, 0, 0);
  labelFrame.size = [[label_ cell]
      cellSizeForBounds:NSMakeRect(0, 0, maxLabelWidth_, CGFLOAT_MAX)];
  [label_ setFrame:labelFrame];

  // Update window's size, but ensure the origin is maintained so that the arrow
  // still points at the same location.
  NSRect windowFrame = [[self window] frame];
  NSPoint origin = windowFrame.origin;
  origin.y += NSHeight(windowFrame);
  windowFrame.size =
      NSMakeSize(NSWidth([label_ frame]),
                 NSHeight([label_ frame]) + info_bubble::kBubbleArrowHeight);
  windowFrame = NSInsetRect(windowFrame, -inset_.width, -inset_.height);
  origin.y -= NSHeight(windowFrame);
  windowFrame.origin = origin;
  [[self window] setFrame:windowFrame display:display];
}

@end
