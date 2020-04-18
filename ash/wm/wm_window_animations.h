// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ASH_WM_WM_WINDOW_ANIMATIONS_H_
#define ASH_WM_WM_WINDOW_ANIMATIONS_H_

#include "ash/ash_export.h"

namespace ui {
class Layer;
}

// This is only for animations specific to Ash. For window animations shared
// with desktop Chrome, see ui/views/corewm/window_animations.h.
namespace ash {

// Direction for ash-specific window animations used in workspaces and
// lock/unlock animations.
enum LayerScaleAnimationDirection {
  LAYER_SCALE_ANIMATION_ABOVE,
  LAYER_SCALE_ANIMATION_BELOW,
};

// Applies scale related to the specified AshWindowScaleType.
ASH_EXPORT void SetTransformForScaleAnimation(
    ui::Layer* layer,
    LayerScaleAnimationDirection type);

}  // namespace ash

#endif  // ASH_WM_WM_WINDOW_ANIMATIONS_H_
