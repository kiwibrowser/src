// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UI_BASE_COCOA_CONTROLS_HOVER_IMAGE_MENU_BUTTON_CELL_H_
#define UI_BASE_COCOA_CONTROLS_HOVER_IMAGE_MENU_BUTTON_CELL_H_

#import <Cocoa/Cocoa.h>

#include "base/mac/scoped_nsobject.h"
#include "ui/base/ui_base_export.h"

// A custom NSPopUpButtonCell that permits a hover image, and draws only an
// image in its frame; no border, bezel or drop-down arrow. Use setDefaultImage:
// to set the default image, setAlternateImage: to set the button shown while
// the menu is active, and setHoverImage: for the mouseover hover image.
UI_BASE_EXPORT
@interface HoverImageMenuButtonCell : NSPopUpButtonCell {
 @private
  base::scoped_nsobject<NSImage> hoverImage_;
  BOOL hovered_;
}

@property(retain, nonatomic) NSImage* hoverImage;
@property(assign, nonatomic, getter=isHovered) BOOL hovered;

// Return the image that would be drawn based on the current state flags.
- (NSImage*)imageToDraw;

// Set the default image to show on the menu button.
- (void)setDefaultImage:(NSImage*)defaultImage;

@end

#endif  // UI_BASE_COCOA_CONTROLS_HOVER_IMAGE_MENU_BUTTON_CELL_H_
