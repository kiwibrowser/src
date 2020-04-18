// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "device/fido/fake_hid_impl_for_testing.h"

#include <utility>

#include "device/fido/fido_parsing_utils.h"

namespace device {

namespace {

MATCHER_P(IsCtapHidCommand, expected_command, "") {
  return arg.size() >= 5 &&
         arg[4] == (0x80 | static_cast<uint8_t>(expected_command));
}

}  // namespace

MockHidConnection::MockHidConnection(
    device::mojom::HidDeviceInfoPtr device,
    device::mojom::HidConnectionRequest request,
    std::vector<uint8_t> connection_channel_id)
    : binding_(this, std::move(request)),
      device_(std::move(device)),
      connection_channel_id_(connection_channel_id) {}

MockHidConnection::~MockHidConnection() {}

void MockHidConnection::Read(ReadCallback callback) {
  return ReadPtr(&callback);
}

void MockHidConnection::Write(uint8_t report_id,
                              const std::vector<uint8_t>& buffer,
                              WriteCallback callback) {
  return WritePtr(report_id, buffer, &callback);
}

void MockHidConnection::GetFeatureReport(uint8_t report_id,
                                         GetFeatureReportCallback callback) {
  NOTREACHED();
}

void MockHidConnection::SendFeatureReport(uint8_t report_id,
                                          const std::vector<uint8_t>& buffer,
                                          SendFeatureReportCallback callback) {
  NOTREACHED();
}

void MockHidConnection::SetNonce(base::span<uint8_t const> nonce) {
  nonce_ = std::vector<uint8_t>(nonce.begin(), nonce.end());
}

void MockHidConnection::ExpectWriteHidInit() {
  EXPECT_CALL(*this, WritePtr(::testing::_,
                              IsCtapHidCommand(FidoHidDeviceCommand::kInit),
                              ::testing::_))
      .WillOnce(::testing::Invoke(
          [&](auto&&, const std::vector<uint8_t>& buffer,
              device::mojom::HidConnection::WriteCallback* cb) {
            ASSERT_EQ(64u, buffer.size());
            // First 7 bytes are 4 bytes of channel id, one byte representing
            // HID command, 2 bytes for payload length.
            SetNonce(base::make_span(buffer).subspan(7, 8));
            std::move(*cb).Run(true);
          }));
}

void MockHidConnection::ExpectHidWriteWithCommand(FidoHidDeviceCommand cmd) {
  EXPECT_CALL(*this,
              WritePtr(::testing::_, IsCtapHidCommand(cmd), ::testing::_))
      .WillOnce(::testing::Invoke(
          [&](auto&&, const std::vector<uint8_t>& buffer,
              device::mojom::HidConnection::WriteCallback* cb) {
            std::move(*cb).Run(true);
          }));
}

bool FakeHidConnection::mock_connection_error_ = false;

FakeHidConnection::FakeHidConnection(device::mojom::HidDeviceInfoPtr device)
    : device_(std::move(device)) {}

FakeHidConnection::~FakeHidConnection() = default;

void FakeHidConnection::Read(ReadCallback callback) {
  std::vector<uint8_t> buffer = {'F', 'a', 'k', 'e', ' ', 'H', 'i', 'd'};
  std::move(callback).Run(true, 0, buffer);
}

void FakeHidConnection::Write(uint8_t report_id,
                              const std::vector<uint8_t>& buffer,
                              WriteCallback callback) {
  if (mock_connection_error_) {
    std::move(callback).Run(false);
    return;
  }

  std::move(callback).Run(true);
}

void FakeHidConnection::GetFeatureReport(uint8_t report_id,
                                         GetFeatureReportCallback callback) {
  NOTREACHED();
}

void FakeHidConnection::SendFeatureReport(uint8_t report_id,
                                          const std::vector<uint8_t>& buffer,
                                          SendFeatureReportCallback callback) {
  NOTREACHED();
}

FakeHidManager::FakeHidManager() = default;

FakeHidManager::~FakeHidManager() = default;

void FakeHidManager::AddBinding(mojo::ScopedMessagePipeHandle handle) {
  bindings_.AddBinding(this,
                       device::mojom::HidManagerRequest(std::move(handle)));
}

void FakeHidManager::AddBinding2(device::mojom::HidManagerRequest request) {
  bindings_.AddBinding(this, std::move(request));
}

void FakeHidManager::GetDevicesAndSetClient(
    device::mojom::HidManagerClientAssociatedPtrInfo client,
    GetDevicesCallback callback) {
  GetDevices(std::move(callback));

  device::mojom::HidManagerClientAssociatedPtr client_ptr;
  client_ptr.Bind(std::move(client));
  clients_.AddPtr(std::move(client_ptr));
}

void FakeHidManager::GetDevices(GetDevicesCallback callback) {
  std::vector<device::mojom::HidDeviceInfoPtr> device_list;
  for (auto& map_entry : devices_)
    device_list.push_back(map_entry.second->Clone());

  std::move(callback).Run(std::move(device_list));
}

void FakeHidManager::Connect(const std::string& device_guid,
                             ConnectCallback callback) {
  auto device_it = devices_.find(device_guid);
  auto connection_it = connections_.find(device_guid);
  if (device_it == devices_.end() || connection_it == connections_.end()) {
    std::move(callback).Run(nullptr);
    return;
  }

  std::move(callback).Run(std::move(connection_it->second));
}

void FakeHidManager::AddDevice(device::mojom::HidDeviceInfoPtr device) {
  device::mojom::HidDeviceInfo* device_info = device.get();
  clients_.ForAllPtrs([device_info](device::mojom::HidManagerClient* client) {
    client->DeviceAdded(device_info->Clone());
  });

  devices_[device->guid] = std::move(device);
}

void FakeHidManager::AddDeviceAndSetConnection(
    device::mojom::HidDeviceInfoPtr device,
    device::mojom::HidConnectionPtr connection) {
  connections_[device->guid] = std::move(connection);
  AddDevice(std::move(device));
}

void FakeHidManager::RemoveDevice(const std::string device_guid) {
  auto it = devices_.find(device_guid);
  if (it == devices_.end())
    return;

  device::mojom::HidDeviceInfo* device_info = it->second.get();
  clients_.ForAllPtrs([device_info](device::mojom::HidManagerClient* client) {
    client->DeviceRemoved(device_info->Clone());
  });
  devices_.erase(it);
}

}  // namespace device
