// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UI_MESSAGE_CENTER_COCOA_OPAQUE_VIEWS_H_
#define UI_MESSAGE_CENTER_COCOA_OPAQUE_VIEWS_H_

#import <Cocoa/Cocoa.h>

#include "base/mac/scoped_nsobject.h"

// MCDropDown is the same as an NSPopupButton except that it fills its
// background with a settable color.
@interface MCDropDown : NSPopUpButton {
 @private
  base::scoped_nsobject<NSColor> backgroundColor_;
}

// Gets and sets the bubble's background color.
- (NSColor*)backgroundColor;
- (void)setBackgroundColor:(NSColor*)backgroundColor;
@end

// MCTextField fills its background with an opaque color.  It also configures
// the view to have a plan appearance, without bezel, border, editing, etc.
@interface MCTextField : NSTextField {
 @private
  base::scoped_nsobject<NSColor> backgroundColor_;
}

// Use this method to create the text field.  The color is required so it
// can correctly subpixel antialias.
- (id)initWithFrame:(NSRect)frameRect backgroundColor:(NSColor*)color;
@end

#endif  // UI_MESSAGE_CENTER_COCOA_OPAQUE_VIEWS_H_
