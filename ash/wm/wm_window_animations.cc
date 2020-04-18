// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ash/wm/wm_window_animations.h"

#include "ui/compositor/layer.h"
#include "ui/gfx/transform.h"

namespace ash {

void SetTransformForScaleAnimation(ui::Layer* layer,
                                   LayerScaleAnimationDirection type) {
  // Scales for windows above and below the current workspace.
  const float kLayerScaleAboveSize = 1.1f;
  const float kLayerScaleBelowSize = .9f;

  const float scale = type == LAYER_SCALE_ANIMATION_ABOVE
                          ? kLayerScaleAboveSize
                          : kLayerScaleBelowSize;
  gfx::Transform transform;
  transform.Translate(-layer->bounds().width() * (scale - 1.0f) / 2,
                      -layer->bounds().height() * (scale - 1.0f) / 2);
  transform.Scale(scale, scale);
  layer->SetTransform(transform);
}

}  // namespace ash
