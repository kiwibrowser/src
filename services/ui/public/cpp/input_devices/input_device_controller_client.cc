// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "services/ui/public/cpp/input_devices/input_device_controller_client.h"

#include <utility>

#include "base/bind_helpers.h"
#include "services/service_manager/public/cpp/connector.h"
#include "services/ui/public/interfaces/constants.mojom.h"

namespace ui {

InputDeviceControllerClient::InputDeviceControllerClient(
    service_manager::Connector* connector,
    const std::string& service_name)
    : binding_(this) {
  connector->BindInterface(
      service_name.empty() ? mojom::kServiceName : service_name,
      &input_device_controller_);
  mojom::KeyboardDeviceObserverPtr ptr;
  binding_.Bind(mojo::MakeRequest(&ptr));
  input_device_controller_->AddKeyboardDeviceObserver(std::move(ptr));
}

InputDeviceControllerClient::~InputDeviceControllerClient() = default;

void InputDeviceControllerClient::GetHasMouse(GetHasMouseCallback callback) {
  input_device_controller_->GetHasMouse(std::move(callback));
}

void InputDeviceControllerClient::GetHasTouchpad(
    GetHasTouchpadCallback callback) {
  input_device_controller_->GetHasTouchpad(std::move(callback));
}

bool InputDeviceControllerClient::IsCapsLockEnabled() {
  return keyboard_device_state_.is_caps_lock_enabled;
}

void InputDeviceControllerClient::SetCapsLockEnabled(bool enabled) {
  keyboard_device_state_.is_caps_lock_enabled = enabled;
  input_device_controller_->SetCapsLockEnabled(enabled);
}

void InputDeviceControllerClient::SetNumLockEnabled(bool enabled) {
  input_device_controller_->SetNumLockEnabled(enabled);
}

bool InputDeviceControllerClient::IsAutoRepeatEnabled() {
  return keyboard_device_state_.is_auto_repeat_enabled;
}

void InputDeviceControllerClient::SetAutoRepeatEnabled(bool enabled) {
  keyboard_device_state_.is_auto_repeat_enabled = enabled;
  input_device_controller_->SetAutoRepeatEnabled(enabled);
}

void InputDeviceControllerClient::SetAutoRepeatRate(base::TimeDelta delay,
                                                    base::TimeDelta interval) {
  input_device_controller_->SetAutoRepeatRate(delay, interval);
}

void InputDeviceControllerClient::SetKeyboardLayoutByName(
    const std::string& layout_name) {
  input_device_controller_->SetKeyboardLayoutByName(layout_name);
}

void InputDeviceControllerClient::SetTouchpadSensitivity(int value) {
  input_device_controller_->SetTouchpadSensitivity(value);
}

void InputDeviceControllerClient::SetTapToClick(bool enable) {
  input_device_controller_->SetTapToClick(enable);
}

void InputDeviceControllerClient::SetThreeFingerClick(bool enable) {
  input_device_controller_->SetThreeFingerClick(enable);
}

void InputDeviceControllerClient::SetTapDragging(bool enable) {
  input_device_controller_->SetTapDragging(enable);
}

void InputDeviceControllerClient::SetNaturalScroll(bool enable) {
  input_device_controller_->SetNaturalScroll(enable);
}

void InputDeviceControllerClient::SetMouseSensitivity(int value) {
  input_device_controller_->SetMouseSensitivity(value);
}

void InputDeviceControllerClient::SetPrimaryButtonRight(bool right) {
  input_device_controller_->SetPrimaryButtonRight(right);
}

void InputDeviceControllerClient::SetMouseReverseScroll(bool enabled) {
  input_device_controller_->SetMouseReverseScroll(enabled);
}

void InputDeviceControllerClient::GetTouchDeviceStatus(
    GetTouchDeviceStatusCallback callback) {
  input_device_controller_->GetTouchDeviceStatus(std::move(callback));
}

void InputDeviceControllerClient::GetTouchEventLog(
    const base::FilePath& out_dir,
    GetTouchEventLogCallback callback) {
  input_device_controller_->GetTouchEventLog(out_dir, std::move(callback));
}

void InputDeviceControllerClient::SetTapToClickPaused(bool state) {
  input_device_controller_->SetTapToClickPaused(state);
}

void InputDeviceControllerClient::SetTouchscreensEnabled(bool enable) {
  input_device_controller_->SetTouchscreensEnabled(enable);
}

void InputDeviceControllerClient::SetInternalKeyboardFilter(
    bool enable_filter,
    const std::vector<DomCode>& allowed_keys) {
  std::vector<uint32_t> transport_keys(allowed_keys.size());
  for (size_t i = 0; i < allowed_keys.size(); ++i)
    transport_keys[i] = static_cast<uint32_t>(allowed_keys[i]);
  input_device_controller_->SetInternalKeyboardFilter(enable_filter,
                                                      transport_keys);
}

void InputDeviceControllerClient::SetInternalTouchpadEnabled(
    bool enable,
    SetInternalTouchpadEnabledCallback callback) {
  input_device_controller_->SetInternalTouchpadEnabled(enable,
                                                       std::move(callback));
}

void InputDeviceControllerClient::OnKeyboardStateChanged(
    mojom::KeyboardDeviceStatePtr state) {
  keyboard_device_state_ = *state;
}

}  // namespace ui
