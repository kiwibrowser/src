// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UI_BASE_COCOA_HOVER_IMAGE_BUTTON_H_
#define UI_BASE_COCOA_HOVER_IMAGE_BUTTON_H_

#import <Cocoa/Cocoa.h>

#import "base/mac/scoped_nsobject.h"
#import "ui/base/cocoa/hover_button.h"
#include "ui/base/ui_base_export.h"

// A button that changes images when you hover over it and click it.
UI_BASE_EXPORT
@interface HoverImageButton : HoverButton {
 @private
  base::scoped_nsobject<NSImage> defaultImage_;
  base::scoped_nsobject<NSImage> hoverImage_;
  base::scoped_nsobject<NSImage> pressedImage_;
}

// Disables a click within the button from activating the application.
@property(nonatomic) BOOL disableActivationOnClick;

// Sets the default image.
- (void)setDefaultImage:(NSImage*)image;

// Sets the hover image.
- (void)setHoverImage:(NSImage*)image;

// Sets the pressed image.
- (void)setPressedImage:(NSImage*)image;

@end

#endif  // UI_BASE_COCOA_HOVER_IMAGE_BUTTON_H_
