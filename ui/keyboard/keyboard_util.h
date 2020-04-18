// Copyright (c) 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UI_KEYBOARD_KEYBOARD_UTIL_H_
#define UI_KEYBOARD_KEYBOARD_UTIL_H_

#include <string>

#include "base/strings/string16.h"
#include "ui/keyboard/keyboard_export.h"

namespace aura {
class WindowTreeHost;
}

namespace keyboard {

// For virtual keyboard IME extension.
struct KeyboardConfig {
  bool auto_complete = true;
  bool auto_correct = true;
  bool handwriting = true;
  bool spell_check = true;
  // It denotes the preferred value, and can be true even if there is no actual
  // audio input device.
  bool voice_input = true;

  bool operator==(const keyboard::KeyboardConfig& rhs) const {
    return auto_complete == rhs.auto_complete &&
           auto_correct == rhs.auto_correct && handwriting == rhs.handwriting &&
           spell_check == rhs.spell_check && voice_input == rhs.voice_input;
  }
};

// An enumeration of different keyboard control events that should be logged.
enum KeyboardControlEvent {
  KEYBOARD_CONTROL_SHOW = 0,
  KEYBOARD_CONTROL_HIDE_AUTO,
  KEYBOARD_CONTROL_HIDE_USER,
  KEYBOARD_CONTROL_MAX,
};

// An enumeration of keyboard overscroll override value.
enum KeyboardOverscrolOverride {
  KEYBOARD_OVERSCROLL_OVERRIDE_DISABLED = 0,
  KEYBOARD_OVERSCROLL_OVERRIDE_ENABLED,
  KEYBOARD_OVERSCROLL_OVERRIDE_NONE,
};

// An enumeration of keyboard policy settings.
enum KeyboardShowOverride {
  KEYBOARD_SHOW_OVERRIDE_DISABLED = 0,
  KEYBOARD_SHOW_OVERRIDE_ENABLED,
  KEYBOARD_SHOW_OVERRIDE_NONE,
};

// An enumeration of keyboard states.
enum KeyboardState {
  // Default state. System decides whether to show the keyboard or not.
  KEYBOARD_STATE_AUTO = 0,
  // Request virtual keyboard be deployed.
  KEYBOARD_STATE_ENABLED,
  // Request virtual keyboard be suppressed.
  KEYBOARD_STATE_DISABLED,
};

// Updates the current keyboard config with the given config is they are
// different, notifying to observers. Returns whether update happened.
KEYBOARD_EXPORT bool UpdateKeyboardConfig(
    const keyboard::KeyboardConfig& keyboard_config);

// Gets the current virtual keyboard IME config.
KEYBOARD_EXPORT const keyboard::KeyboardConfig& GetKeyboardConfig();

// Sets the state of the a11y onscreen keyboard.
KEYBOARD_EXPORT void SetAccessibilityKeyboardEnabled(bool enabled);

// Gets the state of the a11y onscreen keyboard.
KEYBOARD_EXPORT bool GetAccessibilityKeyboardEnabled();

// Sets the state of the hotrod onscreen keyboard.
KEYBOARD_EXPORT void SetHotrodKeyboardEnabled(bool enabled);

// Gets the state of the hotrod onscreen keyboard.
KEYBOARD_EXPORT bool GetHotrodKeyboardEnabled();

// Sets the state of the touch onscreen keyboard.
KEYBOARD_EXPORT void SetTouchKeyboardEnabled(bool enabled);

// Gets the state of the touch onscreen keyboard.
KEYBOARD_EXPORT bool GetTouchKeyboardEnabled();

// Sets the requested state of the keyboard.
KEYBOARD_EXPORT void SetRequestedKeyboardState(KeyboardState state);

// Gets the requested state of the keyboard.
KEYBOARD_EXPORT int GetRequestedKeyboardState();

// Gets the default keyboard layout.
KEYBOARD_EXPORT std::string GetKeyboardLayout();

// Returns true if the virtual keyboard is enabled.
KEYBOARD_EXPORT bool IsKeyboardEnabled();

// Returns true if the virtual keyboard is currently visible.
KEYBOARD_EXPORT bool IsKeyboardVisible();

// Returns true if keyboard overscroll mode is enabled.
KEYBOARD_EXPORT bool IsKeyboardOverscrollEnabled();

// Sets temporary keyboard overscroll override.
KEYBOARD_EXPORT void SetKeyboardOverscrollOverride(
    KeyboardOverscrolOverride override);

// Sets policy override on whether to show the keyboard.
KEYBOARD_EXPORT void SetKeyboardShowOverride(KeyboardShowOverride override);

// Returns true if an IME extension can specify a custom input view for the
// virtual keyboard window.
KEYBOARD_EXPORT bool IsInputViewEnabled();

// Sets whehther the keyboards is in restricted state - state where advanced
// virtual keyboard features are disabled.
KEYBOARD_EXPORT void SetKeyboardRestricted(bool restricted);

// Returns whether the keyboard is in restricted state.
KEYBOARD_EXPORT bool GetKeyboardRestricted();

// Returns true if experimental features are enabled for IME input-views.
KEYBOARD_EXPORT bool IsExperimentalInputViewEnabled();

// Returns true if floating virtual keyboard feature is enabled.
KEYBOARD_EXPORT bool IsFloatingVirtualKeyboardEnabled();

// Returns true if fullscreen handwriting virtual keyboard feature is enabled.
KEYBOARD_EXPORT bool IsFullscreenHandwritingVirtualKeyboardEnabled();

// Returns true if stylus virtual keyboard feature is enabled.
KEYBOARD_EXPORT bool IsStylusVirtualKeyboardEnabled();

// Returns true if gesture typing option is enabled for virtual keyboard.
KEYBOARD_EXPORT bool IsGestureTypingEnabled();

// Returns true if gesture editing option is enabled for virtual keyboard.
KEYBOARD_EXPORT bool IsGestureEditingEnabled();

// Returns true if voice input is not disabled for the keyboard by the command
// line switch. It's up to the client to check if there is an input device
// available.
KEYBOARD_EXPORT bool IsVoiceInputEnabled();

// Insert |text| into the active TextInputClient if there is one. Returns true
// if |text| was successfully inserted.
KEYBOARD_EXPORT bool InsertText(const base::string16& text);

// Sends a fabricated key event, where |type| is the event type, |key_value|
// is the unicode value of the character, |key_code| is the legacy key code
// value, |key_name| is the name of the key as defined in the DOM3 key event
// specification, and |modifier| indicates if any modifier keys are being
// virtually pressed. The event is dispatched to the active TextInputClient
// associated with |root_window|. The type may be "keydown" or "keyup".
KEYBOARD_EXPORT bool SendKeyEvent(std::string type,
                                  int key_value,
                                  int key_code,
                                  std::string key_name,
                                  int modifiers,
                                  aura::WindowTreeHost* host);

// Marks that the keyboard load has started. This is used to measure the time it
// takes to fully load the keyboard. This should be called before
// MarkKeyboardLoadFinished.
KEYBOARD_EXPORT void MarkKeyboardLoadStarted();

// Marks that the keyboard load has ended. This finishes measuring that the
// keyboard is loaded.
KEYBOARD_EXPORT void MarkKeyboardLoadFinished();

// Logs the keyboard control event as a UMA stat.
void LogKeyboardControlEvent(KeyboardControlEvent event);

}  // namespace keyboard

#endif  // UI_KEYBOARD_KEYBOARD_UTIL_H_
