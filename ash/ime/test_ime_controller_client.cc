// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ash/ime/test_ime_controller_client.h"

#include <memory>
#include <string>
#include <utility>

#include "ash/public/interfaces/ime_controller.mojom.h"

namespace ash {

TestImeControllerClient::TestImeControllerClient() : binding_(this) {}

TestImeControllerClient::~TestImeControllerClient() = default;

mojom::ImeControllerClientPtr TestImeControllerClient::CreateInterfacePtr() {
  mojom::ImeControllerClientPtr ptr;
  binding_.Bind(mojo::MakeRequest(&ptr));
  return ptr;
}

void TestImeControllerClient::SwitchToNextIme() {
  ++next_ime_count_;
}

void TestImeControllerClient::SwitchToPreviousIme() {
  ++previous_ime_count_;
}

void TestImeControllerClient::SwitchImeById(const std::string& id,
                                            bool show_message) {
  ++switch_ime_count_;
  last_switch_ime_id_ = id;
  last_show_message_ = show_message;
}

void TestImeControllerClient::ActivateImeMenuItem(const std::string& key) {}

void TestImeControllerClient::SetCapsLockEnabled(bool enabled) {
  ++set_caps_lock_count_;
}

void TestImeControllerClient::OverrideKeyboardKeyset(
    chromeos::input_method::mojom::ImeKeyset keyset,
    OverrideKeyboardKeysetCallback callback) {
  last_keyset_ = keyset;
  std::move(callback).Run();
}

}  // namespace ash
