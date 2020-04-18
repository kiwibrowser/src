// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "chrome/browser/ui/cocoa/draggable_button.h"

#include "base/logging.h"

@implementation DraggableButton

- (id)initWithFrame:(NSRect)frame {
  if ((self = [super initWithFrame:frame])) {
    draggableButtonImpl_.reset(
        [[DraggableButtonImpl alloc] initWithButton:self]);
  }
  return self;
}

- (id)initWithCoder:(NSCoder*)coder {
  if ((self = [super initWithCoder:coder])) {
    draggableButtonImpl_.reset(
        [[DraggableButtonImpl alloc] initWithButton:self]);
  }
  return self;
}

- (DraggableButtonImpl*)draggableButton {
  return draggableButtonImpl_.get();
}

- (void)mouseUp:(NSEvent*)theEvent {
  if ([draggableButtonImpl_ mouseUpImpl:theEvent] ==
          kDraggableButtonMixinCallSuper) {
    [super mouseUp:theEvent];
  }
}

- (void)mouseDown:(NSEvent*)theEvent {
  // The impl spins an event loop to distinguish clicks from drags,
  // which could result in our destruction.  Wire ourselves down for
  // the duration.
  base::scoped_nsobject<DraggableButton> keepAlive([self retain]);

  if ([draggableButtonImpl_ mouseDownImpl:theEvent] ==
          kDraggableButtonMixinCallSuper) {
    // Hack to suppress a crash. See http://crbug.com/509833 for details.
    if ([self window] && ![self isHiddenOrHasHiddenAncestor])
      [super mouseDown:theEvent];
  }
}

- (void)beginDrag:(NSEvent*)dragEvent {
  // Must be overridden by subclasses.
  NOTREACHED();
}

@end  // @interface DraggableButton
