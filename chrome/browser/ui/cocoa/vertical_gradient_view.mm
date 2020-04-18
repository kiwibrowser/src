// Copyright (c) 2010 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/cocoa/vertical_gradient_view.h"

@implementation VerticalGradientView

- (NSGradient*)gradient {
  return gradient_;
}

- (void)setGradient:(NSGradient*)gradient {
  gradient_.reset([gradient retain]);
}

- (NSColor*)strokeColor {
  return strokeColor_;
}

- (void)setStrokeColor:(NSColor*)strokeColor {
  strokeColor_.reset([strokeColor retain]);
}

- (void)drawRect:(NSRect)rect {
  // Draw gradient.
  [[self gradient] drawInRect:[self bounds] angle:270];

  // Draw bottom stroke.
  NSColor* strokeColor = [self strokeColor];
  if (strokeColor) {
    [[self strokeColor] set];
    NSRect borderRect, contentRect;
    NSDivideRect([self bounds], &borderRect, &contentRect, 1, NSMinYEdge);
    NSRectFillUsingOperation(borderRect, NSCompositeSourceOver);
  }
}

@end
