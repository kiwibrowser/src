// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ui/base/cocoa/controls/hover_image_menu_button.h"

#include "base/mac/foundation_util.h"
#import "ui/base/cocoa/controls/hover_image_menu_button_cell.h"

@implementation HoverImageMenuButton

+ (Class)cellClass {
  return [HoverImageMenuButtonCell class];
}

- (id)initWithFrame:(NSRect)frameRect
          pullsDown:(BOOL)flag {
  if ((self = [super initWithFrame:frameRect
                         pullsDown:flag])) {
    trackingArea_.reset(
        [[CrTrackingArea alloc] initWithRect:NSZeroRect
                                     options:NSTrackingInVisibleRect |
                                             NSTrackingMouseEnteredAndExited |
                                             NSTrackingActiveInKeyWindow
                                       owner:self
                                    userInfo:nil]);
    [self addTrackingArea:trackingArea_.get()];
  }
  return self;
}

- (HoverImageMenuButtonCell*)hoverImageMenuButtonCell {
  return base::mac::ObjCCastStrict<HoverImageMenuButtonCell>([self cell]);
}

- (void)mouseEntered:(NSEvent*)theEvent {
  [[self cell] setHovered:YES];
}

- (void)mouseExited:(NSEvent*)theEvent {
  [[self cell] setHovered:NO];
}

@end
