// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ASH_IME_TEST_IME_CONTROLLER_H_
#define ASH_IME_TEST_IME_CONTROLLER_H_

#include <memory>
#include <string>
#include <utility>

#include "ash/ash_export.h"
#include "ash/public/interfaces/ime_controller.mojom.h"
#include "mojo/public/cpp/bindings/binding.h"

namespace ash {

class ASH_EXPORT TestImeController : mojom::ImeController {
 public:
  TestImeController();
  ~TestImeController() override;

  // Returns a mojo interface pointer bound to this object.
  mojom::ImeControllerPtr CreateInterfacePtr();

  // mojom::ImeController:
  void SetClient(mojom::ImeControllerClientPtr client) override;
  void RefreshIme(const std::string& current_ime_id,
                  std::vector<mojom::ImeInfoPtr> available_imes,
                  std::vector<mojom::ImeMenuItemPtr> menu_items) override;
  void SetImesManagedByPolicy(bool managed) override;
  void ShowImeMenuOnShelf(bool show) override;
  void UpdateCapsLockState(bool enabled) override;
  void OnKeyboardLayoutNameChanged(const std::string& layout_name) override;
  void SetExtraInputOptionsEnabledState(bool is_extra_input_options_enabled,
                                        bool is_emoji_enabled,
                                        bool is_handwriting_enabled,
                                        bool is_voice_enabled) override;

  // The most recent values received via mojo.
  std::string current_ime_id_;
  std::vector<mojom::ImeInfoPtr> available_imes_;
  std::vector<mojom::ImeMenuItemPtr> menu_items_;
  bool managed_by_policy_ = false;
  bool show_ime_menu_on_shelf_ = false;
  bool is_caps_lock_enabled_ = false;
  std::string keyboard_layout_name_;
  bool is_extra_input_options_enabled_ = false;
  bool is_emoji_enabled_ = false;
  bool is_handwriting_enabled_ = false;
  bool is_voice_enabled_ = false;

 private:
  mojo::Binding<mojom::ImeController> binding_;

  DISALLOW_COPY_AND_ASSIGN(TestImeController);
};

}  // namespace ash

#endif  // ASH_IME_TEST_IME_CONTROLLER_H_
