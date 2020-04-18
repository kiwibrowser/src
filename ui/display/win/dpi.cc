// Copyright 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/display/win/dpi.h"

#include <windows.h>

#include "base/win/scoped_hdc.h"
#include "ui/display/display.h"

namespace display {
namespace win {

namespace {

const float kDefaultDPI = 96.f;

float g_device_scale_factor = 0.f;

gfx::Size GetDPI() {
  static int dpi_x = 0;
  static int dpi_y = 0;
  static bool should_initialize = true;

  if (should_initialize) {
    should_initialize = false;
    base::win::ScopedGetDC screen_dc(NULL);
    // This value is safe to cache for the life time of the app since the
    // user must logout to change the DPI setting. This value also applies
    // to all screens.
    dpi_x = GetDeviceCaps(screen_dc, LOGPIXELSX);
    dpi_y = GetDeviceCaps(screen_dc, LOGPIXELSY);
  }
  return gfx::Size(dpi_x, dpi_y);
}

}  // namespace

void SetDefaultDeviceScaleFactor(float scale) {
  DCHECK_NE(0.f, scale);
  g_device_scale_factor = scale;
}

float GetDPIScale() {
  if (Display::HasForceDeviceScaleFactor())
    return Display::GetForcedDeviceScaleFactor();
  return GetUnforcedDeviceScaleFactor();
}

float GetUnforcedDeviceScaleFactor() {
  return g_device_scale_factor ? g_device_scale_factor
                               : GetScalingFactorFromDPI(GetDPI().width());
}

int GetDPIFromScalingFactor(float device_scaling_factor) {
  return kDefaultDPI * device_scaling_factor;
}

float GetScalingFactorFromDPI(int dpi) {
  return static_cast<float>(dpi) / kDefaultDPI;
}

}  // namespace win
}  // namespace display
