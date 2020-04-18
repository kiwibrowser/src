// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/display/win/display_info.h"

#include "base/hash.h"
#include "base/strings/utf_string_conversions.h"

namespace display {
namespace {

Display::Rotation GetRotationForDevice(const wchar_t* device_name) {
  DEVMODE mode;
  ::ZeroMemory(&mode, sizeof(mode));
  mode.dmSize = sizeof(mode);
  mode.dmDriverExtra = 0;
  if (::EnumDisplaySettings(device_name, ENUM_CURRENT_SETTINGS, &mode)) {
    switch (mode.dmDisplayOrientation) {
    case DMDO_DEFAULT:
      return Display::ROTATE_0;
    case DMDO_90:
      return Display::ROTATE_90;
    case DMDO_180:
      return Display::ROTATE_180;
    case DMDO_270:
      return Display::ROTATE_270;
    default:
      NOTREACHED();
    }
  }
  return Display::ROTATE_0;
}

}  // namespace

namespace win {

DisplayInfo::DisplayInfo(const MONITORINFOEX& monitor_info,
                         float device_scale_factor,
                         float sdr_white_level)
    : DisplayInfo(monitor_info,
                  device_scale_factor,
                  sdr_white_level,
                  GetRotationForDevice(monitor_info.szDevice)) {}

DisplayInfo::DisplayInfo(const MONITORINFOEX& monitor_info,
                         float device_scale_factor,
                         float sdr_white_level,
                         Display::Rotation rotation)
    : id_(DeviceIdFromDeviceName(monitor_info.szDevice)),
      rotation_(rotation),
      screen_rect_(monitor_info.rcMonitor),
      screen_work_rect_(monitor_info.rcWork),
      device_scale_factor_(device_scale_factor),
      sdr_white_level_(sdr_white_level) {}

// static
int64_t DisplayInfo::DeviceIdFromDeviceName(const wchar_t* device_name) {
  return static_cast<int64_t>(base::Hash(base::WideToUTF8(device_name)));
}

}  // namespace win
}  // namespace display
