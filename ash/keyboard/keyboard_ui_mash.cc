// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ash/keyboard/keyboard_ui_mash.h"

#include <memory>

#include "ash/keyboard/keyboard_ui_observer.h"
#include "services/service_manager/public/cpp/connector.h"

namespace ash {

KeyboardUIMash::KeyboardUIMash(service_manager::Connector* connector)
    : is_enabled_(false), observer_binding_(this) {
  // TODO: chrome should register the keyboard interface with ash.
  // http://crbug.com/683289.
}

KeyboardUIMash::~KeyboardUIMash() = default;

// static
std::unique_ptr<KeyboardUI> KeyboardUIMash::Create(
    service_manager::Connector* connector) {
  return std::make_unique<KeyboardUIMash>(connector);
}

void KeyboardUIMash::Hide() {
  if (keyboard_)
    keyboard_->Hide();
}

void KeyboardUIMash::ShowInDisplay(const display::Display& display) {
  // TODO(yhanada): Send display id after adding a display_id argument to
  // |Keyboard::Show()| in keyboard.mojom. See crbug.com/585253.
  if (keyboard_)
    keyboard_->Show();
}

bool KeyboardUIMash::IsEnabled() {
  return is_enabled_;
}

void KeyboardUIMash::OnKeyboardStateChanged(bool is_enabled,
                                            bool is_visible,
                                            uint64_t display_id,
                                            const gfx::Rect& bounds) {
  if (is_enabled_ == is_enabled)
    return;

  is_enabled_ = is_enabled;
  for (auto& observer : *observers())
    observer.OnKeyboardEnabledStateChanged(is_enabled);
}

}  // namespace ash
