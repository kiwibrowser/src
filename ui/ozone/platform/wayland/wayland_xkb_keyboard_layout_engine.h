// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UI_OZONE_PLATFORM_WAYLAND_WAYLAND_XKB_KEYBOARD_LAYOUT_ENGINE_H_
#define UI_OZONE_PLATFORM_WAYLAND_WAYLAND_XKB_KEYBOARD_LAYOUT_ENGINE_H_

#include "ui/events/ozone/layout/xkb/xkb_keyboard_layout_engine.h"

#include "ui/events/ozone/layout/xkb/xkb_key_code_converter.h"

namespace ui {

class EventModifiers;

class WaylandXkbKeyboardLayoutEngine : public XkbKeyboardLayoutEngine {
 public:
  WaylandXkbKeyboardLayoutEngine(const XkbKeyCodeConverter& converter)
      : XkbKeyboardLayoutEngine(converter) {}

  // Used to sync up client side 'xkb_state' instance with modifiers status
  // update from the compositor.
  void UpdateModifiers(uint32_t depressed_mods,
                       uint32_t latched_mods,
                       uint32_t locked_mods,
                       uint32_t group);

  void SetEventModifiers(EventModifiers* event_modifiers);

 private:
  void SetKeymap(xkb_keymap* keymap) override;

  // Cache to access modifiers xkb_mode_index_t value.
  struct {
    xkb_mod_index_t control = 0;
    xkb_mod_index_t alt = 0;
    xkb_mod_index_t shift = 0;
    xkb_mod_index_t caps = 0;
  } xkb_mod_indexes_;

  EventModifiers* event_modifiers_ = nullptr;  // Owned by WaylandKeyboard.
};

}  // namespace ui

#endif  // UI_EVENTS_OZONE_LAYOUT_XKB_XKB_KEYBOARD_LAYOUT_ENGINE_H_
