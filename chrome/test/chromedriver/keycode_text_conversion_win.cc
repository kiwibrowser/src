// Copyright (c) 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/test/chromedriver/keycode_text_conversion.h"

#include <VersionHelpers.h>
#include <stdlib.h>
#include <string.h>
#include <windows.h>

#include <memory>

#include "base/strings/utf_string_conversions.h"
#include "chrome/test/chromedriver/chrome/ui_events.h"

bool ConvertKeyCodeToText(
    ui::KeyboardCode key_code, int modifiers, std::string* text,
    std::string* error_msg) {
  UINT scan_code = ::MapVirtualKeyW(key_code, MAPVK_VK_TO_VSC);
  BYTE keyboard_state[256];
  memset(keyboard_state, 0, 256);
  *error_msg = std::string();
  if (modifiers & kShiftKeyModifierMask)
    keyboard_state[VK_SHIFT] |= 0x80;
  if (modifiers & kControlKeyModifierMask)
    keyboard_state[VK_CONTROL] |= 0x80;
  if (modifiers & kAltKeyModifierMask)
    keyboard_state[VK_MENU] |= 0x80;
  wchar_t chars[5];
  int code = ::ToUnicode(key_code, scan_code, keyboard_state, chars, 4, 0);
  // |ToUnicode| converts some non-text key codes like F1 to various
  // control chars. Filter those out.
  if (code <= 0 || (code == 1 && iswcntrl(chars[0])))
    *text = std::string();
  else
    base::WideToUTF8(chars, code, text);
  return true;
}

bool ConvertCharToKeyCode(
    base::char16 key, ui::KeyboardCode* key_code, int *necessary_modifiers,
    std::string* error_msg) {
  short vkey_and_modifiers = ::VkKeyScanW(key);
  bool translated = vkey_and_modifiers != -1 &&
                    LOBYTE(vkey_and_modifiers) != 0xFF &&
                    HIBYTE(vkey_and_modifiers) != 0xFF;
  *error_msg = std::string();
  if (translated) {
    *key_code = static_cast<ui::KeyboardCode>(LOBYTE(vkey_and_modifiers));
    int win_modifiers = HIBYTE(vkey_and_modifiers);
    int modifiers = 0;
    if (win_modifiers & 0x01)
      modifiers |= kShiftKeyModifierMask;
    if (win_modifiers & 0x02)
      modifiers |= kControlKeyModifierMask;
    if (win_modifiers & 0x04)
      modifiers |= kAltKeyModifierMask;
    // Ignore bit 0x08: It is for Hankaku key.
    *necessary_modifiers = modifiers;
  }
  return translated;
}

bool SwitchToUSKeyboardLayout() {
  // For LoadKeyboardLayout - Prior to Windows 8: If the specified input
  // locale identifier is not already loaded, the function loads and
  // activates the input locale identifier for the current thread.
  // Beginning in Windows 8: If the specified input locale identifier is not
  // already loaded, the function loads and activates the input
  // locale identifier for the system.
  // For Windows 8 - Use ActivateKeyboardLayout instead of LoadKeyboardLayout
  LPCTSTR kUsKeyboardLayout = TEXT("00000409");

  if (IsWindows8OrGreater()) {
    int size;
    TCHAR active_keyboard[KL_NAMELENGTH];

    if ((size = ::GetKeyboardLayoutList(0, NULL)) <= 0)
      return false;

    std::unique_ptr<HKL[]> keyboard_handles_list(new HKL[size]);
    ::GetKeyboardLayoutList(size, keyboard_handles_list.get());

    for (int keyboard_index = 0; keyboard_index < size; keyboard_index++) {
      ::ActivateKeyboardLayout(keyboard_handles_list[keyboard_index],
          KLF_SETFORPROCESS);
      ::GetKeyboardLayoutName(active_keyboard);
      if (wcscmp(active_keyboard, kUsKeyboardLayout) == 0)
        return true;
    }
    return false;
  } else {
    return ::LoadKeyboardLayout(kUsKeyboardLayout, KLF_ACTIVATE) != NULL;
  }
}
