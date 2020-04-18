// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chromecast/graphics/cast_screen.h"

#include <stdint.h>

#include "ui/aura/env.h"
#include "ui/display/display.h"

namespace chromecast {

CastScreen::~CastScreen() {
}

gfx::Point CastScreen::GetCursorScreenPoint() {
  return aura::Env::GetInstance()->last_mouse_location();
}

bool CastScreen::IsWindowUnderCursor(gfx::NativeWindow window) {
  NOTIMPLEMENTED();
  return false;
}

gfx::NativeWindow CastScreen::GetWindowAtScreenPoint(const gfx::Point& point) {
  return gfx::NativeWindow(nullptr);
}

display::Display CastScreen::GetDisplayNearestWindow(
    gfx::NativeWindow window) const {
  return GetPrimaryDisplay();
}

CastScreen::CastScreen() {
}

void CastScreen::OnDisplayChanged(int64_t display_id,
                                  float device_scale_factor,
                                  display::Display::Rotation rotation,
                                  const gfx::Rect& bounds) {
  display::Display display(display_id);
  display.SetScaleAndBounds(device_scale_factor, bounds);
  display.set_rotation(rotation);
  VLOG(1) << __func__ << " " << display.ToString();
  ProcessDisplayChanged(display, true /* is_primary */);
}

}  // namespace chromecast
