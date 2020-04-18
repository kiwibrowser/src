// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UI_BASE_IME_CHROMEOS_IME_KEYMAP_H_
#define UI_BASE_IME_CHROMEOS_IME_KEYMAP_H_

#include <string>
#include "ui/base/ime/ui_base_ime_export.h"
#include "ui/events/keycodes/keyboard_codes.h"

namespace ui {

// Translates the DOM4 key code string to ui::KeyboardCode.
UI_BASE_IME_EXPORT KeyboardCode
DomKeycodeToKeyboardCode(const std::string& code);

// Translates the ui::KeyboardCode to DOM4 key code string.
UI_BASE_IME_EXPORT std::string KeyboardCodeToDomKeycode(KeyboardCode code);

}  // namespace ui

#endif  // UI_BASE_IME_CHROMEOS_IME_KEYMAP_H_
