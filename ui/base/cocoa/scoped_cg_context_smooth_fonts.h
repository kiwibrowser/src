// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UI_BASE_COCOA_SCOPED_CG_CONTEXT_SMOOTH_FONTS_H_
#define UI_BASE_COCOA_SCOPED_CG_CONTEXT_SMOOTH_FONTS_H_

#include "base/macros.h"
#include "ui/base/ui_base_export.h"
#include "ui/gfx/scoped_ns_graphics_context_save_gstate_mac.h"

namespace ui {

// Ensures LCD font smoothing is enabled before drawing text. This allows Cocoa
// drawing code to override a decision made by AppKit to disable font smoothing.
// E.g. this occurs when a view is layer-backed or, since 10.8, when a view
// returns NO from -[NSView isOpaque]. For this to look nice, there must be an
// opaque background already drawn in the graphics context at the location of
// the text (but it doesn't need to fill the view bounds, which is required when
// -[NSView isOpaque] returns YES).
class UI_BASE_EXPORT ScopedCGContextSmoothFonts {
 public:
  ScopedCGContextSmoothFonts();
  ~ScopedCGContextSmoothFonts();

 private:
  gfx::ScopedNSGraphicsContextSaveGState save_state_;

  DISALLOW_COPY_AND_ASSIGN(ScopedCGContextSmoothFonts);
};

}  // namespace ui

#endif  // UI_BASE_COCOA_SCOPED_CG_CONTEXT_SMOOTH_FONTS_H_
