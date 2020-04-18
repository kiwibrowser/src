// Copyright 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/display/screen.h"

#include "ui/display/display.h"
#include "ui/gfx/geometry/rect.h"

namespace display {

namespace {

Screen* g_screen;

}  // namespace

Screen::Screen() {}

Screen::~Screen() {}

// static
Screen* Screen::GetScreen() {
#if defined(OS_MACOSX)
  // TODO(scottmg): https://crbug.com/558054
  if (!g_screen)
    g_screen = CreateNativeScreen();
#endif
  return g_screen;
}

// static
void Screen::SetScreenInstance(Screen* instance) {
  g_screen = instance;
}

Display Screen::GetDisplayNearestView(gfx::NativeView view) const {
  return GetDisplayNearestWindow(GetWindowForView(view));
}

gfx::Rect Screen::ScreenToDIPRectInWindow(gfx::NativeView view,
                                          const gfx::Rect& screen_rect) const {
  float scale = GetDisplayNearestView(view).device_scale_factor();
  return ScaleToEnclosingRect(screen_rect, 1.0f / scale);
}

gfx::Rect Screen::DIPToScreenRectInWindow(gfx::NativeView view,
                                          const gfx::Rect& dip_rect) const {
  float scale = GetDisplayNearestView(view).device_scale_factor();
  return ScaleToEnclosingRect(dip_rect, scale);
}

bool Screen::GetDisplayWithDisplayId(int64_t display_id,
                                     Display* display) const {
  for (const Display& display_in_list : GetAllDisplays()) {
    if (display_in_list.id() == display_id) {
      *display = display_in_list;
      return true;
    }
  }
  return false;
}

}  // namespace display
