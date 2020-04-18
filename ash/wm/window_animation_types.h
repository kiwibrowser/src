// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ASH_WM_WINDOW_ANIMATION_TYPES_H_
#define ASH_WM_WINDOW_ANIMATION_TYPES_H_

#include "ui/wm/core/window_animations.h"

namespace ash {
namespace wm {

// An extension of the window animations provided by CoreWm. These are
// Ash-specific only.
enum WindowVisibilityAnimationType {
  // Window scale/rotates down to its launcher icon.
  WINDOW_VISIBILITY_ANIMATION_TYPE_MINIMIZE =
      ::wm::WINDOW_VISIBILITY_ANIMATION_MAX,
  // Fade in/out using brightness and grayscale web filters.
  WINDOW_VISIBILITY_ANIMATION_TYPE_BRIGHTNESS_GRAYSCALE
};

}  // namespace wm
}  // namespace ash

#endif  // ASH_WM_WINDOW_ANIMATION_TYPES_H_
