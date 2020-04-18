// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SERVICES_UI_PUBLIC_CPP_INPUT_DEVICES_INPUT_DEVICE_CONTROLLER_H_
#define SERVICES_UI_PUBLIC_CPP_INPUT_DEVICES_INPUT_DEVICE_CONTROLLER_H_

#include "base/macros.h"
#include "mojo/public/cpp/bindings/binding_set.h"
#include "mojo/public/cpp/bindings/interface_ptr_set.h"
#include "services/service_manager/public/cpp/binder_registry.h"
#include "services/ui/public/interfaces/input_devices/input_device_controller.mojom.h"

namespace ui {

class InputController;

// Implementation of mojom::InputDeviceController that forwards to
// ui::InputController.
class InputDeviceController : public mojom::InputDeviceController {
 public:
  InputDeviceController();
  ~InputDeviceController() override;

  // Registers the interface provided by this class with |registry|.
  void AddInterface(
      service_manager::BinderRegistry* registry,
      const scoped_refptr<base::SequencedTaskRunner>& task_runner = nullptr);

  // mojom::InputDeviceController::
  void AddKeyboardDeviceObserver(
      mojom::KeyboardDeviceObserverPtr observer) override;
  void GetHasTouchpad(GetHasTouchpadCallback callback) override;
  void GetHasMouse(GetHasMouseCallback callback) override;
  void SetCapsLockEnabled(bool enabled) override;
  void SetNumLockEnabled(bool enabled) override;
  void SetAutoRepeatEnabled(bool enabled) override;
  void SetAutoRepeatRate(base::TimeDelta auto_repeat_delay,
                         base::TimeDelta auto_repeat_interval) override;
  void SetKeyboardLayoutByName(const std::string& name) override;
  void SetTouchpadSensitivity(int32_t value) override;
  void SetTapToClick(bool enabled) override;
  void SetThreeFingerClick(bool enabled) override;
  void SetTapDragging(bool enabled) override;
  void SetNaturalScroll(bool enabled) override;
  void SetMouseSensitivity(int32_t value) override;
  void SetPrimaryButtonRight(bool right) override;
  void SetMouseReverseScroll(bool enabled) override;
  void GetTouchDeviceStatus(GetTouchDeviceStatusCallback callback) override;
  void GetTouchEventLog(const base::FilePath& out_dir,
                        GetTouchEventLogCallback callback) override;
  void SetTapToClickPaused(bool state) override;
  void SetInternalTouchpadEnabled(
      bool enabled,
      SetInternalTouchpadEnabledCallback callback) override;
  void SetTouchscreensEnabled(bool enabled) override;
  void SetInternalKeyboardFilter(
      bool enable_filter,
      const std::vector<uint32_t>& allowed_keys) override;

 private:
  ui::InputController* GetInputController();

  // Notifies all KeyboardDeviceObservers.
  void NotifyObservers();

  // Notifies a single KeyboardDeviceObserver.
  void NotifyObserver(mojom::KeyboardDeviceObserver* observer);

  void BindInputDeviceControllerRequest(
      mojom::InputDeviceControllerRequest request);

  mojo::BindingSet<mojom::InputDeviceController> bindings_;
  mojo::InterfacePtrSet<mojom::KeyboardDeviceObserver> observers_;

  DISALLOW_COPY_AND_ASSIGN(InputDeviceController);
};

}  // namespace ui

#endif  // SERVICES_UI_PUBLIC_CPP_INPUT_DEVICES_INPUT_DEVICE_CONTROLLER_H_
