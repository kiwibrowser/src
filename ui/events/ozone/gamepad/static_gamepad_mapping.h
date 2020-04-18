// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UI_EVENTS_OZONE_STATIC_GAMEPAD_GAMEPAD_MAPPING_H_
#define UI_EVENTS_OZONE_STATIC_GAMEPAD_GAMEPAD_MAPPING_H_

#include "ui/events/ozone/evdev/events_ozone_evdev_export.h"
#include "ui/events/ozone/gamepad/gamepad_mapping.h"
#include "ui/events/ozone/gamepad/webgamepad_constants.h"

namespace ui {

// This function gets the static mapper for the gamepad vendor_id and
// product_id.
GamepadMapper* EVENTS_OZONE_EVDEV_EXPORT
GetStaticGamepadMapper(uint16_t vendor_id, uint16_t product_id);

}  // namespace ui

#endif  // UI_EVENTS_OZONE_STATIC_GAMEPAD_GAMEPAD_MAPPING_H_
