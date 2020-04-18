// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/events/ozone/gamepad/gamepad_mapping.h"

#include <memory>

#include "ui/events/ozone/evdev/event_device_info.h"
#include "ui/events/ozone/gamepad/generic_gamepad_mapping.h"
#include "ui/events/ozone/gamepad/static_gamepad_mapping.h"

namespace ui {

std::unique_ptr<GamepadMapper> GetGamepadMapper(
    const EventDeviceInfo& devinfo) {
  std::unique_ptr<GamepadMapper> result(
      GetStaticGamepadMapper(devinfo.vendor_id(), devinfo.product_id()));
  if (!result) {
    return BuildGenericGamepadMapper(devinfo);
  }
  return result;
}

}  // namespace ui
