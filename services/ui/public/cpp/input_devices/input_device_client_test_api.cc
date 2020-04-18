// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "services/ui/public/cpp/input_devices/input_device_client_test_api.h"

#include "services/ui/public/cpp/input_devices/input_device_client.h"
#include "ui/events/devices/device_data_manager.h"
#include "ui/events/devices/input_device.h"
#include "ui/events/devices/touchscreen_device.h"

namespace ui {

InputDeviceClientTestApi::InputDeviceClientTestApi() = default;

InputDeviceClientTestApi::~InputDeviceClientTestApi() = default;

void InputDeviceClientTestApi::NotifyObserversDeviceListsComplete() {
  if (DeviceDataManager::instance_)
    DeviceDataManager::instance_->NotifyObserversDeviceListsComplete();
  else
    GetInputDeviceClient()->NotifyObserversDeviceListsComplete();
}

void InputDeviceClientTestApi::
    NotifyObserversKeyboardDeviceConfigurationChanged() {
  if (DeviceDataManager::instance_)
    DeviceDataManager::instance_
        ->NotifyObserversKeyboardDeviceConfigurationChanged();
  else
    GetInputDeviceClient()->NotifyObserversKeyboardDeviceConfigurationChanged();
}

void InputDeviceClientTestApi::NotifyObserversStylusStateChanged(
    StylusState stylus_state) {
  if (DeviceDataManager::instance_) {
    DeviceDataManager::instance_->NotifyObserversStylusStateChanged(
        stylus_state);
  } else {
    GetInputDeviceClient()->OnStylusStateChanged(stylus_state);
  }
}

void InputDeviceClientTestApi::
    NotifyObserversTouchscreenDeviceConfigurationChanged() {
  if (DeviceDataManager::instance_) {
    DeviceDataManager::instance_
        ->NotifyObserversTouchscreenDeviceConfigurationChanged();
  } else {
    GetInputDeviceClient()
        ->NotifyObserversTouchscreenDeviceConfigurationChanged();
  }
}

void InputDeviceClientTestApi::OnDeviceListsComplete() {
  if (DeviceDataManager::instance_)
    DeviceDataManager::instance_->OnDeviceListsComplete();
  else
    GetInputDeviceClient()->OnDeviceListsComplete({}, {}, {}, {}, false);
}

void InputDeviceClientTestApi::SetKeyboardDevices(
    const std::vector<InputDevice>& devices) {
  if (DeviceDataManager::instance_) {
    DeviceDataManager::instance_->OnKeyboardDevicesUpdated(devices);
  } else {
    GetInputDeviceClient()->OnKeyboardDeviceConfigurationChanged(devices);
  }
}

void InputDeviceClientTestApi::SetMouseDevices(
    const std::vector<InputDevice>& devices) {
  if (DeviceDataManager::instance_) {
    DeviceDataManager::instance_->OnMouseDevicesUpdated(devices);
  } else {
    GetInputDeviceClient()->OnMouseDeviceConfigurationChanged(devices);
  }
}

void InputDeviceClientTestApi::SetTouchscreenDevices(
    const std::vector<TouchscreenDevice>& devices,
    bool are_touchscreen_target_displays_valid) {
  if (DeviceDataManager::instance_) {
    DeviceDataManager::instance_->OnTouchscreenDevicesUpdated(devices);
  } else {
    GetInputDeviceClient()->OnTouchscreenDeviceConfigurationChanged(
        devices, are_touchscreen_target_displays_valid);
  }
}

InputDeviceClient* InputDeviceClientTestApi::GetInputDeviceClient() {
  if (DeviceDataManager::instance_ || !InputDeviceManager::HasInstance())
    return nullptr;
  return static_cast<InputDeviceClient*>(InputDeviceManager::GetInstance());
}

}  // namespace ui
