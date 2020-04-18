// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/ozone/platform/wayland/wayland_xkb_keyboard_layout_engine.h"

#include "ui/events/event_constants.h"
#include "ui/events/event_modifiers.h"

namespace ui {

void WaylandXkbKeyboardLayoutEngine::SetKeymap(xkb_keymap* keymap) {
  XkbKeyboardLayoutEngine::SetKeymap(keymap);

  xkb_mod_indexes_.control =
      xkb_keymap_mod_get_index(keymap, XKB_MOD_NAME_CTRL);
  xkb_mod_indexes_.alt = xkb_keymap_mod_get_index(keymap, XKB_MOD_NAME_ALT);
  xkb_mod_indexes_.shift = xkb_keymap_mod_get_index(keymap, XKB_MOD_NAME_SHIFT);
  xkb_mod_indexes_.caps = xkb_keymap_mod_get_index(keymap, XKB_MOD_NAME_CAPS);
}

void WaylandXkbKeyboardLayoutEngine::UpdateModifiers(uint32_t depressed_mods,
                                                     uint32_t latched_mods,
                                                     uint32_t locked_mods,
                                                     uint32_t group) {
  xkb_state_update_mask(xkb_state_.get(), depressed_mods, latched_mods,
                        locked_mods, 0, 0, group);

  event_modifiers_->ResetKeyboardModifiers();

  auto component = static_cast<xkb_state_component>(XKB_STATE_MODS_DEPRESSED |
                                                    XKB_STATE_MODS_LATCHED |
                                                    XKB_STATE_MODS_LOCKED);
  if (xkb_state_mod_index_is_active(xkb_state_.get(), xkb_mod_indexes_.control,
                                    component))
    event_modifiers_->UpdateModifier(MODIFIER_CONTROL, true);

  if (xkb_state_mod_index_is_active(xkb_state_.get(), xkb_mod_indexes_.alt,
                                    component))
    event_modifiers_->UpdateModifier(MODIFIER_ALT, true);

  if (xkb_state_mod_index_is_active(xkb_state_.get(), xkb_mod_indexes_.shift,
                                    component))
    event_modifiers_->UpdateModifier(MODIFIER_SHIFT, true);

  if (xkb_state_mod_index_is_active(xkb_state_.get(), xkb_mod_indexes_.caps,
                                    component))
    event_modifiers_->SetModifierLock(MODIFIER_CAPS_LOCK, true);
  else
    event_modifiers_->SetModifierLock(MODIFIER_CAPS_LOCK, false);
}

void WaylandXkbKeyboardLayoutEngine::SetEventModifiers(
    EventModifiers* event_modifiers) {
  event_modifiers_ = event_modifiers;
}

}  // namespace ui
