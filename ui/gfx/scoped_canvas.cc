// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/gfx/scoped_canvas.h"

#include "base/i18n/rtl.h"
#include "ui/gfx/geometry/rect.h"

namespace gfx {

ScopedRTLFlipCanvas::ScopedRTLFlipCanvas(gfx::Canvas* canvas,
                                         int width,
                                         bool flip)
    : canvas_(canvas) {
  if (flip && base::i18n::IsRTL()) {
    canvas->Translate(gfx::Vector2d(width, 0));
    canvas->Scale(-1, 1);
  }
}

}  // namespace gfx
