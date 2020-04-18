// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/keyboard/container_fullscreen_behavior.h"

namespace keyboard {

ContainerFullscreenBehavior::ContainerFullscreenBehavior(
    KeyboardController* controller)
    : ContainerFullWidthBehavior(controller) {}

ContainerFullscreenBehavior::~ContainerFullscreenBehavior() {}

gfx::Rect ContainerFullscreenBehavior::AdjustSetBoundsRequest(
    const gfx::Rect& display_bounds,
    const gfx::Rect& requested_bounds_in_screen_coords) {
  return display_bounds;
}

void ContainerFullscreenBehavior::SetCanonicalBounds(
    aura::Window* container,
    const gfx::Rect& display_bounds) {
  container->SetBounds(display_bounds);
}

gfx::Rect ContainerFullscreenBehavior::GetOccludedBounds(
    const gfx::Rect& visual_bounds_in_screen) const {
  // TODO(https://crbug.com/826617): Get occluded bounds from IME.
  NOTIMPLEMENTED_LOG_ONCE();
  return {};
}

ContainerType ContainerFullscreenBehavior::GetType() const {
  return ContainerType::FULLSCREEN;
}

}  //  namespace keyboard
