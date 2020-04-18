// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "chrome/browser/ui/cocoa/draggable_button.h"

#include <cmath>

#include "base/logging.h"

// Code taken from <http://codereview.chromium.org/180036/diff/3001/3004>.
// TODO(viettrungluu): Do we want common, standard code for drag hysteresis?
const CGFloat kWebDragStartHysteresisX = 5.0;
const CGFloat kWebDragStartHysteresisY = 5.0;
const CGFloat kDragExpirationTimeout = 0.45;

// Private /////////////////////////////////////////////////////////////////////

@interface DraggableButtonImpl (Private)

- (BOOL)deltaIndicatesDragStartWithXDelta:(float)xDelta
                                   yDelta:(float)yDelta
                              xHysteresis:(float)xHysteresis
                              yHysteresis:(float)yHysteresis;
- (BOOL)deltaIndicatesConclusionReachedWithXDelta:(float)xDelta
                                           yDelta:(float)yDelta
                                      xHysteresis:(float)xHysteresis
                                      yHysteresis:(float)yHysteresis;
- (void)performMouseDownAction:(NSEvent*)theEvent;
- (void)endDrag;
- (BOOL)dragShouldBeginFromMouseDown:(NSEvent*)mouseDownEvent
                      withExpiration:(NSDate*)expiration;
- (BOOL)dragShouldBeginFromMouseDown:(NSEvent*)mouseDownEvent
                      withExpiration:(NSDate*)expiration
                         xHysteresis:(float)xHysteresis
                         yHysteresis:(float)yHysteresis;

@end

// Implementation //////////////////////////////////////////////////////////////

@implementation DraggableButtonImpl

@synthesize draggable = draggable_;
@synthesize actsOnMouseDown = actsOnMouseDown_;
@synthesize durationMouseWasDown = durationMouseWasDown_;
@synthesize actionHasFired = actionHasFired_;
@synthesize whenMouseDown = whenMouseDown_;

- (id)initWithButton:(NSButton<DraggableButtonMixin>*)button {
  if ((self = [super init])) {
    button_ = button;
    draggable_ = YES;
    actsOnMouseDown_ = NO;
    actionHasFired_ = NO;
  }
  return self;
}

// NSButton/NSResponder Implementations ////////////////////////////////////////

- (DraggableButtonResult)mouseUpImpl:(NSEvent*)theEvent {
  durationMouseWasDown_ = [theEvent timestamp] - whenMouseDown_;

  if (actionHasFired_)
    return kDraggableButtonImplDidWork;

  if (!draggable_)
    return kDraggableButtonMixinCallSuper;

  // There are non-drag cases where a |-mouseUp:| may happen (e.g. mouse-down,
  // cmd-tab to another application, move mouse, mouse-up), so check.
  NSPoint viewLocal = [button_ convertPoint:[theEvent locationInWindow]
                                   fromView:[[button_ window] contentView]];
  if (NSPointInRect(viewLocal, [button_ bounds]))
    [button_ performClick:self];

  return kDraggableButtonImplDidWork;
}

// Mimic "begin a click" operation visually.  Do NOT follow through with normal
// button event handling.
- (DraggableButtonResult)mouseDownImpl:(NSEvent*)theEvent {
  [[NSCursor arrowCursor] set];

  whenMouseDown_ = [theEvent timestamp];
  actionHasFired_ = NO;

  if (draggable_) {
    NSDate* date = [NSDate dateWithTimeIntervalSinceNow:kDragExpirationTimeout];
    if ([self dragShouldBeginFromMouseDown:theEvent
                            withExpiration:date]) {
      [button_ beginDrag:theEvent];
      [self endDrag];
    } else {
      if (actsOnMouseDown_) {
        [self performMouseDownAction:theEvent];
        return kDraggableButtonImplDidWork;
      } else {
        return kDraggableButtonMixinCallSuper;
      }
    }
  } else {
    if (actsOnMouseDown_) {
      [self performMouseDownAction:theEvent];
      return kDraggableButtonImplDidWork;
    } else {
      return kDraggableButtonMixinCallSuper;
    }
  }

  return kDraggableButtonImplDidWork;
}

// Idempotent Helpers //////////////////////////////////////////////////////////

- (BOOL)deltaIndicatesDragStartWithXDelta:(float)xDelta
                                   yDelta:(float)yDelta
                              xHysteresis:(float)xHysteresis
                              yHysteresis:(float)yHysteresis {
  if ([button_ respondsToSelector:@selector(deltaIndicatesDragStartWithXDelta:
                                            yDelta:
                                            xHysteresis:
                                            yHysteresis:
                                            indicates:)]) {
    BOOL indicates = NO;
    DraggableButtonResult result = [button_
        deltaIndicatesDragStartWithXDelta:xDelta
        yDelta:yDelta
        xHysteresis:xHysteresis
        yHysteresis:yHysteresis
        indicates:&indicates];
    if (result != kDraggableButtonImplUseBase)
      return indicates;
  }
  return (std::abs(xDelta) >= xHysteresis) || (std::abs(yDelta) >= yHysteresis);
}

