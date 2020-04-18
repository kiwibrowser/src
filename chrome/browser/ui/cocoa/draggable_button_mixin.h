// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_COCOA_DRAGGABLE_BUTTON_MIXIN_H_
#define CHROME_BROWSER_UI_COCOA_DRAGGABLE_BUTTON_MIXIN_H_

#import <Cocoa/Cocoa.h>

// The design of this class is extraordinarily poor. Apologies to all clients in
// advance. Unfortunately, the lack of multiple inheritance and our desire to
// avoid runtime hacks makes this convoluted dance necessary.
//
// Buttons that want to be draggable should implement the Mixin protocol below
// and keep an instance of the Impl as an ivar. The button should forward mouse
// events to the impl, which will tell the button whether or not to call super
// and let the event be handled normally.
//
// If the impl decides to do work on the event, methods of the mixin protocol
// may be called. Some of the methods declared in that protocol have base
// implementations. If the method is not implemented by the button, that base
// implementation will be called. Otherwise, the button's implementation will
// be called first and the DraggableButtonResult will be used to determine
// whether the base implementation should be called. This requires the client to
// understand what the base does.

enum DraggableButtonResult {
  // Return values for Impl methods.
  kDraggableButtonImplDidWork,
  kDraggableButtonMixinCallSuper,

  // Return values for Mixin methods.
  kDraggableButtonMixinDidWork,
  kDraggableButtonImplUseBase,
};

// Mixin Protocol //////////////////////////////////////////////////////////////

// Buttons that make use of the below impl need to conform to this protocol.
@protocol DraggableButtonMixin

@required

// Called when a drag should start. Implement this to do any pasteboard
// manipulation and begin the drag, usually with
// -dragImage:at:offset:event:. Subclasses must call one of the blocking
// -drag* methods of NSView when implementing this method.
- (void)beginDrag:(NSEvent*)dragEvent;

@optional

// Called if the actsOnMouseDown property is set. Fires the button's action and
// tracks the click.
- (DraggableButtonResult)performMouseDownAction:(NSEvent*)theEvent;

// Implement if you want to do any extra work on mouseUp, after a mouseDown
// action has already fired.
- (DraggableButtonResult)secondaryMouseUpAction:(BOOL)wasInside;

// Resets the draggable state of the button after dragging is finished. This is
// called by DraggableButtonImpl when the beginDrag call returns.
- (DraggableButtonResult)endDrag;

// Decides whether to treat the click as a cue to start dragging, or to instead
// call the mouseDown/mouseUp handler as appropriate.  Implement if you want to
// do something tricky when making the decision.
- (DraggableButtonResult)deltaIndicatesDragStartWithXDelta:(float)xDelta
    yDelta:(float)yDelta
    xHysteresis:(float)xHysteresis
    yHysteresis:(float)yHysteresis
    indicates:(BOOL*)result;

// Decides if there is enough information to stop tracking the mouse.
// It's deltaIndicatesDragStartWithXDelta, however, that decides whether it's a
// drag or not. Implement if you want to do something tricky when making the
// decision.
- (DraggableButtonResult)deltaIndicatesConclusionReachedWithXDelta:(float)xDelta
    yDelta:(float)yDelta
    xHysteresis:(float)xHysteresis
    yHysteresis:(float)yHysteresis
    indicates:(BOOL*)result;

@end

// Impl Interface //////////////////////////////////////////////////////////////

// Implementation of the drag and drop logic. NSButton Mixin subclasses should
// forward their mouse events to this, which in turn will call out to the mixin
// protocol.
@interface DraggableButtonImpl : NSObject {
 @private
  // The button for which this class is implementing stuff.
  NSButton<DraggableButtonMixin>* button_;

  // Is this a draggable type of button?
  BOOL draggable_;

  // Has the action already fired for this click?
  BOOL actionHasFired_;

  // Does button action happen on mouse down when possible?
  BOOL actsOnMouseDown_;

  NSTimeInterval durationMouseWasDown_;
  NSTimeInterval whenMouseDown_;
}

@property(nonatomic) NSTimeInterval durationMouseWasDown;

@property(nonatomic) NSTimeInterval whenMouseDown;

// Whether the action has already fired for this click.
@property(nonatomic) BOOL actionHasFired;

// Enable or disable dragability for special buttons like "Other Bookmarks".
@property(nonatomic) BOOL draggable;

// If it has a popup menu, for example, we want to perform the action on mouse
// down, if possible (as long as user still gets chance to drag, if
// appropriate).
@property(nonatomic) BOOL actsOnMouseDown;

// Designated initializer.
- (id)initWithButton:(NSButton<DraggableButtonMixin>*)button;

// NSResponder implementation. NSButton subclasses should invoke these methods
// and only call super if the return value indicates such.
- (DraggableButtonResult)mouseDownImpl:(NSEvent*)event;
- (DraggableButtonResult)mouseUpImpl:(NSEvent*)event;

@end

#endif  // CHROME_BROWSER_UI_COCOA_DRAGGABLE_BUTTON_MIXIN_H_
