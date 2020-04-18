// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ui/base/cocoa/controls/hover_image_menu_button_cell.h"

@implementation HoverImageMenuButtonCell

@synthesize hovered = hovered_;

- (id)initTextCell:(NSString*)stringValue
         pullsDown:(BOOL)pullDown {
  if ((self = [super initTextCell:stringValue
                        pullsDown:pullDown])) {
    [self setUsesItemFromMenu:NO];
  }
  return self;
}

- (void)setHoverImage:(NSImage*)newImage {
  if ([hoverImage_ isEqual:newImage])
    return;

  hoverImage_.reset([newImage retain]);
  if (hovered_)
    [[self controlView] setNeedsDisplay:YES];
}

- (NSImage*)hoverImage {
  return hoverImage_;
}

- (void)setHovered:(BOOL)hovered {
  if (hovered_ == hovered)
    return;

  hovered_ = hovered;
  [[self controlView] setNeedsDisplay:YES];
}

- (NSImage*)imageToDraw {
  if ([self isHighlighted] && [self alternateImage])
    return [self alternateImage];

  if ([self isHovered] && [self hoverImage])
    return [self hoverImage];

  // Note that NSPopUpButtonCell updates the cell image when the [self menuItem]
  // changes.
  return [self image];
}

- (void)setDefaultImage:(NSImage*)defaultImage {
  base::scoped_nsobject<NSMenuItem> buttonMenuItem([[NSMenuItem alloc] init]);
  [buttonMenuItem setImage:defaultImage];
  [self setMenuItem:buttonMenuItem];
}

- (void)drawWithFrame:(NSRect)cellFrame
               inView:(NSView*)controlView {
  [[self imageToDraw] drawInRect:cellFrame
                        fromRect:NSZeroRect
                       operation:NSCompositeSourceOver
                        fraction:1.0
                  respectFlipped:YES
                           hints:nil];
}

@end
