// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/views/window/window_shape.h"

#include "ui/gfx/geometry/size.h"
#include "ui/gfx/path.h"

namespace views {

void GetDefaultWindowMask(const gfx::Size& size, float scale,
                          gfx::Path* window_mask) {
  const SkScalar sk_scale = SkFloatToScalar(scale);
  const SkScalar width = SkIntToScalar(size.width()) / sk_scale;
  const SkScalar height = SkIntToScalar(size.height()) / sk_scale;

  window_mask->moveTo(0, 3);
  window_mask->lineTo(1, 3);
  window_mask->lineTo(1, 1);
  window_mask->lineTo(3, 1);
  window_mask->lineTo(3, 0);

  window_mask->lineTo(width - 3, 0);
  window_mask->lineTo(width - 3, 1);
  window_mask->lineTo(width - 1, 1);
  window_mask->lineTo(width - 1, 3);
  window_mask->lineTo(width, 3);

  window_mask->lineTo(width, height - 3);
  window_mask->lineTo(width - 1, height - 3);
  window_mask->lineTo(width - 1, height - 1);
  window_mask->lineTo(width - 3, height - 1);
  window_mask->lineTo(width - 3, height);

  window_mask->lineTo(3, height);
  window_mask->lineTo(3, height - 1);
  window_mask->lineTo(1, height - 1);
  window_mask->lineTo(1, height - 3);
  window_mask->lineTo(0, height - 3);

  window_mask->close();

  SkMatrix m;
  m.setScale(sk_scale, sk_scale);
  window_mask->transform(m);
}

} // namespace views
