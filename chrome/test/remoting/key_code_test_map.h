// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Source of data in this file:
//  1. ui/events/keycodes/dom/keycode_converter_data.inc
//  2. ui/events/keycodes/keyboard_codes.h
//  3. third_party/WebKit/Source/core/platform/chromium/KeyboardCodes.h
#ifndef CHROME_TEST_REMOTING_KEY_CODE_TEST_MAP_H_
#define CHROME_TEST_REMOTING_KEY_CODE_TEST_MAP_H_

#include "ui/events/keycodes/keyboard_codes.h"

namespace remoting {

typedef struct {
  // The UIEvents (aka: DOM4Events) |code| value as defined in:
  // https://dvcs.w3.org/hg/d4e/raw-file/tip/source_respec.htm
  const char* code;

  // The (Windows) virtual keyboard code.
  ui::KeyboardCode vkey_code;
} KeyCodeTestMap;

const KeyCodeTestMap test_alpha_map[] = {
  {"KeyA", ui::VKEY_A},
  {"KeyB", ui::VKEY_B},
  {"KeyC", ui::VKEY_C},
  {"KeyD", ui::VKEY_D},
  {"KeyE", ui::VKEY_E},
  {"KeyF", ui::VKEY_F},
  {"KeyG", ui::VKEY_G},
  {"KeyH", ui::VKEY_H},
  {"KeyI", ui::VKEY_I},
  {"KeyJ", ui::VKEY_J},
  {"KeyK", ui::VKEY_K},
  {"KeyL", ui::VKEY_L},
  {"KeyM", ui::VKEY_M},
  {"KeyN", ui::VKEY_N},
  {"KeyO", ui::VKEY_O},
  {"KeyP", ui::VKEY_P},
  {"KeyQ", ui::VKEY_Q},
  {"KeyR", ui::VKEY_R},
  {"KeyS", ui::VKEY_S},
  {"KeyT", ui::VKEY_T},
  {"KeyU", ui::VKEY_U},
  {"KeyV", ui::VKEY_V},
  {"KeyW", ui::VKEY_W},
  {"KeyX", ui::VKEY_X},
  {"KeyY", ui::VKEY_Y},
  {"KeyZ", ui::VKEY_Z},
};

const KeyCodeTestMap test_digit_map[] = {
  {"Digit1", ui::VKEY_1},
  {"Digit2", ui::VKEY_2},
  {"Digit3", ui::VKEY_3},
  {"Digit4", ui::VKEY_4},
  {"Digit5", ui::VKEY_5},
  {"Digit6", ui::VKEY_6},
  {"Digit7", ui::VKEY_7},
  {"Digit8", ui::VKEY_8},
  {"Digit9", ui::VKEY_9},
  {"Digit0", ui::VKEY_0},
};

const KeyCodeTestMap test_numpad_map[] = {
  {"Numpad0", ui::VKEY_NUMPAD0},
  {"Numpad1", ui::VKEY_NUMPAD1},
  {"Numpad2", ui::VKEY_NUMPAD2},
  {"Numpad3", ui::VKEY_NUMPAD3},
  {"Numpad4", ui::VKEY_NUMPAD4},
  {"Numpad5", ui::VKEY_NUMPAD5},
  {"Numpad6", ui::VKEY_NUMPAD6},
  {"Numpad7", ui::VKEY_NUMPAD7},
  {"Numpad8", ui::VKEY_NUMPAD8},
  {"Numpad9", ui::VKEY_NUMPAD9},
  {"NumpadMultiply", ui::VKEY_MULTIPLY},
  {"NumpadAdd", ui::VKEY_ADD},
  {"NumpadSubtract", ui::VKEY_SUBTRACT},
  {"NumpadDecimal", ui::VKEY_DECIMAL},
  {"NumpadDivide", ui::VKEY_DIVIDE},
};

const KeyCodeTestMap test_special_map[] = {
  {"Enter", ui::VKEY_RETURN},
  {"ShiftRight", ui::VKEY_SHIFT},
  {"Space", ui::VKEY_SPACE},
  {"Backquote", ui::VKEY_OEM_3},
  {"Comma", ui::VKEY_OEM_COMMA},
  {"Period", ui::VKEY_OEM_PERIOD},
  {"Home", ui::VKEY_HOME},
};

}  // namespace remoting
#endif // CHROME_TEST_REMOTING_KEY_CODE_TEST_MAP_H_
