// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef DEVICE_HID_FAKE_INPUT_SERVICE_LINUX_H_
#define DEVICE_HID_FAKE_INPUT_SERVICE_LINUX_H_

#include <map>
#include <string>

#include "mojo/public/cpp/bindings/binding_set.h"
#include "mojo/public/cpp/bindings/interface_ptr_set.h"
#include "services/device/public/mojom/input_service.mojom.h"
#include "services/service_manager/public/cpp/service_context.h"

namespace device {

class FakeInputServiceLinux : public mojom::InputDeviceManager {
 public:
  using DeviceMap = std::map<std::string, mojom::InputDeviceInfoPtr>;

  FakeInputServiceLinux();
  ~FakeInputServiceLinux() override;

  // mojom::InputDeviceManager implementation:
  void GetDevicesAndSetClient(
      mojom::InputDeviceManagerClientAssociatedPtrInfo client,
      GetDevicesCallback callback) override;
  void GetDevices(GetDevicesCallback callback) override;

  void Bind(const std::string& interface_name,
            mojo::ScopedMessagePipeHandle handle,
            const service_manager::BindSourceInfo& source_info);
  void AddDevice(mojom::InputDeviceInfoPtr info);
  void RemoveDevice(const std::string& id);

  DeviceMap devices_;

 private:
  mojo::BindingSet<mojom::InputDeviceManager> bindings_;
  mojo::AssociatedInterfacePtrSet<mojom::InputDeviceManagerClient> clients_;

  DISALLOW_COPY_AND_ASSIGN(FakeInputServiceLinux);
};

}  // namespace device

#endif  // DEVICE_HID_FAKE_INPUT_SERVICE_LINUX_H_
