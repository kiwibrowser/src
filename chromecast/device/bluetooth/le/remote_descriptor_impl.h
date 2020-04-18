// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROMECAST_DEVICE_BLUETOOTH_LE_REMOTE_DESCRIPTOR_IMPL_H_
#define CHROMECAST_DEVICE_BLUETOOTH_LE_REMOTE_DESCRIPTOR_IMPL_H_

#include <map>
#include <memory>
#include <vector>

#include "base/memory/weak_ptr.h"
#include "base/single_thread_task_runner.h"
#include "chromecast/device/bluetooth/le/remote_descriptor.h"
#include "chromecast/public/bluetooth/gatt.h"

namespace chromecast {
namespace bluetooth {

class GattClientManagerImpl;

class RemoteDescriptorImpl : public RemoteDescriptor {
 public:
  // RemoteDescriptorImpl implementation:
  void ReadAuth(bluetooth_v2_shlib::Gatt::Client::AuthReq auth_req,
                ReadCallback callback) override;
  void Read(ReadCallback callback) override;
  void WriteAuth(bluetooth_v2_shlib::Gatt::Client::AuthReq auth_req,
                 const std::vector<uint8_t>& value,
                 StatusCallback callback) override;
  void Write(const std::vector<uint8_t>& value,
             StatusCallback callback) override;
  const bluetooth_v2_shlib::Gatt::Descriptor& descriptor() const override;
  const bluetooth_v2_shlib::Uuid uuid() const override;
  uint16_t handle() const override;
  bluetooth_v2_shlib::Gatt::Permissions permissions() const override;

 private:
  friend class GattClientManagerImpl;
  friend class RemoteCharacteristicImpl;
  friend class RemoteDeviceImpl;

  RemoteDescriptorImpl(
      RemoteDevice* device,
      base::WeakPtr<GattClientManagerImpl> gatt_client_manager,
      const bluetooth_v2_shlib::Gatt::Descriptor* characteristic,
      scoped_refptr<base::SingleThreadTaskRunner> io_task_runner);
  ~RemoteDescriptorImpl() override;

  void OnConnectChanged(bool connected);
  void OnReadComplete(bool status, const std::vector<uint8_t>& value);
  void OnWriteComplete(bool status);

  RemoteDevice* const device_;
  const base::WeakPtr<GattClientManagerImpl> gatt_client_manager_;
  const bluetooth_v2_shlib::Gatt::Descriptor* const descriptor_;

  // All bluetooth_v2_shlib calls are run on this task_runner. All members must
  // be accessed on this task_runner.
  const scoped_refptr<base::SingleThreadTaskRunner> io_task_runner_;

  ReadCallback read_callback_;
  StatusCallback write_callback_;

  bool pending_read_ = false;
  bool pending_write_ = false;

  DISALLOW_COPY_AND_ASSIGN(RemoteDescriptorImpl);
};

}  // namespace bluetooth
}  // namespace chromecast

#endif  // CHROMECAST_DEVICE_BLUETOOTH_LE_REMOTE_DESCRIPTOR_IMPL_H_
