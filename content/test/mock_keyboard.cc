// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/test/mock_keyboard.h"

#include "base/logging.h"
#include "build/build_config.h"

namespace content {

MockKeyboard::MockKeyboard() {}

MockKeyboard::~MockKeyboard() {
}

int MockKeyboard::GetCharacters(Layout layout,
                                int key_code,
                                Modifiers modifiers,
                                std::wstring* output) {
#if defined(OS_WIN)
  CHECK(output);
  // Change the keyboard layout only when we have to because it takes a lot of
  // time to load a keyboard-layout driver.
  // When we change the layout, we reset the modifier status to force updating
  // the keyboard status.
  if (layout != keyboard_layout_) {
    if (!driver_.SetLayout(layout))
      return -1;
    keyboard_layout_ = layout;
    keyboard_modifiers_ = INVALID;
  }

  // Update the keyboard states.
  if (modifiers != keyboard_modifiers_) {
    if (!driver_.SetModifiers(modifiers))
      return -1;
    keyboard_modifiers_ = modifiers;
  }

  // Retrieve Unicode characters associate with the key code.
  return driver_.GetCharacters(key_code, output);
#else
  NOTIMPLEMENTED();
  return -1;
#endif
}

}  // namespace content
