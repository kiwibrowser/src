// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ash/ime/test_ime_controller.h"

#include <memory>
#include <string>
#include <utility>

#include "ash/public/interfaces/ime_controller.mojom.h"

namespace ash {

TestImeController::TestImeController() : binding_(this) {}
TestImeController::~TestImeController() = default;

ash::mojom::ImeControllerPtr TestImeController::CreateInterfacePtr() {
  ash::mojom::ImeControllerPtr ptr;
  binding_.Bind(mojo::MakeRequest(&ptr));
  return ptr;
}

void TestImeController::SetClient(mojom::ImeControllerClientPtr client) {}

void TestImeController::RefreshIme(
    const std::string& current_ime_id,
    std::vector<ash::mojom::ImeInfoPtr> available_imes,
    std::vector<ash::mojom::ImeMenuItemPtr> menu_items) {
  current_ime_id_ = current_ime_id;
  available_imes_ = std::move(available_imes);
  menu_items_ = std::move(menu_items);
}

void TestImeController::SetImesManagedByPolicy(bool managed) {
  managed_by_policy_ = managed;
}

void TestImeController::ShowImeMenuOnShelf(bool show) {
  show_ime_menu_on_shelf_ = show;
}

void TestImeController::UpdateCapsLockState(bool enabled) {
  is_caps_lock_enabled_ = enabled;
}

void TestImeController::OnKeyboardLayoutNameChanged(
    const std::string& layout_name) {
  keyboard_layout_name_ = layout_name;
}

void TestImeController::SetExtraInputOptionsEnabledState(
    bool is_extra_input_options_enabled,
    bool is_emoji_enabled,
    bool is_handwriting_enabled,
    bool is_voice_enabled) {
  is_extra_input_options_enabled_ = is_extra_input_options_enabled;
  is_emoji_enabled_ = is_emoji_enabled;
  is_handwriting_enabled_ = is_handwriting_enabled;
  is_voice_enabled_ = is_voice_enabled;
}

}  // namespace ash
