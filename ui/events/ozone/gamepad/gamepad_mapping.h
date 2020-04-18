// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UI_EVENTS_OZONE_GAMEPAD_GAMEPAD_MAPPING_H_
#define UI_EVENTS_OZONE_GAMEPAD_GAMEPAD_MAPPING_H_

#include <memory>

#include "ui/events/ozone/gamepad/webgamepad_constants.h"

namespace ui {

class EventDeviceInfo;

// The following HATX and HATY is not part of web gamepad definition, but we
// need to specially treat them cause HAT_Y can be mapped to DPAD_UP or
// DPAD_DOWN, and HAT_X can be mapped to DAPD_LEFT or DPAD_RIGHT.
constexpr int kHAT_X = 4;
constexpr int kHAT_Y = 5;

// KeyMap maps evdev key code to web gamepad code.
struct KeyMapEntry {
  uint16_t evdev_code;
  uint16_t mapped_code;
};

// AbsMap maps evdev abs code to web gamepad (type, code).
struct AbsMapEntry {
  uint16_t evdev_code;
  GamepadEventType mapped_type;
  uint16_t mapped_code;
};

using KeyMapType = const KeyMapEntry[];
using AbsMapType = const AbsMapEntry[];

#define TO_BTN(code, mapped_code) \
  { code, GamepadEventType::BUTTON, mapped_code }

#define TO_ABS(code, mapped_code) \
  { code, GamepadEventType::AXIS, mapped_code }

class GamepadMapper {
 public:
  virtual bool Map(uint16_t key,
                   uint16_t code,
                   GamepadEventType* mapped_type,
                   uint16_t* mapped_code) const = 0;

  virtual ~GamepadMapper() {}
};

// This function gets the best mapper for the gamepad vendor_id and product_id.
std::unique_ptr<GamepadMapper> GetGamepadMapper(const EventDeviceInfo& devinfo);

}  // namespace ui

#endif  // UI_EVENTS_OZONE_GAMEPAD_GAMEPAD_MAPPING_H_
