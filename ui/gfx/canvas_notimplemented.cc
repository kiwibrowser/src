// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/gfx/canvas.h"

#include "base/logging.h"

namespace gfx {

// static
void Canvas::SizeStringFloat(const base::string16& text,
                             const FontList& font_list,
                             float* width,
                             float* height,
                             int line_height,
                             int flags,
                             Typesetter Typesetter) {
  NOTIMPLEMENTED();
}

void Canvas::DrawStringRectWithFlags(const base::string16& text,
                                     const FontList& font_list,
                                     SkColor color,
                                     const Rect& text_bounds,
                                     int flags) {
  NOTIMPLEMENTED();
}

void Canvas::DrawFadedString(const base::string16& text,
                             const FontList& font_list,
                             SkColor color,
                             const Rect& display_rect,
                             int flags) {
   NOTIMPLEMENTED();
}

}  // namespace gfx
