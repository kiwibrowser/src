// Copyright (c) 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/test/chromedriver/key_converter.h"

#include <stddef.h>

#include "base/format_macros.h"
#include "base/macros.h"
#include "base/strings/stringprintf.h"
#include "base/strings/utf_string_conversions.h"
#include "chrome/test/chromedriver/chrome/status.h"
#include "chrome/test/chromedriver/chrome/ui_events.h"
#include "chrome/test/chromedriver/keycode_text_conversion.h"

namespace {

struct ModifierMaskAndKeyCode {
  int mask;
  ui::KeyboardCode key_code;
};

const ModifierMaskAndKeyCode kModifiers[] = {
  { kShiftKeyModifierMask, ui::VKEY_SHIFT },
  { kControlKeyModifierMask, ui::VKEY_CONTROL },
  { kAltKeyModifierMask, ui::VKEY_MENU }
};

// TODO(kkania): Use this in KeyMap.
// Ordered list of all the key codes corresponding to special WebDriver keys.
// These WebDriver keys are defined in the Unicode Private Use Area.
// http://code.google.com/p/selenium/wiki/JsonWireProtocol#/session/:sessionId/element/:id/value
const ui::KeyboardCode kSpecialWebDriverKeys[] = {
    ui::VKEY_UNKNOWN,
    ui::VKEY_UNKNOWN,
    ui::VKEY_HELP,
    ui::VKEY_BACK,
    ui::VKEY_TAB,
    ui::VKEY_CLEAR,
    ui::VKEY_RETURN,
    ui::VKEY_RETURN,
    ui::VKEY_SHIFT,
    ui::VKEY_CONTROL,
    ui::VKEY_MENU,
    ui::VKEY_PAUSE,
    ui::VKEY_ESCAPE,
    ui::VKEY_SPACE,
    ui::VKEY_PRIOR,    // page up
    ui::VKEY_NEXT,     // page down
    ui::VKEY_END,
    ui::VKEY_HOME,
    ui::VKEY_LEFT,
    ui::VKEY_UP,
    ui::VKEY_RIGHT,
    ui::VKEY_DOWN,
    ui::VKEY_INSERT,
    ui::VKEY_DELETE,
    ui::VKEY_OEM_1,     // semicolon
    ui::VKEY_OEM_PLUS,  // equals
    ui::VKEY_NUMPAD0,
    ui::VKEY_NUMPAD1,
    ui::VKEY_NUMPAD2,
    ui::VKEY_NUMPAD3,
    ui::VKEY_NUMPAD4,
    ui::VKEY_NUMPAD5,
    ui::VKEY_NUMPAD6,
    ui::VKEY_NUMPAD7,
    ui::VKEY_NUMPAD8,
    ui::VKEY_NUMPAD9,
    ui::VKEY_MULTIPLY,
    ui::VKEY_ADD,
    ui::VKEY_OEM_COMMA,
    ui::VKEY_SUBTRACT,
    ui::VKEY_DECIMAL,
    ui::VKEY_DIVIDE,
    ui::VKEY_UNKNOWN,
    ui::VKEY_UNKNOWN,
    ui::VKEY_UNKNOWN,
    ui::VKEY_UNKNOWN,
    ui::VKEY_UNKNOWN,
    ui::VKEY_UNKNOWN,
    ui::VKEY_UNKNOWN,
    ui::VKEY_F1,
    ui::VKEY_F2,
    ui::VKEY_F3,
    ui::VKEY_F4,
    ui::VKEY_F5,
    ui::VKEY_F6,
    ui::VKEY_F7,
    ui::VKEY_F8,
    ui::VKEY_F9,
    ui::VKEY_F10,
    ui::VKEY_F11,
    ui::VKEY_F12};

const base::char16 kWebDriverNullKey = 0xE000U;
const base::char16 kWebDriverShiftKey = 0xE008U;
const base::char16 kWebDriverControlKey = 0xE009U;
const base::char16 kWebDriverAltKey = 0xE00AU;
const base::char16 kWebDriverCommandKey = 0xE03DU;

// Returns whether the given key code has a corresponding printable char.
// Notice: The given key code should be a special WebDriver key code.
bool IsSpecialKeyPrintable(ui::KeyboardCode key_code) {
  return key_code == ui::VKEY_TAB || key_code == ui::VKEY_SPACE ||
      key_code == ui::VKEY_OEM_1 || key_code == ui::VKEY_OEM_PLUS ||
      key_code == ui::VKEY_OEM_COMMA ||
      (key_code >= ui::VKEY_NUMPAD0 && key_code <= ui::VKEY_DIVIDE);
}

// Returns whether the given key is a WebDriver key modifier.
bool IsModifierKey(base::char16 key) {
  switch (key) {
    case kWebDriverShiftKey:
    case kWebDriverControlKey:
    case kWebDriverAltKey:
    case kWebDriverCommandKey:
      return true;
    default:
      return false;
  }
}

// Gets the key code associated with |key|, if it is a special WebDriver key.
// Returns whether |key| is a special WebDriver key. If true, |key_code| will
// be set.
bool KeyCodeFromSpecialWebDriverKey(base::char16 key,
                                    ui::KeyboardCode* key_code) {
  int index = static_cast<int>(key) - 0xE000U;
  bool is_special_key = index >= 0 &&
      index < static_cast<int>(arraysize(kSpecialWebDriverKeys));
  if (is_special_key)
    *key_code = kSpecialWebDriverKeys[index];
  return is_special_key;
}

// Gets the key code associated with |key_utf16|, if it is a special shorthand
// key. Shorthand keys are common text equivalents for keys, such as the newline
// character, which is shorthand for the return key. Returns whether |key| is
// a shorthand key. If true, |key_code| will be set and |client_should_skip|
// will be set to whether the key should be skipped.
bool KeyCodeFromShorthandKey(base::char16 key_utf16,
                             ui::KeyboardCode* key_code,
                             bool* client_should_skip) {
  base::string16 key_str_utf16;
  key_str_utf16.push_back(key_utf16);
  std::string key_str_utf8 = base::UTF16ToUTF8(key_str_utf16);
  if (key_str_utf8.length() != 1)
    return false;
  bool should_skip = false;
  char key = key_str_utf8[0];
  if (key == '\n') {
    *key_code = ui::VKEY_RETURN;
  } else if (key == '\t') {
    *key_code = ui::VKEY_TAB;
  } else if (key == '\b') {
    *key_code = ui::VKEY_BACK;
  } else if (key == ' ') {
    *key_code = ui::VKEY_SPACE;
  } else if (key == '\r') {
    *key_code = ui::VKEY_UNKNOWN;
    should_skip = true;
  } else {
    return false;
  }
  *client_should_skip = should_skip;
  return true;
}

}  // namespace