- (BOOL)deltaIndicatesConclusionReachedWithXDelta:(float)xDelta
                                           yDelta:(float)yDelta
                                      xHysteresis:(float)xHysteresis
                                      yHysteresis:(float)yHysteresis {
  if ([button_ respondsToSelector:
          @selector(deltaIndicatesConclusionReachedWithXDelta:
                    yDelta:
                    xHysteresis:
                    yHysteresis:
                    indicates:)]) {
    BOOL indicates = NO;
    DraggableButtonResult result = [button_
        deltaIndicatesConclusionReachedWithXDelta:xDelta
        yDelta:yDelta
        xHysteresis:xHysteresis
        yHysteresis:yHysteresis
        indicates:&indicates];
    if (result != kDraggableButtonImplUseBase)
      return indicates;
  }
  return (std::abs(xDelta) >= xHysteresis) || (std::abs(yDelta) >= yHysteresis);
}

- (BOOL)dragShouldBeginFromMouseDown:(NSEvent*)mouseDownEvent
                      withExpiration:(NSDate*)expiration {
  return [self dragShouldBeginFromMouseDown:mouseDownEvent
                             withExpiration:expiration
                                xHysteresis:kWebDragStartHysteresisX
                                yHysteresis:kWebDragStartHysteresisY];
}

// Implementation Details //////////////////////////////////////////////////////

// Determine whether a mouse down should turn into a drag; started as copy of
// NSTableView code.
- (BOOL)dragShouldBeginFromMouseDown:(NSEvent*)mouseDownEvent
                      withExpiration:(NSDate*)expiration
                         xHysteresis:(float)xHysteresis
                         yHysteresis:(float)yHysteresis {
  if ([mouseDownEvent type] != NSLeftMouseDown) {
    return NO;
  }

  NSEvent* nextEvent = nil;
  NSEvent* firstEvent = nil;
  NSEvent* dragEvent = nil;
  NSEvent* mouseUp = nil;
  BOOL dragIt = NO;

  while ((nextEvent = [[button_ window]
      nextEventMatchingMask:NSLeftMouseUpMask | NSLeftMouseDraggedMask
                  untilDate:expiration
                     inMode:NSEventTrackingRunLoopMode
                    dequeue:YES]) != nil) {
    if (firstEvent == nil) {
      firstEvent = nextEvent;
    }
    if ([nextEvent type] == NSLeftMouseDragged) {
      float deltax = [nextEvent locationInWindow].x -
          [mouseDownEvent locationInWindow].x;
      float deltay = [nextEvent locationInWindow].y -
          [mouseDownEvent locationInWindow].y;
      dragEvent = nextEvent;
      if ([self deltaIndicatesConclusionReachedWithXDelta:deltax
                                                   yDelta:deltay
                                              xHysteresis:xHysteresis
                                              yHysteresis:yHysteresis]) {
        dragIt = [self deltaIndicatesDragStartWithXDelta:deltax
                                                  yDelta:deltay
                                             xHysteresis:xHysteresis
                                             yHysteresis:yHysteresis];
        break;
      }
    } else if ([nextEvent type] == NSLeftMouseUp) {
      mouseUp = nextEvent;
      break;
    }
  }

  // Since we've been dequeuing the events (If we don't, we'll never see
  // the mouse up...), we need to push some of the events back on.
  // It makes sense to put the first and last drag events and the mouse
  // up if there was one.
  if (mouseUp != nil) {
    [NSApp postEvent:mouseUp atStart:YES];
  }
  if (dragEvent != nil) {
    [NSApp postEvent:dragEvent atStart:YES];
  }
  if (firstEvent != mouseUp && firstEvent != dragEvent) {
    [NSApp postEvent:firstEvent atStart:YES];
  }

  return dragIt;
}

- (void)secondaryMouseUpAction:(BOOL)wasInside {
  if ([button_ respondsToSelector:_cmd])
    [button_ secondaryMouseUpAction:wasInside];

  // No actual implementation yet.
}

- (void)performMouseDownAction:(NSEvent*)event {
  if ([button_ respondsToSelector:_cmd] &&
      [button_ performMouseDownAction:event] != kDraggableButtonImplUseBase) {
      return;
  }

  int eventMask = NSLeftMouseUpMask;

  [[button_ target] performSelector:[button_ action] withObject:self];
  actionHasFired_ = YES;

  while (1) {
    event = [[button_ window] nextEventMatchingMask:eventMask];
    if (!event)
      continue;
    NSPoint mouseLoc = [button_ convertPoint:[event locationInWindow]
                                    fromView:nil];
    BOOL isInside = [button_ mouse:mouseLoc inRect:[button_ bounds]];
    [button_ highlight:isInside];

    switch ([event type]) {
      case NSLeftMouseUp:
        durationMouseWasDown_ = [event timestamp] - whenMouseDown_;
        [self secondaryMouseUpAction:isInside];
        break;
      default:
        // Ignore any other kind of event.
        break;
    }
  }

  [button_ highlight:NO];
}

- (void)endDrag {
  if ([button_ respondsToSelector:_cmd] &&
      [button_ endDrag] != kDraggableButtonImplUseBase) {
    return;
  }
  [button_ highlight:NO];
}

@end
