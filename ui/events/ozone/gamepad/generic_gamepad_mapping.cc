// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <linux/input.h>
#include <algorithm>
#include <bitset>
#include <cstdint>
#include <list>
#include <set>
#include <vector>

#include "base/macros.h"
#include "ui/events/ozone/evdev/event_device_info.h"
#include "ui/events/ozone/gamepad/generic_gamepad_mapping.h"
#include "ui/events/ozone/gamepad/webgamepad_constants.h"

namespace {
using ui::GamepadEventType;

class GenericGamepadMapper : public ui::GamepadMapper {
 public:
  GenericGamepadMapper(std::vector<ui::AbsMapEntry> axis_mapping,
                       std::vector<ui::KeyMapEntry> button_mapping) {
    axis_mapping.swap(axis_mapping_);
    button_mapping.swap(button_mapping_);
  }

  bool Map(uint16_t type,
           uint16_t code,
           ui::GamepadEventType* mapped_type,
           uint16_t* mapped_code) const override {
    if (type == EV_KEY) {
      for (auto entry : button_mapping_) {
        if (entry.evdev_code == code) {
          *mapped_type = ui::GamepadEventType::BUTTON;
          *mapped_code = entry.mapped_code;
          return true;
        }
      }
      return false;
    }

    if (type == EV_ABS) {
      for (auto entry : axis_mapping_) {
        if (entry.evdev_code == code) {
          *mapped_type = entry.mapped_type;
          *mapped_code = entry.mapped_code;
          return true;
        }
      }
      return false;
    }
    return false;
  }

  ~GenericGamepadMapper() override {}

 private:
  std::vector<ui::AbsMapEntry> axis_mapping_;
  std::vector<ui::KeyMapEntry> button_mapping_;
};

// This helper class will be used to build generic mapping.
class GamepadMapperBuilder {
 public:
  explicit GamepadMapperBuilder(const ui::EventDeviceInfo& devinfo)
      : devinfo_(devinfo) {}

  std::unique_ptr<ui::GamepadMapper> ToGamepadMapper() {
    return std::make_unique<GenericGamepadMapper>(std::move(axis_mapping_),
                                                  std::move(button_mapping_));
  }

  void MapButton(uint16_t from_button, uint16_t mapped_button) {
    if (!devinfo_.HasKeyEvent(from_button)) {
      return;
    }
    DCHECK(!IfEvdevButtonMappedFrom(from_button));
    DCHECK(!IfWebgamepadButtonMappedTo(mapped_button));

    button_mapping_.push_back({from_button, mapped_button});
    evdev_buttons_.set(from_button);
    webgamepad_buttons_.set(mapped_button);
  }

  void MapAxisToButton(uint16_t from_axis, uint16_t mapped_button) {
    if (!devinfo_.HasAbsEvent(from_axis)) {
      return;
    }
    DCHECK(!IfEvdevAxisMappedFrom(from_axis));
    evdev_axes_.set(from_axis);
    axis_mapping_.push_back(TO_BTN(from_axis, mapped_button));

    if (mapped_button == ui::kHAT_X) {
      DCHECK(!IfWebgamepadButtonMappedTo(ui::WG_BUTTON_DPAD_LEFT));
      DCHECK(!IfWebgamepadButtonMappedTo(ui::WG_BUTTON_DPAD_RIGHT));

      webgamepad_buttons_.set(ui::WG_BUTTON_DPAD_LEFT);
      webgamepad_buttons_.set(ui::WG_BUTTON_DPAD_RIGHT);
    } else if (mapped_button == ui::kHAT_Y) {
      DCHECK(!IfWebgamepadButtonMappedTo(ui::WG_BUTTON_DPAD_UP));
      DCHECK(!IfWebgamepadButtonMappedTo(ui::WG_BUTTON_DPAD_DOWN));

      webgamepad_buttons_.set(ui::WG_BUTTON_DPAD_UP);
      webgamepad_buttons_.set(ui::WG_BUTTON_DPAD_DOWN);
    } else {
      DCHECK(!IfWebgamepadButtonMappedTo(mapped_button));
      webgamepad_buttons_.set(mapped_button);
    }
  }

  void MapAxisToAxis(uint16_t from_axis, uint16_t mapped_axis) {
    if (!devinfo_.HasAbsEvent(from_axis)) {
      return;
    }
    DCHECK(!IfEvdevAxisMappedFrom(from_axis));
    DCHECK(!IfWebgamepadAxisMappedTo(mapped_axis));

    axis_mapping_.push_back(TO_ABS(from_axis, mapped_axis));
    evdev_axes_.set(from_axis);
    webgamepad_axes_.set(mapped_axis);
  }

  void MapButtonLikeJoydev() {
    uint16_t next_unmapped_button = 0;
    // In linux kernel, joydev.c map evdev events in the same way.
    for (int i = BTN_JOYSTICK - BTN_MISC; i < KEY_MAX - BTN_MISC + 1; i++) {
      int code = i + BTN_MISC;
      if (devinfo_.HasKeyEvent(code) && !IfEvdevButtonMappedFrom(code) &&
          FindNextUnmappedCode(webgamepad_buttons_, &next_unmapped_button)) {
        MapButton(code, next_unmapped_button);
      }
    }

    for (int i = 0; i < BTN_JOYSTICK - BTN_MISC; i++) {
      int code = i + BTN_MISC;
      if (devinfo_.HasKeyEvent(code) && !IfEvdevButtonMappedFrom(code) &&
          FindNextUnmappedCode(webgamepad_buttons_, &next_unmapped_button)) {
        MapButton(code, next_unmapped_button);
      }
    }
  }

