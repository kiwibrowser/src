// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "services/device/public/cpp/hid/fake_input_service_linux.h"

namespace device {

FakeInputServiceLinux::FakeInputServiceLinux() {}

FakeInputServiceLinux::~FakeInputServiceLinux() {}

// mojom::InputDeviceManager implementation:
void FakeInputServiceLinux::GetDevicesAndSetClient(
    mojom::InputDeviceManagerClientAssociatedPtrInfo client,
    GetDevicesCallback callback) {
  GetDevices(std::move(callback));

  if (!client.is_valid())
    return;

  mojom::InputDeviceManagerClientAssociatedPtr client_ptr;
  client_ptr.Bind(std::move(client));
  clients_.AddPtr(std::move(client_ptr));
}

void FakeInputServiceLinux::GetDevices(GetDevicesCallback callback) {
  std::vector<mojom::InputDeviceInfoPtr> devices;
  for (auto& device : devices_)
    devices.push_back(device.second->Clone());

  std::move(callback).Run(std::move(devices));
}

void FakeInputServiceLinux::Bind(
    const std::string& interface_name,
    mojo::ScopedMessagePipeHandle handle,
    const service_manager::BindSourceInfo& source_info) {
  bindings_.AddBinding(this,
                       mojom::InputDeviceManagerRequest(std::move(handle)));
}

void FakeInputServiceLinux::AddDevice(mojom::InputDeviceInfoPtr info) {
  auto* device_info = info.get();
  clients_.ForAllPtrs([device_info](mojom::InputDeviceManagerClient* client) {
    client->InputDeviceAdded(device_info->Clone());
  });

  devices_[info->id] = std::move(info);
}

void FakeInputServiceLinux::RemoveDevice(const std::string& id) {
  devices_.erase(id);

  clients_.ForAllPtrs([id](mojom::InputDeviceManagerClient* client) {
    client->InputDeviceRemoved(id);
  });
}

}  // namespace device
