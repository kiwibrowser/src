// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Source of data in this file:
//  1. ui/events/keycodes/dom/keycode_converter_data.inc
//  2. ui/events/keycodes/keyboard_codes.h
//  3. third_party/WebKit/Source/core/platform/chromium/KeyboardCodes.h
#ifndef CHROME_TEST_REMOTING_KEY_CODE_MAP_H_
#define CHROME_TEST_REMOTING_KEY_CODE_MAP_H_

#include "ui/events/keycodes/keyboard_codes.h"

namespace remoting {

typedef struct {
  // The character typed as a result of the key press without shift.
  char lower_char;

  // The character typed as a result of the key press with shift.
  char upper_char;

  // The UIEvents (aka: DOM4Events) |code| value as defined in:
  // https://dvcs.w3.org/hg/d4e/raw-file/tip/source_respec.htm
  const char* code;

  // The (Windows) virtual keyboard code.
  ui::KeyboardCode vkey_code;
} KeyCodeMap;

// The mapping between the native scan codes and the characters are based
// on US keyboard layout.
const KeyCodeMap key_code_map[] = {

// lower UPPER Code  KeyboardCode
  {'a', 'A', "KeyA", ui::VKEY_A},  // aA
  {'b', 'B', "KeyB", ui::VKEY_B},  // bB
  {'c', 'C', "KeyC", ui::VKEY_C},  // cC
  {'d', 'D', "KeyD", ui::VKEY_D},  // dD
  {'e', 'E', "KeyE", ui::VKEY_E},  // eE
  {'f', 'F', "KeyF", ui::VKEY_F},  // fF
  {'g', 'G', "KeyG", ui::VKEY_G},  // gG
  {'h', 'H', "KeyH", ui::VKEY_H},  // hH
  {'i', 'I', "KeyI", ui::VKEY_I},  // iI
  {'j', 'J', "KeyJ", ui::VKEY_J},  // jJ
  {'k', 'K', "KeyK", ui::VKEY_K},  // kK
  {'l', 'L', "KeyL", ui::VKEY_L},  // lL
  {'m', 'M', "KeyM", ui::VKEY_M},  // mM
  {'n', 'N', "KeyN", ui::VKEY_N},  // nN
  {'o', 'O', "KeyO", ui::VKEY_O},  // oO
  {'p', 'P', "KeyP", ui::VKEY_P},  // pP
  {'q', 'Q', "KeyQ", ui::VKEY_Q},  // qQ
  {'r', 'R', "KeyR", ui::VKEY_R},  // rR
  {'s', 'S', "KeyS", ui::VKEY_S},  // sS
  {'t', 'T', "KeyT", ui::VKEY_T},  // tT
  {'u', 'U', "KeyU", ui::VKEY_U},  // uU
  {'v', 'V', "KeyV", ui::VKEY_V},  // vV
  {'w', 'W', "KeyW", ui::VKEY_W},  // wW
  {'x', 'X', "KeyX", ui::VKEY_X},  // xX
  {'y', 'Y', "KeyY", ui::VKEY_Y},  // yY
  {'z', 'Z', "KeyZ", ui::VKEY_Z},  // zZ
  {'1', '1', "Digit1", ui::VKEY_1},  // 1!
  {'2', '2', "Digit2", ui::VKEY_2},  // 2@
  {'3', '3', "Digit3", ui::VKEY_3},  // 3#
  {'4', '4', "Digit4", ui::VKEY_4},  // 4$
  {'5', '5', "Digit5", ui::VKEY_5},  // 5%
  {'6', '6', "Digit6", ui::VKEY_6},  // 6^
  {'7', '7', "Digit7", ui::VKEY_7},  // 7&
  {'8', '8', "Digit8", ui::VKEY_8},  // 8*
  {'9', '9', "Digit9", ui::VKEY_9},  // 9(
  {'0', '0', "Digit0", ui::VKEY_0},  // 0)

  {'\n', '\n', "Enter", ui::VKEY_RETURN},  // Return
  { 0 ,  0 , "Escape", ui::VKEY_UNKNOWN},  // Escape
  {'\b', '\b', "Backspace", ui::VKEY_BACK},  // Backspace
  {'\t', '\t', "Tab", ui::VKEY_TAB},  // Tab
  {' ', ' ', "Space", ui::VKEY_SPACE},  // Spacebar
  {'-', '_', "Minus", ui::VKEY_OEM_MINUS},  // -_
  {'=', '+', "Equal", ui::VKEY_OEM_PLUS},  // =+
  {'[', '{', "BracketLeft", ui::VKEY_OEM_4},  // [{
  {']', '}', "BracketRight", ui::VKEY_OEM_6},  // ]}
  {'\\', '|', "Backslash", ui::VKEY_OEM_5},  // \| (US keyboard only)
  {';', ':', "Semicolon", ui::VKEY_OEM_1},  // ;:
  {'\'', '\"', "Quote", ui::VKEY_OEM_7},  // '"
  {'`', '~', "Backquote", ui::VKEY_OEM_3},  // `~
  {',', '<', "Comma", ui::VKEY_OEM_COMMA},  // ,<
  {'.', '>', "Period", ui::VKEY_OEM_PERIOD},  // .>
  {'/', '?', "Slash", ui::VKEY_OEM_2},  // /?
};

}  // namespace remoting

#endif  // CHROME_TEST_REMOTING_KEY_CODE_MAP_H_
