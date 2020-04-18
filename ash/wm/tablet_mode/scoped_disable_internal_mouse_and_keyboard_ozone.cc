// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ash/wm/tablet_mode/scoped_disable_internal_mouse_and_keyboard_ozone.h"

#include "ash/shell.h"
#include "ash/shell_delegate.h"
#include "ash/touch/touch_devices_controller.h"
#include "services/ui/public/cpp/input_devices/input_device_controller_client.h"
#include "ui/events/keycodes/dom/dom_code.h"

namespace ash {

namespace {

ui::InputDeviceControllerClient* GetInputDeviceControllerClient() {
  return Shell::Get()->shell_delegate()->GetInputDeviceControllerClient();
}

}  // namespace

class ScopedDisableInternalMouseAndKeyboardOzone::Disabler {
 public:
  Disabler() {
    Shell::Get()->touch_devices_controller()->SetTouchpadEnabled(
        false, TouchDeviceEnabledSource::GLOBAL);

    // Allow the acccessible keys present on the side of some devices to
    // continue working.
    std::vector<ui::DomCode> allowed_keys;
    allowed_keys.push_back(ui::DomCode::VOLUME_DOWN);
    allowed_keys.push_back(ui::DomCode::VOLUME_UP);
    allowed_keys.push_back(ui::DomCode::POWER);
    const bool enable_filter = true;
    GetInputDeviceControllerClient()->SetInternalKeyboardFilter(enable_filter,
                                                                allowed_keys);
  }

  ~Disabler() {
    Shell::Get()->touch_devices_controller()->SetTouchpadEnabled(
        true, TouchDeviceEnabledSource::GLOBAL);

    std::vector<ui::DomCode> allowed_keys;
    const bool enable_filter = false;
    GetInputDeviceControllerClient()->SetInternalKeyboardFilter(enable_filter,
                                                                allowed_keys);
  }

 private:
  DISALLOW_COPY_AND_ASSIGN(Disabler);
};

ScopedDisableInternalMouseAndKeyboardOzone::
    ScopedDisableInternalMouseAndKeyboardOzone()
    : disabler_(nullptr) {
  // InputDeviceControllerClient may be null in tests.
  if (GetInputDeviceControllerClient())
    disabler_ = std::make_unique<Disabler>();
}

ScopedDisableInternalMouseAndKeyboardOzone::
    ~ScopedDisableInternalMouseAndKeyboardOzone() {
}

}  // namespace ash
