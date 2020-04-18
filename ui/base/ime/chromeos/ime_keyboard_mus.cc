// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/base/ime/chromeos/ime_keyboard_mus.h"

#include "services/ui/public/cpp/input_devices/input_device_controller_client.h"

namespace chromeos {
namespace input_method {

ImeKeyboardMus::ImeKeyboardMus(
    ui::InputDeviceControllerClient* input_device_controller_client)
    : input_device_controller_client_(input_device_controller_client) {
  ImeKeyboard::SetCapsLockEnabled(CapsLockIsEnabled());
}

ImeKeyboardMus::~ImeKeyboardMus() = default;

bool ImeKeyboardMus::SetCurrentKeyboardLayoutByName(
    const std::string& layout_name) {
  ImeKeyboard::SetCurrentKeyboardLayoutByName(layout_name);
  last_layout_ = layout_name;
  input_device_controller_client_->SetKeyboardLayoutByName(layout_name);
  return true;
}

bool ImeKeyboardMus::SetAutoRepeatRate(const AutoRepeatRate& rate) {
  input_device_controller_client_->SetAutoRepeatRate(
      base::TimeDelta::FromMilliseconds(rate.initial_delay_in_ms),
      base::TimeDelta::FromMilliseconds(rate.repeat_interval_in_ms));
  return true;
}

bool ImeKeyboardMus::SetAutoRepeatEnabled(bool enabled) {
  input_device_controller_client_->SetAutoRepeatEnabled(enabled);
  return true;
}

bool ImeKeyboardMus::GetAutoRepeatEnabled() {
  return input_device_controller_client_->IsAutoRepeatEnabled();
}

bool ImeKeyboardMus::ReapplyCurrentKeyboardLayout() {
  return SetCurrentKeyboardLayoutByName(last_layout_);
}

void ImeKeyboardMus::ReapplyCurrentModifierLockStatus() {}

void ImeKeyboardMus::DisableNumLock() {
  input_device_controller_client_->SetNumLockEnabled(false);
}

void ImeKeyboardMus::SetCapsLockEnabled(bool enable_caps_lock) {
  // Inform ImeKeyboard of caps lock state.
  ImeKeyboard::SetCapsLockEnabled(enable_caps_lock);
  // Inform Ozone InputController input of caps lock state.
  input_device_controller_client_->SetCapsLockEnabled(enable_caps_lock);
}

bool ImeKeyboardMus::CapsLockIsEnabled() {
  return input_device_controller_client_->IsCapsLockEnabled();
}

}  // namespace input_method
}  // namespace chromeos