Status ConvertKeysToKeyEvents(const base::string16& client_keys,
                              bool release_modifiers,
                              int* modifiers,
                              std::list<KeyEvent>* client_key_events) {
  std::list<KeyEvent> key_events;

  base::string16 keys = client_keys;
  // Add an implicit NULL character to the end of the input to depress all
  // modifiers.
  if (release_modifiers)
    keys.push_back(kWebDriverNullKey);

  int sticky_modifiers = *modifiers;
  for (size_t i = 0; i < keys.size(); ++i) {
    base::char16 key = keys[i];

    if (key == kWebDriverNullKey) {
      // Release all modifier keys and clear |stick_modifiers|.
      KeyEventBuilder builder;
      builder.SetType(kKeyUpEventType);
      if (sticky_modifiers & kShiftKeyModifierMask)
        key_events.push_back(builder.SetKeyCode(ui::VKEY_SHIFT)->Build());
      if (sticky_modifiers & kControlKeyModifierMask)
        key_events.push_back(builder.SetKeyCode(ui::VKEY_CONTROL)->Build());
      if (sticky_modifiers & kAltKeyModifierMask)
        key_events.push_back(builder.SetKeyCode(ui::VKEY_MENU)->Build());
      if (sticky_modifiers & kMetaKeyModifierMask)
        key_events.push_back(builder.SetKeyCode(ui::VKEY_COMMAND)->Build());
      sticky_modifiers = 0;
      continue;
    }
    if (IsModifierKey(key)) {
      // Press or release the modifier, and adjust |sticky_modifiers|.
      bool modifier_down = false;
      ui::KeyboardCode key_code = ui::VKEY_UNKNOWN;
      if (key == kWebDriverShiftKey) {
        sticky_modifiers ^= kShiftKeyModifierMask;
        modifier_down = (sticky_modifiers & kShiftKeyModifierMask) != 0;
        key_code = ui::VKEY_SHIFT;
      } else if (key == kWebDriverControlKey) {
        sticky_modifiers ^= kControlKeyModifierMask;
        modifier_down = (sticky_modifiers & kControlKeyModifierMask) != 0;
        key_code = ui::VKEY_CONTROL;
      } else if (key == kWebDriverAltKey) {
        sticky_modifiers ^= kAltKeyModifierMask;
        modifier_down = (sticky_modifiers & kAltKeyModifierMask) != 0;
        key_code = ui::VKEY_MENU;
      } else if (key == kWebDriverCommandKey) {
        sticky_modifiers ^= kMetaKeyModifierMask;
        modifier_down = (sticky_modifiers & kMetaKeyModifierMask) != 0;
        key_code = ui::VKEY_COMMAND;
      } else {
        return Status(kUnknownError, "unknown modifier key");
      }
      KeyEventBuilder builder;
      if (modifier_down)
        builder.SetType(kRawKeyDownEventType);
      else
        builder.SetType(kKeyUpEventType);
      key_events.push_back(builder.SetKeyCode(key_code)
                               ->SetModifiers(sticky_modifiers)
                               ->Build());
      continue;
    }

    ui::KeyboardCode key_code = ui::VKEY_UNKNOWN;
    std::string unmodified_text, modified_text;
    int all_modifiers = sticky_modifiers;

    // Get the key code, text, and modifiers for the given key.
    bool should_skip = false;
    bool is_special_key = KeyCodeFromSpecialWebDriverKey(key, &key_code);
    std::string error_msg;
    if (is_special_key ||
        KeyCodeFromShorthandKey(key, &key_code, &should_skip)) {
      if (should_skip)
        continue;
      if (key_code == ui::VKEY_UNKNOWN) {
        return Status(kUnknownError, base::StringPrintf(
            "unknown WebDriver key(%d) at string index (%" PRIuS ")",
            static_cast<int>(key),
            i));
      }
      if (key_code == ui::VKEY_RETURN) {
        // For some reason Chrome expects a carriage return for the return key.
        modified_text = unmodified_text = "\r";
      } else if (is_special_key && !IsSpecialKeyPrintable(key_code)) {
        // To prevent char event for special keys like DELETE.
        modified_text = unmodified_text = std::string();
      } else {
        // WebDriver assumes a numpad key should translate to the number,
        // which requires NumLock to be on with some platforms. This isn't
        // formally in the spec, but is expected by their tests.
        int webdriver_modifiers = 0;
        if (key_code >= ui::VKEY_NUMPAD0 && key_code <= ui::VKEY_NUMPAD9)
          webdriver_modifiers = kNumLockKeyModifierMask;
        if (!ConvertKeyCodeToText(
            key_code, webdriver_modifiers, &unmodified_text, &error_msg))
          return Status(kUnknownError, error_msg);
        if (!ConvertKeyCodeToText(
            key_code, all_modifiers | webdriver_modifiers, &modified_text,
            &error_msg))
          return Status(kUnknownError, error_msg);
      }
    } else {
      int necessary_modifiers = 0;
      ConvertCharToKeyCode(key, &key_code, &necessary_modifiers, &error_msg);
      if (!error_msg.empty())
        return Status(kUnknownError, error_msg);
      all_modifiers |= necessary_modifiers;
      if (key_code != ui::VKEY_UNKNOWN) {
        if (!ConvertKeyCodeToText(key_code, 0, &unmodified_text, &error_msg))
          return Status(kUnknownError, error_msg);
        if (!ConvertKeyCodeToText(
            key_code, all_modifiers, &modified_text, &error_msg))
          return Status(kUnknownError, error_msg);
        if (unmodified_text.empty() || modified_text.empty()) {
          // To prevent char event for special cases like CTRL + x (cut).
          unmodified_text.clear();
          modified_text.clear();
        }
      } else {
        // Do a best effort and use the raw key we were given.
        unmodified_text = base::UTF16ToUTF8(keys.substr(i, 1));
        modified_text = base::UTF16ToUTF8(keys.substr(i, 1));
      }
    }

    // Create the key events.
    bool necessary_modifiers[3];
    for (int i = 0; i < 3; ++i) {
      necessary_modifiers[i] =
          all_modifiers & kModifiers[i].mask &&
          !(sticky_modifiers & kModifiers[i].mask);
      if (necessary_modifiers[i]) {
        KeyEventBuilder builder;
        key_events.push_back(builder.SetType(kRawKeyDownEventType)
                                   ->SetKeyCode(kModifiers[i].key_code)
                                   ->SetModifiers(sticky_modifiers)
                                   ->Build());
      }
    }

    KeyEventBuilder builder;
    builder.SetModifiers(all_modifiers)
        ->SetText(unmodified_text, modified_text)
        ->SetKeyCode(key_code)
        ->Generate(&key_events);

    for (int i = 2; i > -1; --i) {
      if (necessary_modifiers[i]) {
        KeyEventBuilder builder;
        key_events.push_back(builder.SetType(kKeyUpEventType)
                                   ->SetKeyCode(kModifiers[i].key_code)
                                   ->SetModifiers(sticky_modifiers)
                                   ->Build());
      }
    }
  }
  client_key_events->swap(key_events);
  *modifiers = sticky_modifiers;
  return Status(kOk);
}
