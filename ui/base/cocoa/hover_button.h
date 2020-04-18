// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UI_BASE_COCOA_HOVER_BUTTON_
#define UI_BASE_COCOA_HOVER_BUTTON_

#import <Cocoa/Cocoa.h>

#import "ui/base/cocoa/tracking_area.h"
#import "ui/base/ui_base_export.h"

@class HoverButton;

// Assign an object which conforms to this protocol to a HoverButton's
// dragDelegate property to make the button draggable.
UI_BASE_EXPORT
@protocol HoverButtonDragDelegate

// When the user performs a drag on the HoverButton, this method will be called
// with the button and the mouse down event. The delegate is expected to begin
// a drag by calling -[NSView beginDraggingSessionWithItems:event:source:] with
// the event or run a nested tracking loop. When it returns, the HoverButton
// returns to kHoverStateNone and stops tracking the mouse.
- (void)beginDragFromHoverButton:(HoverButton*)button event:(NSEvent*)event;

@end

// A button that changes when you hover over it and click it.
UI_BASE_EXPORT
@interface HoverButton : NSButton {
 @protected
  // Enumeration of the hover states that the close button can be in at any one
  // time. The button cannot be in more than one hover state at a time.
  enum HoverState {
    kHoverStateNone = 0,
    kHoverStateMouseOver = 1,
    kHoverStateMouseDown = 2
  };

  HoverState hoverState_;

 @private
  // Tracking area for button mouseover states. Nil if not enabled.
  ui::ScopedCrTrackingArea trackingArea_;
  BOOL mouseDown_;
  BOOL sendActionOnMouseDown_;
}

@property(nonatomic) HoverState hoverState;

// Enables or disables the tracking for the button.
@property(nonatomic) BOOL trackingEnabled;

// Assign an object to make the button a drag source.
@property(nonatomic, assign) id<HoverButtonDragDelegate> dragDelegate;

// Enables or disables sending the action on mouse down event.
@property(nonatomic) BOOL sendActionOnMouseDown;

// An NSRect in the view's coordinate space which is used for hover and hit
// testing. Default value is NSZeroRect, which makes the hitbox equal to the
// view's bounds. May be overridden by subclasses. Example: A button in the
// corner of a fullscreen window might extend its hitbox to the edges of the
// window so that it can be clicked more easily (Fitts's law).
@property(readonly, nonatomic) NSRect hitbox;

// Common initialization called from initWithFrame: and awakeFromNib.
// Subclassers should call [super commonInit].
- (void)commonInit;

// Text that would be announced by screen readers.
- (void)setAccessibilityTitle:(NSString*)accessibilityTitle;

// Checks to see whether the mouse is in the button's bounds and update
// the image in case it gets out of sync.  This occurs to the close button
// when you close a tab so the tab to the left of it takes its place, and
// drag the button without moving the mouse before you press the button down.
- (void)checkImageState;

@end

#endif  // UI_BASE_COCOA_HOVER_BUTTON_
