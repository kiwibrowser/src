// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "services/ui/public/cpp/input_devices/input_device_controller.h"

#include <utility>

#include "base/bind.h"
#include "ui/events/devices/input_device.h"
#include "ui/events/devices/touchscreen_device.h"
#include "ui/events/keycodes/dom/dom_code.h"
#include "ui/events/keycodes/dom/keycode_converter.h"
#include "ui/ozone/public/input_controller.h"
#include "ui/ozone/public/ozone_platform.h"

namespace ui {

InputDeviceController::InputDeviceController() = default;

InputDeviceController::~InputDeviceController() = default;

void InputDeviceController::AddInterface(
    service_manager::BinderRegistry* registry,
    const scoped_refptr<base::SequencedTaskRunner>& task_runner) {
  // base::Unretained() is safe here as this class is tied to the life of
  // Service, so that no requests should come in after this class is deleted.
  registry->AddInterface<mojom::InputDeviceController>(
      base::Bind(&InputDeviceController::BindInputDeviceControllerRequest,
                 base::Unretained(this)),
      task_runner);
}

void InputDeviceController::AddKeyboardDeviceObserver(
    mojom::KeyboardDeviceObserverPtr observer) {
  NotifyObserver(observer.get());
  observers_.AddPtr(std::move(observer));
}

void InputDeviceController::GetHasTouchpad(GetHasTouchpadCallback callback) {
  std::move(callback).Run(GetInputController()->HasTouchpad());
}

void InputDeviceController::GetHasMouse(GetHasMouseCallback callback) {
  std::move(callback).Run(GetInputController()->HasMouse());
}

void InputDeviceController::SetCapsLockEnabled(bool enabled) {
  GetInputController()->SetCapsLockEnabled(enabled);
  NotifyObservers();
}

void InputDeviceController::SetNumLockEnabled(bool enabled) {
  GetInputController()->SetNumLockEnabled(enabled);
}

void InputDeviceController::SetAutoRepeatEnabled(bool enabled) {
  GetInputController()->SetAutoRepeatEnabled(enabled);
  NotifyObservers();
}

void InputDeviceController::SetAutoRepeatRate(
    base::TimeDelta auto_repeat_delay,
    base::TimeDelta auto_repeat_interval) {
  GetInputController()->SetAutoRepeatRate(auto_repeat_delay,
                                          auto_repeat_interval);
}

void InputDeviceController::SetKeyboardLayoutByName(const std::string& name) {
  GetInputController()->SetCurrentLayoutByName(name);
}

void InputDeviceController::SetTouchpadSensitivity(int32_t value) {
  GetInputController()->SetTouchpadSensitivity(value);
}

void InputDeviceController::SetTapToClick(bool enabled) {
  GetInputController()->SetTapToClick(enabled);
}

void InputDeviceController::SetThreeFingerClick(bool enabled) {
  GetInputController()->SetThreeFingerClick(enabled);
}

void InputDeviceController::SetTapDragging(bool enabled) {
  GetInputController()->SetTapDragging(enabled);
}

void InputDeviceController::SetNaturalScroll(bool enabled) {
  GetInputController()->SetNaturalScroll(enabled);
}

void InputDeviceController::SetMouseSensitivity(int32_t value) {
  GetInputController()->SetMouseSensitivity(value);
}

void InputDeviceController::SetPrimaryButtonRight(bool right) {
  GetInputController()->SetPrimaryButtonRight(right);
}

void InputDeviceController::SetMouseReverseScroll(bool enabled) {
  GetInputController()->SetMouseReverseScroll(enabled);
}

void InputDeviceController::GetTouchDeviceStatus(
    GetTouchDeviceStatusCallback callback) {
  GetInputController()->GetTouchDeviceStatus(std::move(callback));
}

void InputDeviceController::GetTouchEventLog(
    const base::FilePath& out_dir,
    GetTouchEventLogCallback callback) {
  GetInputController()->GetTouchEventLog(out_dir, std::move(callback));
}

void InputDeviceController::SetTapToClickPaused(bool state) {
  GetInputController()->SetTapToClickPaused(state);
}

void InputDeviceController::SetInternalTouchpadEnabled(
    bool enabled,
    SetInternalTouchpadEnabledCallback callback) {
  InputController* input_controller = GetInputController();
  const bool value_changed =
      input_controller->HasTouchpad() &&
      (input_controller->IsInternalTouchpadEnabled() != enabled);
  if (value_changed)
    input_controller->SetInternalTouchpadEnabled(enabled);
  std::move(callback).Run(value_changed);
}

void InputDeviceController::SetTouchscreensEnabled(bool enabled) {
  GetInputController()->SetTouchscreensEnabled(enabled);
}

void InputDeviceController::SetInternalKeyboardFilter(
    bool enable_filter,
    const std::vector<uint32_t>& allowed_keys) {
  std::vector<DomCode> dom_codes;
  for (uint32_t key : allowed_keys) {
    // NOTE: DomCodes and UsbKeycodes are the same thing.
    const DomCode dom_code = KeycodeConverter::UsbKeycodeToDomCode(key);
    if (dom_code != DomCode::NONE)
      dom_codes.push_back(dom_code);
  }
  GetInputController()->SetInternalKeyboardFilter(enable_filter,
                                                  std::move(dom_codes));
}

ui::InputController* InputDeviceController::GetInputController() {
  return OzonePlatform::GetInstance()->GetInputController();
}

void InputDeviceController::NotifyObservers() {
  observers_.ForAllPtrs([this](mojom::KeyboardDeviceObserver* observer) {
    NotifyObserver(observer);
  });
}

void InputDeviceController::NotifyObserver(
    mojom::KeyboardDeviceObserver* observer) {
  mojom::KeyboardDeviceStatePtr state = mojom::KeyboardDeviceState::New();
  ui::InputController* input_controller = GetInputController();
  state->is_caps_lock_enabled = input_controller->IsCapsLockEnabled();
  state->is_auto_repeat_enabled = input_controller->IsAutoRepeatEnabled();
  observer->OnKeyboardStateChanged(std::move(state));
}

void InputDeviceController::BindInputDeviceControllerRequest(
    mojom::InputDeviceControllerRequest request) {
  bindings_.AddBinding(this, std::move(request));
}

}  // namespace ui
