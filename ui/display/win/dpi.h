// Copyright 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UI_DISPLAY_WIN_DPI_H_
#define UI_DISPLAY_WIN_DPI_H_

#include "ui/display/display_export.h"
#include "ui/gfx/geometry/size.h"

namespace display {
namespace win {

// Deprecated. Use --force-device-scale-factor instead.
//
// Sets the device scale factor that will be used unless overridden on the
// command line by --force-device-scale-factor.
DISPLAY_EXPORT void SetDefaultDeviceScaleFactor(float scale);

// Deprecated. Use win::ScreenWin::GetScaleFactorForHWND instead.
//
// Gets the system's scale factor. For example, if the system DPI is 96 then the
// scale factor is 1.0. This does not handle per-monitor DPI.
DISPLAY_EXPORT float GetDPIScale();

// Equivalent to GetDPIScale() but ignores the --force-device-scale-factor flag.
float GetUnforcedDeviceScaleFactor();

// Returns the equivalent DPI for |device_scaling_factor|.
DISPLAY_EXPORT int GetDPIFromScalingFactor(float device_scaling_factor);

// Returns the equivalent scaling factor for |dpi|.
DISPLAY_EXPORT float GetScalingFactorFromDPI(int dpi);

}  // namespace win
}  // namespace display

#endif  // UI_DISPLAY_WIN_DPI_H_
