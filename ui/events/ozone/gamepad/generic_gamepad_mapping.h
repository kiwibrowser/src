// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UI_EVENTS_OZONE_GENERIC_GAMEPAD_GAMEPAD_MAPPING_H_
#define UI_EVENTS_OZONE_GENERIC_GAMEPAD_GAMEPAD_MAPPING_H_

#include <vector>
#include "ui/events/ozone/evdev/events_ozone_evdev_export.h"
#include "ui/events/ozone/gamepad/gamepad_mapping.h"

namespace ui {

class EventDeviceInfo;

std::unique_ptr<GamepadMapper> EVENTS_OZONE_EVDEV_EXPORT
BuildGenericGamepadMapper(const EventDeviceInfo& info);

}  // namespace ui

#endif  // UI_EVENTS_OZONE_GENERIC_GAMEPAD_GAMEPAD_MAPPING_H_
