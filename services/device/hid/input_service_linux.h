// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SERVICES_DEVICE_HID_INPUT_SERVICE_LINUX_H_
#define SERVICES_DEVICE_HID_INPUT_SERVICE_LINUX_H_

#include <map>
#include <memory>
#include <string>
#include <vector>

#include "base/compiler_specific.h"
#include "base/macros.h"
#include "base/threading/thread_checker.h"
#include "mojo/public/cpp/bindings/binding_set.h"
#include "mojo/public/cpp/bindings/interface_ptr_set.h"
#include "services/device/public/mojom/input_service.mojom.h"

namespace device {

// This class provides information and notifications about
// connected/disconnected input/HID devices. This class is *NOT*
// thread-safe and all methods must be called from the FILE thread.
class InputServiceLinux : public mojom::InputDeviceManager {
 public:
  using DeviceMap = std::map<std::string, mojom::InputDeviceInfoPtr>;

  InputServiceLinux();
  ~InputServiceLinux() override;

  // Binds the |request| to an InputServiceLinux instance.
  static void BindRequest(mojom::InputDeviceManagerRequest request);

  // Returns the InputServiceLinux instance for the current process. Creates one
  // if none has been set.
  static InputServiceLinux* GetInstance();

  // Returns true if an InputServiceLinux instance has been set for the current
  // process. An instance is set on the first call to GetInstance() or
  // SetForTesting().
  static bool HasInstance();

  // Sets the InputServiceLinux instance for the current process. Cannot be
  // called if GetInstance() or SetForTesting() has already been called in the
  // current process. |service| will never be deleted.
  static void SetForTesting(std::unique_ptr<InputServiceLinux> service);

  void AddBinding(mojom::InputDeviceManagerRequest request);

  // mojom::InputDeviceManager implementation:
  void GetDevicesAndSetClient(
      mojom::InputDeviceManagerClientAssociatedPtrInfo client,
      GetDevicesCallback callback) override;
  void GetDevices(GetDevicesCallback callback) override;

 protected:
  void AddDevice(mojom::InputDeviceInfoPtr info);
  void RemoveDevice(const std::string& id);

  bool CalledOnValidThread() const;

  DeviceMap devices_;

 private:
  base::ThreadChecker thread_checker_;
  mojo::BindingSet<mojom::InputDeviceManager> bindings_;
  mojo::AssociatedInterfacePtrSet<mojom::InputDeviceManagerClient> clients_;

  DISALLOW_COPY_AND_ASSIGN(InputServiceLinux);
};

}  // namespace device

#endif  // SERVICES_DEVICE_HID_INPUT_SERVICE_LINUX_H_
