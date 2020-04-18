// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ui/base/cocoa/hover_image_button.h"

@implementation HoverImageButton

@synthesize disableActivationOnClick = disableActivationOnClick_;

- (void)drawRect:(NSRect)rect {
  if (hoverState_ == kHoverStateMouseDown && pressedImage_) {
    [super setImage:pressedImage_.get()];
  } else if (hoverState_ == kHoverStateMouseOver && hoverImage_) {
    [super setImage:hoverImage_.get()];
  } else {
    [super setImage:defaultImage_.get()];
  }

  [super drawRect:rect];
}

- (void)setDefaultImage:(NSImage*)image {
  defaultImage_.reset([image retain]);
}

- (void)setHoverImage:(NSImage*)image {
  hoverImage_.reset([image retain]);
}

- (void)setPressedImage:(NSImage*)image {
  pressedImage_.reset([image retain]);
}

- (BOOL)shouldDelayWindowOrderingForEvent:(NSEvent*)theEvent {
  // To avoid activating the app on a click inside the button, first tell
  // the Appkit not to immediately order the HoverImageButton's window front in
  // response to theEvent.
  return disableActivationOnClick_;
}

- (void)mouseDown:(NSEvent*)mouseDownEvent {
  // If disabling activation on click, tell the Appkit to cancel window ordering
  // for this mouse down.
  if (disableActivationOnClick_) {
    [[NSApplication sharedApplication] preventWindowOrdering];
  }

  [super mouseDown:mouseDownEvent];
}

@end
