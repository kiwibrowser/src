// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/android/edge_effect_base.h"

namespace ui {

// static
gfx::Transform EdgeEffectBase::ComputeTransform(Edge edge,
                                                const gfx::SizeF& viewport_size,
                                                float offset) {
  // Transforms assume the edge layers are anchored to their *top center point*.
  switch (edge) {
    case EDGE_TOP:
      return gfx::Transform(1, 0, 0, 1, 0, offset);
    case EDGE_LEFT:
      return gfx::Transform(0, 1, -1, 0, -viewport_size.height() / 2.f + offset,
                            viewport_size.height() / 2.f);
    case EDGE_BOTTOM:
      return gfx::Transform(-1, 0, 0, -1, 0, viewport_size.height() + offset);
    case EDGE_RIGHT:
      return gfx::Transform(0, -1, 1, 0, -viewport_size.height() / 2.f +
                                             viewport_size.width() + offset,
                            viewport_size.height() / 2.f);
    default:
      NOTREACHED() << "Invalid edge: " << edge;
      return gfx::Transform();
  };
}

// static
gfx::SizeF EdgeEffectBase::ComputeOrientedSize(
    Edge edge,
    const gfx::SizeF& viewport_size) {
  switch (edge) {
    case EDGE_TOP:
    case EDGE_BOTTOM:
      return viewport_size;
    case EDGE_LEFT:
    case EDGE_RIGHT:
      return gfx::SizeF(viewport_size.height(), viewport_size.width());
    default:
      NOTREACHED() << "Invalid edge: " << edge;
      return gfx::SizeF();
  };
}

}  // namespace ui