  void MapAxisLikeJoydev() {
    uint16_t next_unmapped_axis = 0;
    for (int code = 0; code < ABS_CNT; ++code) {
      if (devinfo_.HasAbsEvent(code) && !IfEvdevAxisMappedFrom(code) &&
          FindNextUnmappedCode(webgamepad_axes_, &next_unmapped_axis)) {
        MapAxisToAxis(code, next_unmapped_axis);
      }
    }
  }

 private:
  // This function helps to find the next unmapped button or axis. Code is the
  // last unmapped code and will be the next unmapped code when the function
  // returns.
  template <typename T>
  bool FindNextUnmappedCode(const T& bitset, uint16_t* code) {
    for (uint16_t i = *code; i < bitset.size(); ++i) {
      if (!bitset.test(i)) {
        *code = i;
        return true;
      }
    }
    return false;
  }

  bool IfEvdevButtonMappedFrom(uint16_t code) {
    DCHECK_LT(code, KEY_CNT);
    return evdev_buttons_.test(code);
  }

  bool IfEvdevAxisMappedFrom(uint16_t code) {
    DCHECK_LT(code, ABS_CNT);
    return evdev_axes_.test(code);
  }

  bool IfWebgamepadButtonMappedTo(uint16_t code) {
    DCHECK_LT(code, ui::WG_BUTTON_COUNT);
    return webgamepad_buttons_.test(code);
  }

  bool IfWebgamepadAxisMappedTo(uint16_t code) {
    DCHECK_LT(code, ui::WG_ABS_COUNT);
    return webgamepad_axes_.test(code);
  }

  const ui::EventDeviceInfo& devinfo_;

  // Mapped webgamepad buttons and axes will be marked as true.
  std::bitset<ui::WG_BUTTON_COUNT> webgamepad_buttons_;
  std::bitset<ui::WG_ABS_COUNT> webgamepad_axes_;
  // Evdev buttons and axes that are already mapped will be marked as true.
  std::bitset<KEY_CNT> evdev_buttons_;
  std::bitset<ABS_CNT> evdev_axes_;

  // Generated Mapping.
  std::vector<ui::AbsMapEntry> axis_mapping_;
  std::vector<ui::KeyMapEntry> button_mapping_;
};

void MapSpecialButtons(GamepadMapperBuilder* builder) {
  // Map mode seperately.
  builder->MapButton(BTN_MODE, ui::WG_BUTTON_MODE);
  // Take care of ADT style back and start.
  builder->MapButton(KEY_BACK, ui::WG_BUTTON_SELECT);
  builder->MapButton(KEY_HOMEPAGE, ui::WG_BUTTON_START);
}

void MapSpecialAxes(const ui::EventDeviceInfo& devinfo,
                    GamepadMapperBuilder* builder) {
  // HAT0X and HAT0Y always map to DPAD.
  builder->MapAxisToButton(ABS_HAT0X, ui::kHAT_X);
  builder->MapAxisToButton(ABS_HAT0Y, ui::kHAT_Y);

  // When ABS_BRAKE and ABS_GAS supported at the same time, they are mapped to
  // l-trigger and r-trigger.
  // Otherwise, when x,y,z,rx,ry,rz are all supported, z and rz are mapped to
  // triggers (As XInput gamepad mapper).
  if (devinfo.HasAbsEvent(ABS_BRAKE) && devinfo.HasAbsEvent(ABS_GAS)) {
    builder->MapAxisToButton(ABS_BRAKE, ui::WG_BUTTON_LT);
    builder->MapAxisToButton(ABS_GAS, ui::WG_BUTTON_RT);
  } else if (devinfo.HasAbsEvent(ABS_X) && devinfo.HasAbsEvent(ABS_Y) &&
             devinfo.HasAbsEvent(ABS_Z) && devinfo.HasAbsEvent(ABS_RX) &&
             devinfo.HasAbsEvent(ABS_RY) && devinfo.HasAbsEvent(ABS_RZ)) {
    builder->MapAxisToButton(ABS_Z, ui::WG_BUTTON_LT);
    builder->MapAxisToButton(ABS_RZ, ui::WG_BUTTON_RT);
  }
}
}  // namespace

namespace ui {
std::unique_ptr<GamepadMapper> BuildGenericGamepadMapper(
    const EventDeviceInfo& info) {
  GamepadMapperBuilder builder(info);
  // Must map axes first as axis might be mapped to button and occupy some
  // webgamepad button slots.
  MapSpecialAxes(info, &builder);
  builder.MapAxisLikeJoydev();

  MapSpecialButtons(&builder);
  builder.MapButtonLikeJoydev();

  return builder.ToGamepadMapper();
}

}  // namespace ui
