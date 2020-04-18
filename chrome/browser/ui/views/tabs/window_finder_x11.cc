// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/views/tabs/window_finder.h"

#include "chrome/browser/ui/views/tabs/window_finder_mus.h"
#include "ui/display/display.h"
#include "ui/display/screen.h"
#include "ui/gfx/geometry/point_conversions.h"
#include "ui/views/widget/desktop_aura/x11_topmost_window_finder.h"

namespace {

float GetDeviceScaleFactor() {
  return display::Screen::GetScreen()
      ->GetPrimaryDisplay()
      .device_scale_factor();
}

gfx::Point DIPToPixelPoint(const gfx::Point& dip_point) {
  return gfx::ScaleToFlooredPoint(dip_point, GetDeviceScaleFactor());
}

}  // anonymous namespace

gfx::NativeWindow WindowFinder::GetLocalProcessWindowAtPoint(
    const gfx::Point& screen_point,
    const std::set<gfx::NativeWindow>& ignore) {
  gfx::NativeWindow mus_result = nullptr;
  if (GetLocalProcessWindowAtPointMus(screen_point, ignore, &mus_result))
    return mus_result;

  // The X11 server is the canonical state of what the window stacking order is.
  views::X11TopmostWindowFinder finder;
  return finder.FindLocalProcessWindowAt(DIPToPixelPoint(screen_point), ignore);
}
