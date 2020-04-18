// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/aura/mus/system_input_injector_mus.h"

#include "ui/aura/env.h"
#include "ui/aura/mus/window_manager_delegate.h"
#include "ui/display/display.h"
#include "ui/display/screen.h"
#include "ui/events/base_event_utils.h"
#include "ui/events/event.h"
#include "ui/events/keycodes/keyboard_code_conversion.h"
#include "ui/gfx/geometry/point_conversions.h"

namespace aura {

namespace {

int KeyboardCodeToModifier(ui::KeyboardCode key) {
  switch (key) {
    case ui::VKEY_MENU:
    case ui::VKEY_LMENU:
    case ui::VKEY_RMENU:
      return ui::MODIFIER_ALT;
    case ui::VKEY_ALTGR:
      return ui::MODIFIER_ALTGR;
    case ui::VKEY_CAPITAL:
      return ui::MODIFIER_CAPS_LOCK;
    case ui::VKEY_CONTROL:
    case ui::VKEY_LCONTROL:
    case ui::VKEY_RCONTROL:
      return ui::MODIFIER_CONTROL;
    case ui::VKEY_LWIN:
    case ui::VKEY_RWIN:
      return ui::MODIFIER_COMMAND;
    case ui::VKEY_SHIFT:
    case ui::VKEY_LSHIFT:
    case ui::VKEY_RSHIFT:
      return ui::MODIFIER_SHIFT;
    default:
      return ui::MODIFIER_NONE;
  }
}

}  // namespace

SystemInputInjectorMus::SystemInputInjectorMus(WindowManagerClient* client)
    : client_(client) {}

SystemInputInjectorMus::~SystemInputInjectorMus() {}

void SystemInputInjectorMus::MoveCursorTo(const gfx::PointF& location) {
  ui::MouseEvent event(
      ui::ET_MOUSE_MOVED, gfx::Point(), gfx::Point(), ui::EventTimeForNow(),
      modifiers_.GetModifierFlags(),
      /* changed_button_flags */ 0,
      ui::PointerDetails(ui::EventPointerType::POINTER_TYPE_MOUSE));
  event.set_location_f(location);
  event.set_root_location_f(location);

  InjectEventAt(ui::PointerEvent(event), gfx::ToRoundedPoint(location));
}

void SystemInputInjectorMus::InjectMouseButton(ui::EventFlags button,
                                               bool down) {
  gfx::Point location = aura::Env::GetInstance()->last_mouse_location();

  int modifier = ui::MODIFIER_NONE;
  switch (button) {
    case ui::EF_LEFT_MOUSE_BUTTON:
      modifier = ui::MODIFIER_LEFT_MOUSE_BUTTON;
      break;
    case ui::EF_RIGHT_MOUSE_BUTTON:
      modifier = ui::MODIFIER_RIGHT_MOUSE_BUTTON;
      break;
    case ui::EF_MIDDLE_MOUSE_BUTTON:
      modifier = ui::MODIFIER_MIDDLE_MOUSE_BUTTON;
      break;
    default:
      LOG(WARNING) << "Invalid flag: " << button << " for the button parameter";
      return;
  }

  int flag = modifiers_.GetEventFlagFromModifier(modifier);
  bool was_down = modifiers_.GetModifierFlags() & flag;
  modifiers_.UpdateModifier(modifier, down);
  down = modifiers_.GetModifierFlags() & flag;

  // Suppress nested clicks. EventModifiers counts presses, we only
  // dispatch an event on 0-1 (first press) and 1-0 (last release) transitions.
  if (down == was_down)
    return;

  ui::MouseEvent event(
      down ? ui::ET_MOUSE_PRESSED : ui::ET_MOUSE_RELEASED, location, location,
      ui::EventTimeForNow(), modifiers_.GetModifierFlags() | flag,
      /* changed_button_flags */ flag,
      ui::PointerDetails(ui::EventPointerType::POINTER_TYPE_MOUSE));
  InjectEventAt(ui::PointerEvent(event), location);
}

void SystemInputInjectorMus::InjectMouseWheel(int delta_x, int delta_y) {
  gfx::Point location = aura::Env::GetInstance()->last_mouse_location();

  ui::MouseWheelEvent event(gfx::Vector2d(delta_x, delta_y), location, location,
                            ui::EventTimeForNow(),
                            modifiers_.GetModifierFlags(),
                            /* changed_button_flags */ 0);
  InjectEventAt(ui::PointerEvent(event), location);
}

void SystemInputInjectorMus::InjectKeyEvent(ui::DomCode dom_code,
                                            bool down,
                                            bool suppress_auto_repeat) {
  // |suppress_auto_repeat| is always true, and can be ignored.
  ui::KeyboardCode key_code = ui::DomCodeToUsLayoutKeyboardCode(dom_code);

  int modifier = KeyboardCodeToModifier(key_code);
  if (modifier)
    UpdateModifier(modifier, down);

  ui::KeyEvent e(down ? ui::ET_KEY_PRESSED : ui::ET_KEY_RELEASED, key_code,
                 dom_code, modifiers_.GetModifierFlags());

  InjectEventAt(e, display::Screen::GetScreen()->GetCursorScreenPoint());
}

void SystemInputInjectorMus::InjectEventAt(const ui::Event& event,
                                           const gfx::Point& location) {
  display::Screen* screen = display::Screen::GetScreen();
  display::Display display = screen->GetDisplayNearestPoint(location);
  client_->InjectEvent(event, display.id());
}

void SystemInputInjectorMus::UpdateModifier(unsigned int modifier, bool down) {
  if (modifier == ui::MODIFIER_NONE)
    return;

  // KeyboardEvdev performs a transformation here from MODIFIER_CAPS_LOCK to
  // MODIFIER_MOD3. That was needed to work around X11, we actually want to
  // ship this state across the wire without modification.
  modifiers_.UpdateModifier(modifier, down);
}

}  // namespace aura
