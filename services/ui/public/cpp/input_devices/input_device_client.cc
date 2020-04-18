// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "services/ui/public/cpp/input_devices/input_device_client.h"

#include "base/logging.h"

namespace ui {

InputDeviceClient::InputDeviceClient() : InputDeviceClient(true) {}

InputDeviceClient::~InputDeviceClient() {
  if (is_input_device_manager_)
    InputDeviceManager::ClearInstance();
}

void InputDeviceClient::Connect(mojom::InputDeviceServerPtr server) {
  DCHECK(server.is_bound());
  server->AddObserver(GetIntefacePtr());
}

const std::vector<ui::InputDevice>& InputDeviceClient::GetKeyboardDevices()
    const {
  return keyboard_devices_;
}

const std::vector<ui::TouchscreenDevice>&
InputDeviceClient::GetTouchscreenDevices() const {
  return touchscreen_devices_;
}

const std::vector<ui::InputDevice>& InputDeviceClient::GetMouseDevices() const {
  return mouse_devices_;
}

const std::vector<ui::InputDevice>& InputDeviceClient::GetTouchpadDevices()
    const {
  return touchpad_devices_;
}

bool InputDeviceClient::AreDeviceListsComplete() const {
  return device_lists_complete_;
}

bool InputDeviceClient::AreTouchscreensEnabled() const {
  // TODO(kylechar): This obviously isn't right. We either need to pass this
  // state around or modify the interface.
  return true;
}

bool InputDeviceClient::AreTouchscreenTargetDisplaysValid() const {
  return are_touchscreen_target_displays_valid_;
}

void InputDeviceClient::AddObserver(ui::InputDeviceEventObserver* observer) {
  observers_.AddObserver(observer);
}

void InputDeviceClient::RemoveObserver(ui::InputDeviceEventObserver* observer) {
  observers_.RemoveObserver(observer);
}

InputDeviceClient::InputDeviceClient(bool is_input_device_manager)
    : binding_(this), is_input_device_manager_(is_input_device_manager) {
  if (is_input_device_manager_)
    InputDeviceManager::SetInstance(this);
}

mojom::InputDeviceObserverMojoPtr InputDeviceClient::GetIntefacePtr() {
  mojom::InputDeviceObserverMojoPtr ptr;
  binding_.Bind(mojo::MakeRequest(&ptr));
  return ptr;
}

void InputDeviceClient::OnKeyboardDeviceConfigurationChanged(
    const std::vector<ui::InputDevice>& devices) {
  keyboard_devices_ = devices;
  NotifyObserversKeyboardDeviceConfigurationChanged();
}

void InputDeviceClient::OnTouchscreenDeviceConfigurationChanged(
    const std::vector<ui::TouchscreenDevice>& devices,
    bool touchscreen_target_display_ids_changed) {
  if (touchscreen_target_display_ids_changed)
    DCHECK_EQ(touchscreen_devices_.size(), devices.size());

  touchscreen_devices_ = devices;
  if (touchscreen_target_display_ids_changed) {
    are_touchscreen_target_displays_valid_ = true;
    for (auto& observer : observers_)
      observer.OnTouchDeviceAssociationChanged();
  } else {
    are_touchscreen_target_displays_valid_ = false;
    NotifyObserversTouchscreenDeviceConfigurationChanged();
  }
}

void InputDeviceClient::OnMouseDeviceConfigurationChanged(
    const std::vector<ui::InputDevice>& devices) {
  mouse_devices_ = devices;
  for (auto& observer : observers_)
    observer.OnMouseDeviceConfigurationChanged();
}

void InputDeviceClient::OnTouchpadDeviceConfigurationChanged(
    const std::vector<ui::InputDevice>& devices) {
  touchpad_devices_ = devices;
  for (auto& observer : observers_)
    observer.OnTouchpadDeviceConfigurationChanged();
}

void InputDeviceClient::OnDeviceListsComplete(
    const std::vector<ui::InputDevice>& keyboard_devices,
    const std::vector<ui::TouchscreenDevice>& touchscreen_devices,
    const std::vector<ui::InputDevice>& mouse_devices,
    const std::vector<ui::InputDevice>& touchpad_devices,
    bool are_touchscreen_target_displays_valid) {
  are_touchscreen_target_displays_valid_ =
      are_touchscreen_target_displays_valid;
  // Update the cached device lists if the received list isn't empty.
  if (!keyboard_devices.empty())
    OnKeyboardDeviceConfigurationChanged(keyboard_devices);
  if (!touchscreen_devices.empty()) {
    touchscreen_devices_ = touchscreen_devices;
    are_touchscreen_target_displays_valid_ =
        are_touchscreen_target_displays_valid;
    NotifyObserversTouchscreenDeviceConfigurationChanged();
  }
  if (!mouse_devices.empty())
    OnMouseDeviceConfigurationChanged(mouse_devices);
  if (!touchpad_devices.empty())
    OnTouchpadDeviceConfigurationChanged(touchpad_devices);

  if (!device_lists_complete_) {
    device_lists_complete_ = true;
    NotifyObserversDeviceListsComplete();
  }
}

void InputDeviceClient::OnStylusStateChanged(StylusState state) {
  for (auto& observer : observers_)
    observer.OnStylusStateChanged(state);
}

void InputDeviceClient::NotifyObserversDeviceListsComplete() {
  for (auto& observer : observers_)
    observer.OnDeviceListsComplete();
}

void InputDeviceClient::NotifyObserversKeyboardDeviceConfigurationChanged() {
  for (auto& observer : observers_)
    observer.OnKeyboardDeviceConfigurationChanged();
}

void InputDeviceClient::NotifyObserversTouchscreenDeviceConfigurationChanged() {
  for (auto& observer : observers_)
    observer.OnTouchscreenDeviceConfigurationChanged();
}

}  // namespace ui
