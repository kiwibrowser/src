// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_COCOA_VERTICAL_GRADIENT_VIEW_H_
#define CHROME_BROWSER_UI_COCOA_VERTICAL_GRADIENT_VIEW_H_

#include "base/mac/scoped_nsobject.h"

#import <Cocoa/Cocoa.h>

// Draws a vertical background gradient with a bottom stroke. The gradient and
// stroke colors can be defined by calling |setGradient| and |setStrokeColor|,
// respectively. Alternatively, you may override the |gradient| and
// |strokeColor| accessors in order to provide colors dynamically. If the
// gradient or color is |nil|, the respective element will not be drawn.
@interface VerticalGradientView : NSView {
 @private
  // The gradient to draw.
  base::scoped_nsobject<NSGradient> gradient_;
  // Color for bottom stroke.
  base::scoped_nsobject<NSColor> strokeColor_;
}

// Gets and sets the gradient to paint as background.
- (NSGradient*)gradient;
- (void)setGradient:(NSGradient*)gradient;

// Gets and sets the color of the stroke drawn at the bottom of the view.
- (NSColor*)strokeColor;
- (void)setStrokeColor:(NSColor*)gradient;

@end

#endif // CHROME_BROWSER_UI_COCOA_VERTICAL_GRADIENT_VIEW_H_
