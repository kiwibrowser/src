// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ASH_IME_TEST_IME_CONTROLLER_CLIENT_H_
#define ASH_IME_TEST_IME_CONTROLLER_CLIENT_H_

#include "ash/public/interfaces/ime_controller.mojom.h"
#include "base/macros.h"
#include "mojo/public/cpp/bindings/binding.h"

namespace ash {

class TestImeControllerClient : public mojom::ImeControllerClient {
 public:
  TestImeControllerClient();
  ~TestImeControllerClient() override;

  mojom::ImeControllerClientPtr CreateInterfacePtr();

  // mojom::ImeControllerClient:
  void SwitchToNextIme() override;
  void SwitchToPreviousIme() override;
  void SwitchImeById(const std::string& id, bool show_message) override;
  void ActivateImeMenuItem(const std::string& key) override;
  void SetCapsLockEnabled(bool enabled) override;
  void OverrideKeyboardKeyset(chromeos::input_method::mojom::ImeKeyset keyset,
                              OverrideKeyboardKeysetCallback callback) override;

  int next_ime_count_ = 0;
  int previous_ime_count_ = 0;
  int switch_ime_count_ = 0;
  int set_caps_lock_count_ = 0;
  std::string last_switch_ime_id_;
  bool last_show_message_ = false;
  chromeos::input_method::mojom::ImeKeyset last_keyset_ =
      chromeos::input_method::mojom::ImeKeyset::kNone;

 private:
  mojo::Binding<mojom::ImeControllerClient> binding_;

  DISALLOW_COPY_AND_ASSIGN(TestImeControllerClient);
};

}  // namespace ash

#endif  // ASH_IME_TEST_IME_CONTROLLER_CLIENT_H_
