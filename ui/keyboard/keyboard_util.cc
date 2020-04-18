// Copyright (c) 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/keyboard/keyboard_util.h"

#include <string>

#include "base/command_line.h"
#include "base/feature_list.h"
#include "base/lazy_instance.h"
#include "base/logging.h"
#include "base/metrics/histogram_macros.h"
#include "base/strings/string16.h"
#include "ui/aura/client/aura_constants.h"
#include "ui/aura/window_tree_host.h"
#include "ui/base/ime/input_method.h"
#include "ui/base/ime/input_method_base.h"
#include "ui/base/ime/text_input_client.h"
#include "ui/base/ime/text_input_flags.h"
#include "ui/base/ui_base_features.h"
#include "ui/base/ui_base_switches.h"
#include "ui/events/event_sink.h"
#include "ui/events/event_utils.h"
#include "ui/events/keycodes/dom/dom_code.h"
#include "ui/events/keycodes/dom/dom_key.h"
#include "ui/events/keycodes/dom/keycode_converter.h"
#include "ui/events/keycodes/keyboard_code_conversion.h"
#include "ui/keyboard/keyboard_controller.h"
#include "ui/keyboard/keyboard_switches.h"
#include "ui/keyboard/keyboard_ui.h"

namespace keyboard {

namespace {

const char kKeyDown[] ="keydown";
const char kKeyUp[] = "keyup";

void SendProcessKeyEvent(ui::EventType type,
                         aura::WindowTreeHost* host) {
  ui::KeyEvent event(type, ui::VKEY_PROCESSKEY, ui::DomCode::NONE,
                     ui::EF_IS_SYNTHESIZED, ui::DomKey::PROCESS,
                     ui::EventTimeForNow());
  ui::EventDispatchDetails details =
      host->event_sink()->OnEventFromSource(&event);
  CHECK(!details.dispatcher_destroyed);
}

bool g_keyboard_load_time_logged = false;
base::LazyInstance<base::Time>::DestructorAtExit g_keyboard_load_time_start =
    LAZY_INSTANCE_INITIALIZER;

struct keyboard::KeyboardConfig g_keyboard_config;

bool g_accessibility_keyboard_enabled = false;

bool g_hotrod_keyboard_enabled = false;

bool g_touch_keyboard_enabled = false;

KeyboardState g_requested_keyboard_state = KEYBOARD_STATE_AUTO;

KeyboardOverscrolOverride g_keyboard_overscroll_override =
    KEYBOARD_OVERSCROLL_OVERRIDE_NONE;

KeyboardShowOverride g_keyboard_show_override = KEYBOARD_SHOW_OVERRIDE_NONE;

}  // namespace

bool UpdateKeyboardConfig(const KeyboardConfig& keyboard_config) {
  if (g_keyboard_config == keyboard_config)
    return false;
  g_keyboard_config = keyboard_config;
  keyboard::KeyboardController* controller = KeyboardController::GetInstance();
  if (controller)
    controller->NotifyKeyboardConfigChanged();
  return true;
}

const KeyboardConfig& GetKeyboardConfig() {
  return g_keyboard_config;
}

void SetAccessibilityKeyboardEnabled(bool enabled) {
  g_accessibility_keyboard_enabled = enabled;
}

bool GetAccessibilityKeyboardEnabled() {
  return g_accessibility_keyboard_enabled;
}

void SetHotrodKeyboardEnabled(bool enabled) {
  g_hotrod_keyboard_enabled = enabled;
}

bool GetHotrodKeyboardEnabled() {
  return g_hotrod_keyboard_enabled;
}

void SetTouchKeyboardEnabled(bool enabled) {
  g_touch_keyboard_enabled = enabled;
}

bool GetTouchKeyboardEnabled() {
  return g_touch_keyboard_enabled;
}

void SetRequestedKeyboardState(KeyboardState state) {
  g_requested_keyboard_state = state;
}

KeyboardState GetKeyboardRequestedState() {
  return g_requested_keyboard_state;
}

std::string GetKeyboardLayout() {
  // TODO(bshe): layout string is currently hard coded. We should use more
  // standard keyboard layouts.
  return GetAccessibilityKeyboardEnabled() ? "system-qwerty" : "qwerty";
}

bool IsKeyboardEnabled() {
  // Accessibility setting prioritized over policy setting.
  if (g_accessibility_keyboard_enabled)
    return true;
  // Policy strictly disables showing a virtual keyboard.
  if (g_keyboard_show_override == KEYBOARD_SHOW_OVERRIDE_DISABLED)
    return false;
  // Policy strictly enables the keyboard.
  if (g_keyboard_show_override == KEYBOARD_SHOW_OVERRIDE_ENABLED)
    return true;
  // Run-time flag to enable keyboard has been included.
  if (base::CommandLine::ForCurrentProcess()->HasSwitch(
          switches::kEnableVirtualKeyboard))
    return true;
  // Requested state from the application layer.
  if (g_requested_keyboard_state == KEYBOARD_STATE_DISABLED)
    return false;
  // Check if any of the other flags are enabled.
  return g_touch_keyboard_enabled ||
         g_requested_keyboard_state == KEYBOARD_STATE_ENABLED;
}

bool IsKeyboardVisible() {
  auto* keyboard_controller = keyboard::KeyboardController::GetInstance();
  return keyboard_controller && keyboard_controller->keyboard_visible();
}

bool IsKeyboardOverscrollEnabled() {
  if (!IsKeyboardEnabled())
    return false;

  // Users of the sticky accessibility on-screen keyboard are likely to be using
  // mouse input, which may interfere with overscrolling.
  if (keyboard::KeyboardController::GetInstance() &&
      !keyboard::KeyboardController::GetInstance()->IsOverscrollAllowed()) {
    return false;
  }

  // If overscroll enabled override is set, use it instead. Currently
  // login / out-of-box disable keyboard overscroll. http://crbug.com/363635
  if (g_keyboard_overscroll_override != KEYBOARD_OVERSCROLL_OVERRIDE_NONE) {
    return g_keyboard_overscroll_override ==
        KEYBOARD_OVERSCROLL_OVERRIDE_ENABLED;
  }

  if (base::CommandLine::ForCurrentProcess()->HasSwitch(
          switches::kDisableVirtualKeyboardOverscroll)) {
    return false;
  }
  return true;
}

void SetKeyboardOverscrollOverride(KeyboardOverscrolOverride override) {
  g_keyboard_overscroll_override = override;
}

void SetKeyboardShowOverride(KeyboardShowOverride override) {
  g_keyboard_show_override = override;
}

bool IsInputViewEnabled() {
  return !base::CommandLine::ForCurrentProcess()->HasSwitch(
      switches::kDisableInputView);
}

bool IsExperimentalInputViewEnabled() {
  return base::CommandLine::ForCurrentProcess()->HasSwitch(
          switches::kEnableExperimentalInputViewFeatures);
}

bool IsFloatingVirtualKeyboardEnabled() {
  return base::FeatureList::IsEnabled(features::kEnableFloatingVirtualKeyboard);
}

bool IsFullscreenHandwritingVirtualKeyboardEnabled() {
  return base::FeatureList::IsEnabled(
      features::kEnableFullscreenHandwritingVirtualKeyboard);
}

bool IsStylusVirtualKeyboardEnabled() {
  return base::FeatureList::IsEnabled(features::kEnableStylusVirtualKeyboard);
}

bool IsGestureTypingEnabled() {
  return !base::CommandLine::ForCurrentProcess()->HasSwitch(
      switches::kDisableGestureTyping);
}

bool IsGestureEditingEnabled() {
  return !base::CommandLine::ForCurrentProcess()->HasSwitch(
      switches::kDisableGestureEditing);
}

bool InsertText(const base::string16& text) {
  KeyboardController* controller = KeyboardController::GetInstance();
  if (!controller)
    return false;

  ui::InputMethod* input_method = controller->ui()->GetInputMethod();
  if (!input_method)
    return false;

  ui::TextInputClient* tic = input_method->GetTextInputClient();
  if (!tic || tic->GetTextInputType() == ui::TEXT_INPUT_TYPE_NONE)
    return false;

  tic->InsertText(text);

  return true;
}

bool SendKeyEvent(const std::string type,
                  int key_value,
                  int key_code,
                  std::string key_name,
                  int modifiers,
                  aura::WindowTreeHost* host) {
  ui::EventType event_type = ui::ET_UNKNOWN;
  if (type == kKeyDown)
    event_type = ui::ET_KEY_PRESSED;
  else if (type == kKeyUp)
    event_type = ui::ET_KEY_RELEASED;
  if (event_type == ui::ET_UNKNOWN)
    return false;

  ui::KeyboardCode code = static_cast<ui::KeyboardCode>(key_code);

  ui::InputMethod* input_method = host->GetInputMethod();
  if (code == ui::VKEY_UNKNOWN) {
    // Handling of special printable characters (e.g. accented characters) for
    // which there is no key code.
    if (event_type == ui::ET_KEY_RELEASED) {
      if (!input_method)
        return false;

      // This can be null if no text input field is not focused.
      ui::TextInputClient* tic = input_method->GetTextInputClient();

      SendProcessKeyEvent(ui::ET_KEY_PRESSED, host);

      ui::KeyEvent char_event(key_value, code, ui::EF_NONE);
      if (tic)
        tic->InsertChar(char_event);
      SendProcessKeyEvent(ui::ET_KEY_RELEASED, host);
    }
  } else {
    if (event_type == ui::ET_KEY_RELEASED) {
      // The number of key press events seen since the last backspace.
      static int keys_seen = 0;
      if (code == ui::VKEY_BACK) {
        // Log the rough lengths of characters typed between backspaces. This
        // metric will be used to determine the error rate for the keyboard.
        UMA_HISTOGRAM_CUSTOM_COUNTS(
            "VirtualKeyboard.KeystrokesBetweenBackspaces",
            keys_seen, 1, 1000, 50);
        keys_seen = 0;
      } else {
        ++keys_seen;
      }
    }

    ui::DomCode dom_code = ui::KeycodeConverter::CodeStringToDomCode(key_name);
    if (dom_code == ui::DomCode::NONE)
      dom_code = ui::UsLayoutKeyboardCodeToDomCode(code);
    CHECK(dom_code != ui::DomCode::NONE);
    ui::KeyEvent event(
        event_type,
        code,
        dom_code,
        modifiers);
    ui::EventDispatchDetails details =
        host->event_sink()->OnEventFromSource(&event);
    CHECK(!details.dispatcher_destroyed);
  }
  return true;
}

void MarkKeyboardLoadStarted() {
  if (!g_keyboard_load_time_logged)
    g_keyboard_load_time_start.Get() = base::Time::Now();
}

void MarkKeyboardLoadFinished() {
  // Possible to get a load finished without a start if navigating directly to
  // chrome://keyboard.
  if (g_keyboard_load_time_start.Get().is_null())
    return;

  if (!g_keyboard_load_time_logged) {
    // Log the delta only once.
    UMA_HISTOGRAM_TIMES(
        "VirtualKeyboard.InitLatency.FirstLoad",
        base::Time::Now() - g_keyboard_load_time_start.Get());
    g_keyboard_load_time_logged = true;
  }
}

void LogKeyboardControlEvent(KeyboardControlEvent event) {
  UMA_HISTOGRAM_ENUMERATION("VirtualKeyboard.KeyboardControlEvent", event,
                            KEYBOARD_CONTROL_MAX);
}

}  // namespace keyboard
