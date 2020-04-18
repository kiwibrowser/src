// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SERVICES_UI_PUBLIC_CPP_INPUT_DEVICES_INPUT_DEVICE_CONTROLLER_CLIENT_H_
#define SERVICES_UI_PUBLIC_CPP_INPUT_DEVICES_INPUT_DEVICE_CONTROLLER_CLIENT_H_

#include <string>

#include "base/callback_forward.h"
#include "base/macros.h"
#include "mojo/public/cpp/bindings/binding.h"
#include "services/ui/public/interfaces/input_devices/input_device_controller.mojom.h"

namespace base {
class FilePath;
}

namespace service_manager {
class Connector;
}

namespace ui {

enum class DomCode;

// InputDeviceControllerClient is mostly a call through to
// mojom::InputDeviceController. It does a minimal amount of caching and is
// itself a KeyboardDeviceObserver to maintain local keyboard state.
class InputDeviceControllerClient : public mojom::KeyboardDeviceObserver {
 public:
  // |service_Name| is the name of the service providing mojom::KeyboardDevice,
  // generally use the default, unless a specific service is needed.
  explicit InputDeviceControllerClient(
      service_manager::Connector* connector,
      const std::string& service_name = std::string());
  ~InputDeviceControllerClient() override;

  using GetHasMouseCallback = base::OnceCallback<void(bool)>;
  void GetHasMouse(GetHasMouseCallback callback);

  using GetHasTouchpadCallback = base::OnceCallback<void(bool)>;
  void GetHasTouchpad(GetHasTouchpadCallback callback);

  bool IsCapsLockEnabled();
  void SetCapsLockEnabled(bool enabled);
  void SetNumLockEnabled(bool enabled);
  bool IsAutoRepeatEnabled();
  void SetAutoRepeatEnabled(bool enabled);
  void SetAutoRepeatRate(base::TimeDelta delay, base::TimeDelta interval);
  void SetKeyboardLayoutByName(const std::string& layout_name);
  void SetTouchpadSensitivity(int value);
  void SetTapToClick(bool enabled);
  void SetThreeFingerClick(bool enabled);
  void SetTapDragging(bool enabled);
  void SetNaturalScroll(bool enabled);
  void SetMouseSensitivity(int value);
  void SetPrimaryButtonRight(bool right);
  void SetMouseReverseScroll(bool enabled);

  using GetTouchDeviceStatusCallback =
      base::OnceCallback<void(const std::string&)>;
  void GetTouchDeviceStatus(GetTouchDeviceStatusCallback callback);

  using GetTouchEventLogCallback =
      base::OnceCallback<void(const std::vector<base::FilePath>&)>;
  void GetTouchEventLog(const base::FilePath& out_dir,
                        GetTouchEventLogCallback callback);
  void SetTapToClickPaused(bool state);

  void SetTouchscreensEnabled(bool enabled);
  void SetInternalKeyboardFilter(bool enable_filter,
                                 const std::vector<DomCode>& allowed_keys);

  // Sets whether the internal touch pad. Returns true if there is an internal
  // touchpad.
  using SetInternalTouchpadEnabledCallback = base::OnceCallback<void(bool)>;
  void SetInternalTouchpadEnabled(bool enable,
                                  SetInternalTouchpadEnabledCallback callback);

 private:
  // mojom::KeyboardDeviceObserver:
  void OnKeyboardStateChanged(mojom::KeyboardDeviceStatePtr state) override;

  mojom::InputDeviceControllerPtr input_device_controller_;
  mojom::KeyboardDeviceState keyboard_device_state_;
  mojo::Binding<mojom::KeyboardDeviceObserver> binding_;

  DISALLOW_COPY_AND_ASSIGN(InputDeviceControllerClient);
};

}  // namespace ui

#endif  // SERVICES_UI_PUBLIC_CPP_INPUT_DEVICES_INPUT_DEVICE_CONTROLLER_CLIENT_H_
