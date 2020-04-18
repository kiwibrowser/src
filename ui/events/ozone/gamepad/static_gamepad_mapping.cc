// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <linux/input.h>
#include <cstdint>
#include <list>
#include <map>
#include <vector>

#include "base/macros.h"
#include "ui/events/ozone/evdev/event_device_info.h"
#include "ui/events/ozone/gamepad/static_gamepad_mapping.h"
#include "ui/events/ozone/gamepad/webgamepad_constants.h"

namespace ui {

typedef bool (*GamepadMapperFunction)(uint16_t key,
                                      uint16_t code,
                                      GamepadEventType* mapped_type,
                                      uint16_t* mapped_code);

#define DO_MAPPING                                                   \
  DoGamepadMapping(key_mapping, arraysize(key_mapping), abs_mapping, \
                   arraysize(abs_mapping), type, code, mapped_type,  \
                   mapped_code)

bool DoGamepadMapping(const KeyMapEntry* key_mapping,
                      size_t key_map_size,
                      const AbsMapEntry* abs_mapping,
                      size_t abs_map_size,
                      uint16_t type,
                      uint16_t code,
                      GamepadEventType* mapped_type,
                      uint16_t* mapped_code) {
  if (type == EV_KEY) {
    const KeyMapEntry* entry = nullptr;
    for (size_t i = 0; i < key_map_size; i++) {
      if (key_mapping[i].evdev_code == code) {
        entry = key_mapping + i;
      }
    }
    if (!entry) {
      return false;
    }
    *mapped_type = GamepadEventType::BUTTON;
    *mapped_code = entry->mapped_code;
    return true;
  }

  if (type == EV_ABS) {
    const AbsMapEntry* entry = nullptr;
    for (size_t i = 0; i < abs_map_size; i++) {
      if (abs_mapping[i].evdev_code == code) {
        entry = abs_mapping + i;
      }
    }
    if (!entry) {
      return false;
    }
    *mapped_type = entry->mapped_type;
    *mapped_code = entry->mapped_code;
    return true;
  }
  return false;
}

// this mapper mapps gamepads compatible with xbox gamepad.
bool XInputStyleMapper(uint16_t type,
                       uint16_t code,
                       GamepadEventType* mapped_type,
                       uint16_t* mapped_code) {
  static const KeyMapType key_mapping = {
      {BTN_A, WG_BUTTON_A},            // btn_a = 304 / 0x130
      {BTN_B, WG_BUTTON_B},            // btn_b = 305 / 0x131
      {BTN_X, WG_BUTTON_X},            // btn_x = 307 / 0x133
      {BTN_Y, WG_BUTTON_Y},            // btn_y = 308 / 0x134
      {BTN_TL, WG_BUTTON_L1},          // btn_tl = 310 / 0x136
      {BTN_TR, WG_BUTTON_R1},          // btn_tr = 311 / 0x137
      {BTN_SELECT, WG_BUTTON_SELECT},  // btn_select = 314 / 0x13a
      {BTN_START, WG_BUTTON_START},    // btn_start = 315 / 0x13b
      {BTN_THUMBL, WG_BUTTON_THUMBL},  // btn_thumbl = 317 / 0x13d
      {BTN_THUMBR, WG_BUTTON_THUMBR},  // btn_thumbr = 318 / 0x13e
      {BTN_MODE, WG_BUTTON_MODE}       // btn_mode = 316 / 0x13c
  };

  static const AbsMapType abs_mapping = {
      TO_ABS(ABS_X, WG_ABS_X),       // ABS_X = 0x00
      TO_ABS(ABS_Y, WG_ABS_Y),       // ABS_Y = 0x01
      TO_ABS(ABS_RX, WG_ABS_RX),     // ABS_RX = 0x03
      TO_ABS(ABS_RY, WG_ABS_RY),     // ABS_RZ = 0x04
      TO_BTN(ABS_Z, WG_BUTTON_LT),   // ABS_Z = 0x02
      TO_BTN(ABS_RZ, WG_BUTTON_RT),  // ABS_RZ = 0x05
      TO_BTN(ABS_HAT0X, kHAT_X),     // HAT0X = 0x10
      TO_BTN(ABS_HAT0Y, kHAT_Y)      // HAT0Y = 0x11
  };
  return DO_MAPPING;
}

bool PlaystationSixAxisMapper(uint16_t type,
                              uint16_t code,
                              GamepadEventType* mapped_type,
                              uint16_t* mapped_code) {
  static const KeyMapType key_mapping = {
      {0x12e, WG_BUTTON_A},
      {0x12d, WG_BUTTON_B},
      {BTN_DEAD, WG_BUTTON_X},
      {0x12c, WG_BUTTON_Y},
      {BTN_BASE5, WG_BUTTON_L1},
      {BTN_BASE6, WG_BUTTON_R1},
      {BTN_BASE3, WG_BUTTON_LT},
      {BTN_BASE4, WG_BUTTON_RT},
      {BTN_TRIGGER, WG_BUTTON_SELECT},
      {BTN_TOP, WG_BUTTON_START},
      {BTN_THUMB, WG_BUTTON_THUMBL},
      {BTN_THUMB2, WG_BUTTON_THUMBR},
      {BTN_TOP2, WG_BUTTON_DPAD_UP},
      {BTN_BASE, WG_BUTTON_DPAD_DOWN},
      {BTN_BASE2, WG_BUTTON_DPAD_LEFT},
      {BTN_PINKIE, WG_BUTTON_DPAD_RIGHT},
      {BTN_TRIGGER_HAPPY17, WG_BUTTON_MODE},
  };

  static const AbsMapType abs_mapping = {
      TO_ABS(ABS_X, WG_ABS_X),
      TO_ABS(ABS_Y, WG_ABS_Y),
      TO_ABS(ABS_Z, WG_ABS_RX),
      TO_ABS(ABS_RZ, WG_ABS_RY),
      TO_BTN(ABS_MT_TOUCH_MAJOR, WG_BUTTON_LT),
      TO_BTN(ABS_MT_TOUCH_MINOR, WG_BUTTON_RT)};
  return DO_MAPPING;
}

bool DualShock4Mapper(uint16_t type,
                      uint16_t code,
                      GamepadEventType* mapped_type,
                      uint16_t* mapped_code) {
  static const KeyMapType key_mapping = {
      {BTN_A, WG_BUTTON_A},           {BTN_B, WG_BUTTON_B},
      {BTN_C, WG_BUTTON_X},           {BTN_X, WG_BUTTON_Y},
      {BTN_Y, WG_BUTTON_L1},          {BTN_Z, WG_BUTTON_R1},
      {BTN_TL, WG_BUTTON_LT},         {BTN_TR, WG_BUTTON_RT},
      {BTN_TL2, WG_BUTTON_SELECT},    {BTN_TR2, WG_BUTTON_START},
      {BTN_SELECT, WG_BUTTON_THUMBL}, {BTN_START, WG_BUTTON_THUMBR}};

  static const AbsMapType abs_mapping = {
      TO_ABS(ABS_X, WG_ABS_X),      TO_ABS(ABS_Y, WG_ABS_Y),
      TO_ABS(ABS_RX, WG_BUTTON_LT), TO_ABS(ABS_RY, WG_BUTTON_RT),
      TO_ABS(ABS_Z, WG_ABS_RX),     TO_ABS(ABS_RZ, WG_ABS_RY),
      TO_BTN(ABS_HAT0X, kHAT_X),    TO_BTN(ABS_HAT0Y, kHAT_Y)};
  return DO_MAPPING;
}

bool NintendoSwitchPro(uint16_t type,
                       uint16_t code,
                       GamepadEventType* mapped_type,
                       uint16_t* mapped_code) {
  static const KeyMapType key_mapping = {
      {BTN_A, WG_BUTTON_A},           {BTN_B, WG_BUTTON_B},
      {BTN_C, WG_BUTTON_X},           {BTN_X, WG_BUTTON_Y},
      {BTN_Y, WG_BUTTON_L1},          {BTN_Z, WG_BUTTON_R1},
      {BTN_TL, WG_BUTTON_LT},         {BTN_TR, WG_BUTTON_RT},
      {BTN_TL2, WG_BUTTON_SELECT},    {BTN_TR2, WG_BUTTON_START},
      {BTN_SELECT, WG_BUTTON_THUMBL}, {BTN_START, WG_BUTTON_THUMBR},
      {BTN_MODE, WG_BUTTON_MODE}};

  static const AbsMapType abs_mapping = {
      TO_ABS(ABS_X, WG_ABS_X),   TO_ABS(ABS_Y, WG_ABS_Y),
      TO_ABS(ABS_RX, WG_ABS_RX), TO_ABS(ABS_RY, WG_ABS_RY),
      TO_BTN(ABS_HAT0X, kHAT_X), TO_BTN(ABS_HAT0Y, kHAT_Y)};
  return DO_MAPPING;
}

bool NintendoSwitchLeft(uint16_t type,
                        uint16_t code,
                        GamepadEventType* mapped_type,
                        uint16_t* mapped_code) {
  static const KeyMapType key_mapping = {
      {BTN_A, WG_BUTTON_A},        {BTN_B, WG_BUTTON_B},
      {BTN_C, WG_BUTTON_X},        {BTN_X, WG_BUTTON_Y},
      {BTN_Y, WG_BUTTON_L1},       {BTN_Z, WG_BUTTON_R1},
      {BTN_TL2, WG_BUTTON_SELECT}, {BTN_THUMBL, WG_BUTTON_START}};

  static const AbsMapType abs_mapping = {TO_BTN(ABS_HAT0X, kHAT_X),
                                         TO_BTN(ABS_HAT0Y, kHAT_Y)};
  return DO_MAPPING;
}

bool NintendoSwitchRight(uint16_t type,
                         uint16_t code,
                         GamepadEventType* mapped_type,
                         uint16_t* mapped_code) {
  static const KeyMapType key_mapping = {
      {BTN_A, WG_BUTTON_A},         {BTN_B, WG_BUTTON_B},
      {BTN_C, WG_BUTTON_X},         {BTN_X, WG_BUTTON_Y},
      {BTN_Y, WG_BUTTON_L1},        {BTN_Z, WG_BUTTON_R1},
      {BTN_MODE, WG_BUTTON_SELECT}, {BTN_TR2, WG_BUTTON_START}};

  static const AbsMapType abs_mapping = {TO_BTN(ABS_HAT0X, kHAT_X),
                                         TO_BTN(ABS_HAT0Y, kHAT_Y)};
  return DO_MAPPING;
}

bool IBuffalocClassicMapper(uint16_t type,
                            uint16_t code,
                            GamepadEventType* mapped_type,
                            uint16_t* mapped_code) {
  static const KeyMapType key_mapping = {
      {BTN_TRIGGER, WG_BUTTON_A}, {BTN_THUMB, WG_BUTTON_B},
      {BTN_THUMB2, WG_BUTTON_X},  {BTN_TOP, WG_BUTTON_Y},
      {BTN_BASE, WG_BUTTON_L1},   {BTN_BASE2, WG_BUTTON_R1},
      {BTN_TOP2, WG_BUTTON_LT},   {BTN_PINKIE, WG_BUTTON_RT}};

  static const AbsMapType abs_mapping = {
      TO_BTN(ABS_X, kHAT_X), TO_BTN(ABS_Y, kHAT_Y),
  };
  return DO_MAPPING;
}

bool ClassicNESMapper(uint16_t type,
                      uint16_t code,
                      GamepadEventType* mapped_type,
                      uint16_t* mapped_code) {
  static const KeyMapType key_mapping = {
      {BTN_THUMB, WG_BUTTON_A},      {BTN_THUMB2, WG_BUTTON_B},
      {BTN_TRIGGER, WG_BUTTON_X},    {BTN_TOP, WG_BUTTON_Y},
      {BTN_TOP2, WG_BUTTON_L1},      {BTN_PINKIE, WG_BUTTON_R1},
      {BTN_BASE3, WG_BUTTON_SELECT}, {BTN_BASE4, WG_BUTTON_START}};

  static const AbsMapType abs_mapping = {
      TO_BTN(ABS_X, kHAT_X), TO_BTN(ABS_Y, kHAT_Y),
  };
  return DO_MAPPING;
}

bool SNesRetroMapper(uint16_t type,
                     uint16_t code,
                     GamepadEventType* mapped_type,
                     uint16_t* mapped_code) {
  static const KeyMapType key_mapping = {
      {BTN_C, WG_BUTTON_A},        {BTN_B, WG_BUTTON_B},
      {BTN_X, WG_BUTTON_X},        {BTN_A, WG_BUTTON_Y},
      {BTN_Y, WG_BUTTON_L1},       {BTN_Z, WG_BUTTON_R1},
      {BTN_TL2, WG_BUTTON_SELECT}, {BTN_TR2, WG_BUTTON_START}};

  static const AbsMapType abs_mapping = {
      TO_ABS(ABS_X, kHAT_X), TO_ABS(ABS_Y, kHAT_Y),
  };
  return DO_MAPPING;
}

bool ADT1Mapper(uint16_t type,
                uint16_t code,
                GamepadEventType* mapped_type,
                uint16_t* mapped_code) {
  static const KeyMapType key_mapping = {
      {BTN_A, WG_BUTTON_A},           {BTN_B, WG_BUTTON_B},
      {BTN_X, WG_BUTTON_X},           {BTN_Y, WG_BUTTON_Y},
      {BTN_TL, WG_BUTTON_L1},         {BTN_TR, WG_BUTTON_R1},
      {BTN_THUMBL, WG_BUTTON_THUMBL}, {BTN_THUMBR, WG_BUTTON_THUMBR},
      {BTN_MODE, WG_BUTTON_MODE},     {KEY_BACK, WG_BUTTON_SELECT},
      {KEY_HOMEPAGE, WG_BUTTON_START}};

  static const AbsMapType abs_mapping = {
      TO_ABS(ABS_X, WG_ABS_X),         TO_ABS(ABS_Y, WG_ABS_Y),
      TO_ABS(ABS_Z, WG_ABS_RX),        TO_ABS(ABS_RZ, WG_ABS_RY),
      TO_BTN(ABS_BRAKE, WG_BUTTON_LT), TO_BTN(ABS_GAS, WG_BUTTON_RT),
      TO_BTN(ABS_HAT0X, kHAT_X),       TO_BTN(ABS_HAT0Y, kHAT_Y)};
  return DO_MAPPING;
}

bool Vendor_1d79Product_0009Mapper(uint16_t type,
                                   uint16_t code,
                                   GamepadEventType* mapped_type,
                                   uint16_t* mapped_code) {
  static const KeyMapType key_mapping = {{BTN_A, WG_BUTTON_A},
                                         {BTN_B, WG_BUTTON_B},
                                         {BTN_X, WG_BUTTON_X},
                                         {BTN_Y, WG_BUTTON_Y},
                                         {BTN_TL, WG_BUTTON_L1},
                                         {BTN_TR, WG_BUTTON_R1},
                                         {BTN_START, WG_BUTTON_START},
                                         {BTN_THUMBL, WG_BUTTON_THUMBL},
                                         {BTN_THUMBR, WG_BUTTON_THUMBR},
                                         {KEY_UP, WG_BUTTON_DPAD_UP},
                                         {KEY_DOWN, WG_BUTTON_DPAD_DOWN},
                                         {KEY_LEFT, WG_BUTTON_DPAD_LEFT},
                                         {KEY_RIGHT, WG_BUTTON_DPAD_RIGHT},
                                         {KEY_BACK, WG_BUTTON_SELECT},
                                         {KEY_HOMEPAGE, WG_BUTTON_MODE}};

  static const AbsMapType abs_mapping = {
      TO_ABS(ABS_X, WG_ABS_X),         TO_ABS(ABS_Y, WG_ABS_Y),
      TO_ABS(ABS_Z, WG_ABS_RX),        TO_ABS(ABS_RZ, WG_ABS_RY),
      TO_BTN(ABS_BRAKE, WG_BUTTON_LT), TO_BTN(ABS_GAS, WG_BUTTON_RT),
      TO_BTN(ABS_HAT0X, kHAT_X),       TO_BTN(ABS_HAT0Y, kHAT_Y)};
  return DO_MAPPING;
}

bool Vendor_046dProduct_b501Mapper(uint16_t type,
                                   uint16_t code,
                                   GamepadEventType* mapped_type,
                                   uint16_t* mapped_code) {
  static const KeyMapType key_mapping = {{BTN_A, WG_BUTTON_A},
                                         {BTN_B, WG_BUTTON_B},
                                         {BTN_X, WG_BUTTON_X},
                                         {BTN_Y, WG_BUTTON_Y},
                                         {BTN_TL, WG_BUTTON_L1},
                                         {BTN_TR, WG_BUTTON_R1},
                                         {BTN_TL2, WG_BUTTON_LT},
                                         {BTN_TR2, WG_BUTTON_RT},
                                         {BTN_SELECT, WG_BUTTON_SELECT},
                                         {BTN_START, WG_BUTTON_START},
                                         {BTN_THUMBL, WG_BUTTON_THUMBL},
                                         {BTN_THUMBR, WG_BUTTON_THUMBR},
                                         {KEY_UP, WG_BUTTON_DPAD_UP},
                                         {KEY_DOWN, WG_BUTTON_DPAD_DOWN},
                                         {KEY_LEFT, WG_BUTTON_DPAD_LEFT},
                                         {KEY_RIGHT, WG_BUTTON_DPAD_RIGHT},
                                         {BTN_MODE, WG_BUTTON_MODE}};

  static const AbsMapType abs_mapping = {
      TO_ABS(ABS_X, WG_ABS_X),         TO_ABS(ABS_Y, WG_ABS_Y),
      TO_ABS(ABS_Z, WG_ABS_RX),        TO_ABS(ABS_RZ, WG_ABS_RY),
      TO_BTN(ABS_BRAKE, WG_BUTTON_LT), TO_BTN(ABS_GAS, WG_BUTTON_RT),
      TO_BTN(ABS_HAT0X, kHAT_X),       TO_BTN(ABS_HAT0Y, kHAT_Y)};
  return DO_MAPPING;
}

bool Vendor_046dProduct_c216Mapper(uint16_t type,
                                   uint16_t code,
                                   GamepadEventType* mapped_type,
                                   uint16_t* mapped_code) {
  static const KeyMapType key_mapping = {
      {BTN_TRIGGER, WG_BUTTON_A},    {BTN_TOP, WG_BUTTON_B},
      {BTN_THUMB, WG_BUTTON_X},      {BTN_THUMB2, WG_BUTTON_Y},
      {BTN_TOP2, WG_BUTTON_L1},      {BTN_PINKIE, WG_BUTTON_R1},
      {BTN_BASE, WG_BUTTON_LT},      {BTN_BASE2, WG_BUTTON_RT},
      {BTN_BASE3, WG_BUTTON_SELECT}, {BTN_BASE4, WG_BUTTON_START},
      {BTN_BASE5, WG_BUTTON_THUMBL}, {BTN_BASE6, WG_BUTTON_THUMBR}};

  static const AbsMapType abs_mapping = {
      TO_ABS(ABS_X, WG_ABS_X),   TO_ABS(ABS_Y, WG_ABS_Y),
      TO_ABS(ABS_Z, WG_ABS_RX),  TO_ABS(ABS_RZ, WG_ABS_RY),
      TO_BTN(ABS_HAT0X, kHAT_X), TO_BTN(ABS_HAT0Y, kHAT_Y)};
  return DO_MAPPING;
}

bool Vendor_046dProduct_c219Mapper(uint16_t type,
                                   uint16_t code,
                                   GamepadEventType* mapped_type,
                                   uint16_t* mapped_code) {
  static const KeyMapType key_mapping = {
      {BTN_B, WG_BUTTON_A},          {BTN_C, WG_BUTTON_B},
      {BTN_A, WG_BUTTON_X},          {BTN_X, WG_BUTTON_Y},
      {BTN_Y, WG_BUTTON_L1},         {BTN_Z, WG_BUTTON_R1},
      {BTN_TL, WG_BUTTON_LT},        {BTN_TR, WG_BUTTON_RT},
      {BTN_TR2, WG_BUTTON_START},    {BTN_SELECT, WG_BUTTON_THUMBL},
      {BTN_START, WG_BUTTON_THUMBR}, {BTN_TL2, WG_BUTTON_SELECT}};

  static const AbsMapType abs_mapping = {
      TO_ABS(ABS_X, WG_ABS_X),   TO_ABS(ABS_Y, WG_ABS_Y),
      TO_ABS(ABS_Z, WG_ABS_RX),  TO_ABS(ABS_RZ, WG_ABS_RY),
      TO_BTN(ABS_HAT0X, kHAT_X), TO_BTN(ABS_HAT0Y, kHAT_Y)};
  return DO_MAPPING;
}

bool Vendor_1038Product_1412Mapper(uint16_t type,
                                   uint16_t code,
                                   GamepadEventType* mapped_type,
                                   uint16_t* mapped_code) {
  static const KeyMapType key_mapping = {
      {BTN_A, WG_BUTTON_A},         {BTN_B, WG_BUTTON_B},
      {BTN_X, WG_BUTTON_X},         {BTN_Y, WG_BUTTON_Y},
      {BTN_TL, WG_BUTTON_L1},       {BTN_TR, WG_BUTTON_R1},
      {BTN_MODE, WG_BUTTON_SELECT}, {BTN_START, WG_BUTTON_START},
  };

  static const AbsMapType abs_mapping = {
      TO_ABS(ABS_X, WG_ABS_X),   TO_ABS(ABS_Y, WG_ABS_Y),
      TO_ABS(ABS_Z, WG_ABS_RX),  TO_ABS(ABS_RZ, WG_ABS_RY),
      TO_BTN(ABS_HAT0X, kHAT_X), TO_BTN(ABS_HAT0Y, kHAT_Y)};
  return DO_MAPPING;
}

bool Vendor_1689Product_fd00Mapper(uint16_t type,
                                   uint16_t code,
                                   GamepadEventType* mapped_type,
                                   uint16_t* mapped_code) {
  static const KeyMapType key_mapping = {
      {BTN_A, WG_BUTTON_A},
      {BTN_B, WG_BUTTON_B},
      {BTN_X, WG_BUTTON_X},
      {BTN_Y, WG_BUTTON_Y},
      {BTN_TL, WG_BUTTON_L1},
      {BTN_TR, WG_BUTTON_R1},
      {BTN_START, WG_BUTTON_START},
      {BTN_THUMBL, WG_BUTTON_THUMBL},
      {BTN_THUMBR, WG_BUTTON_THUMBR},
      {BTN_TRIGGER_HAPPY3, WG_BUTTON_DPAD_UP},
      {BTN_TRIGGER_HAPPY4, WG_BUTTON_DPAD_DOWN},
      {BTN_TRIGGER_HAPPY1, WG_BUTTON_DPAD_LEFT},
      {BTN_TRIGGER_HAPPY2, WG_BUTTON_DPAD_RIGHT},
      {BTN_SELECT, WG_BUTTON_SELECT},
      {BTN_MODE, WG_BUTTON_MODE}};

  static const AbsMapType abs_mapping = {
      TO_ABS(ABS_X, WG_ABS_X),     TO_ABS(ABS_Y, WG_ABS_Y),
      TO_ABS(ABS_RX, WG_ABS_RX),   TO_ABS(ABS_RY, WG_ABS_RY),
      TO_BTN(ABS_Z, WG_BUTTON_LT), TO_BTN(ABS_RZ, WG_BUTTON_RT)};
  return DO_MAPPING;
}

bool JoydevLikeMapper(uint16_t type,
                      uint16_t code,
                      GamepadEventType* mapped_type,
                      uint16_t* mapped_code) {
  static const KeyMapType key_mapping = {
      {BTN_A, WG_BUTTON_A},           {BTN_B, WG_BUTTON_B},
      {BTN_C, WG_BUTTON_X},           {BTN_X, WG_BUTTON_Y},
      {BTN_Y, WG_BUTTON_L1},          {BTN_Z, WG_BUTTON_R1},
      {BTN_TL, WG_BUTTON_LT},         {BTN_TR, WG_BUTTON_RT},
      {BTN_TL2, WG_BUTTON_SELECT},    {BTN_TR2, WG_BUTTON_START},
      {BTN_SELECT, WG_BUTTON_THUMBL}, {BTN_START, WG_BUTTON_THUMBR}};

  static const AbsMapType abs_mapping = {
      TO_ABS(ABS_X, WG_ABS_X),   TO_ABS(ABS_Y, WG_ABS_Y),
      TO_ABS(ABS_Z, WG_ABS_RX),  TO_ABS(ABS_RZ, WG_ABS_RY),
      TO_BTN(ABS_HAT0X, kHAT_X), TO_BTN(ABS_HAT0Y, kHAT_Y)};
  return DO_MAPPING;
}

bool NvidiaShieldMapper(uint16_t type,
                        uint16_t code,
                        GamepadEventType* mapped_type,
                        uint16_t* mapped_code) {
  static const KeyMapType key_mapping = {
      {BTN_A, WG_BUTTON_A},           {BTN_B, WG_BUTTON_B},
      {BTN_X, WG_BUTTON_X},           {BTN_Y, WG_BUTTON_Y},
      {BTN_TL, WG_BUTTON_L1},         {BTN_TR, WG_BUTTON_R1},
      {KEY_BACK, WG_BUTTON_SELECT},   {BTN_START, WG_BUTTON_START},
      {KEY_HOMEPAGE, WG_BUTTON_MODE}, {BTN_THUMBL, WG_BUTTON_THUMBL},
      {BTN_THUMBR, WG_BUTTON_THUMBR}};

  static const AbsMapType abs_mapping = {
      TO_ABS(ABS_X, WG_ABS_X),         TO_ABS(ABS_Y, WG_ABS_Y),
      TO_ABS(ABS_Z, WG_ABS_RX),        TO_ABS(ABS_RZ, WG_ABS_RY),
      TO_BTN(ABS_BRAKE, WG_BUTTON_LT), TO_BTN(ABS_GAS, WG_BUTTON_RT),
      TO_BTN(ABS_HAT0X, kHAT_X),       TO_BTN(ABS_HAT0Y, kHAT_Y)};
  return DO_MAPPING;
}

static const struct MappingData {
  uint16_t vendor_id;
  uint16_t product_id;
  GamepadMapperFunction mapper;
} AvailableMappings[] = {
    // Xbox style gamepad.
    {0x045e, 0x028e, XInputStyleMapper},  // Xbox 360 wired.
    {0x045e, 0x028f, XInputStyleMapper},  // Xbox 360 wireless.
    {0x045e, 0x02a1, XInputStyleMapper},  // Xbox 360 wireless.
    {0x045e, 0x02d1, XInputStyleMapper},  // Xbox one wired.
    {0x045e, 0x02dd, XInputStyleMapper},  // Xbox one wired (2015 fw).
    {0x045e, 0x02e3, XInputStyleMapper},  // Xbox elite wired.
    {0x045e, 0x02ea, XInputStyleMapper},  // Xbox one s (usb).
    {0x045e, 0x0719, XInputStyleMapper},  // Xbox 360 wireless.
    {0x046d, 0xc21d, XInputStyleMapper},  // Logitech f310.
    {0x046d, 0xc21e, XInputStyleMapper},  // Logitech f510.
    {0x046d, 0xc21f, XInputStyleMapper},  // Logitech f710.
    {0x2378, 0x1008, XInputStyleMapper},  // Onlive controller (bluetooth).
    {0x2378, 0x100a, XInputStyleMapper},  // Onlive controller (wired).
    {0x1bad, 0xf016, XInputStyleMapper},  // Mad catz gamepad.
    {0x1bad, 0xf023, XInputStyleMapper},  // Mad catz mlg gamepad for Xbox360.
    {0x1bad, 0xf027, XInputStyleMapper},  // Mad catz fps pro.
    {0x1bad, 0xf036, XInputStyleMapper},  // Mad catz generic Xbox controller.
    {0x1689, 0xfd01, XInputStyleMapper},  // Razer Xbox 360 gamepad.
    {0x1689, 0xfe00, XInputStyleMapper},  // Razer sabertooth elite.
    // Sony gamepads.
    {0x054c, 0x0268, PlaystationSixAxisMapper},  // Playstation 3.
    {0x054c, 0x05c4, DualShock4Mapper},          // Dualshock 4.
    // Nintendo switch.
    {0x057e, 0x2009, NintendoSwitchPro},    // Nintendo switch pro.
    {0x057e, 0x2006, NintendoSwitchLeft},   // Nintendo switch left.
    {0x057e, 0x2007, NintendoSwitchRight},  // Nintendo switch right.
    // NES style gamepad.
    {0x0583, 0x2060, IBuffalocClassicMapper},         // iBuffalo Classic.
    {0x0079, 0x0011, ClassicNESMapper},               // Classic NES controller.
    {0x12bd, 0xd015, SNesRetroMapper},                // Hitgaming SNES retro.
    {0x3810, 0x9, Vendor_046dProduct_b501Mapper},     // FC30 Pro, bluetooth.
    {0x1002, 0x9000, Vendor_046dProduct_b501Mapper},  // FC30 Pro, wired.
    // Android gamepad.
    {0x0b05, 0x4500, ADT1Mapper},  // Nexus player controller (asus gamepad).
    {0x1532, 0x0900, ADT1Mapper},  // Razer serval.
    {0x18d1, 0x2c40, ADT1Mapper},  // ADT-1 controller (odie).
    // Other gamepads.
    {0x1d79, 0x0009,
     Vendor_1d79Product_0009Mapper},  // Nyko playpad / Playpad pro.
    {0x046d, 0xb501, Vendor_046dProduct_b501Mapper},  // Logitech redhawk.
    // Logitech dual action controller.
    {0x046d, 0xc216, Vendor_046dProduct_c216Mapper},
    // Logitech cordless rumblepad2.
    {0x046d, 0xc219, Vendor_046dProduct_c219Mapper},
    {0x1038, 0x1412, Vendor_1038Product_1412Mapper},  // Steelseries free.
    // Razer onza tournment edition.
    {0x1689, 0xfd00, Vendor_1689Product_fd00Mapper},
    {0x0955, 0x7214, NvidiaShieldMapper},  // Nvidia shield.
    {0x11c5, 0x5506, JoydevLikeMapper}     // HJC Game ZD-V
};

class StaticGamepadMapper : public GamepadMapper {
 public:
  StaticGamepadMapper(GamepadMapperFunction fp) : mapper_fp_(fp) {}

  bool Map(uint16_t type,
           uint16_t code,
           GamepadEventType* mapped_type,
           uint16_t* mapped_code) const override {
    return mapper_fp_(type, code, mapped_type, mapped_code);
  };

 private:
  GamepadMapperFunction mapper_fp_;
};

GamepadMapper* GetStaticGamepadMapper(uint16_t vendor_id, uint16_t product_id) {
  for (size_t i = 0; i < arraysize(AvailableMappings); i++) {
    if (AvailableMappings[i].vendor_id == vendor_id &&
        AvailableMappings[i].product_id == product_id) {
      return new StaticGamepadMapper(AvailableMappings[i].mapper);
    }
  }
  return nullptr;
}

}  // namespace ui
