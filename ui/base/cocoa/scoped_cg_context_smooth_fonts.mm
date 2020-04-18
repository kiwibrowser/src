// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/base/cocoa/scoped_cg_context_smooth_fonts.h"

#import <AppKit/AppKit.h>

namespace ui {

ScopedCGContextSmoothFonts::ScopedCGContextSmoothFonts() {
  NSGraphicsContext* context = [NSGraphicsContext currentContext];
  CGContextRef cg_context = static_cast<CGContextRef>([context graphicsPort]);
  CGContextSetShouldSmoothFonts(cg_context, true);
}

ScopedCGContextSmoothFonts::~ScopedCGContextSmoothFonts() {
}

}  // namespace ui
