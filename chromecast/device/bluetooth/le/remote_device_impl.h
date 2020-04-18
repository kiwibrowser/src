// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROMECAST_DEVICE_BLUETOOTH_LE_REMOTE_DEVICE_IMPL_H_
#define CHROMECAST_DEVICE_BLUETOOTH_LE_REMOTE_DEVICE_IMPL_H_

#include <atomic>
#include <map>
#include <vector>

#include "base/memory/weak_ptr.h"
#include "base/single_thread_task_runner.h"
#include "base/threading/thread_checker.h"
#include "chromecast/device/bluetooth/le/remote_device.h"

namespace chromecast {
namespace bluetooth {

class GattClientManagerImpl;
class RemoteCharacteristic;
class RemoteDescriptor;

class RemoteDeviceImpl : public RemoteDevice {
 public:
  // RemoteDevice implementation
  void Connect(StatusCallback cb) override;
  bool ConnectSync() override;
  void Disconnect(StatusCallback cb) override;
  bool DisconnectSync() override;
  void ReadRemoteRssi(RssiCallback cb) override;
  void RequestMtu(int mtu, StatusCallback cb) override;
  void ConnectionParameterUpdate(int min_interval,
                                 int max_interval,
                                 int latency,
                                 int timeout,
                                 StatusCallback cb) override;
  bool IsConnected() override;
  int GetMtu() override;
  void GetServices(
      base::OnceCallback<void(std::vector<scoped_refptr<RemoteService>>)> cb)
      override;
  std::vector<scoped_refptr<RemoteService>> GetServicesSync() override;
  void GetServiceByUuid(
      const bluetooth_v2_shlib::Uuid& uuid,
      base::OnceCallback<void(scoped_refptr<RemoteService>)> cb) override;
  scoped_refptr<RemoteService> GetServiceByUuidSync(
      const bluetooth_v2_shlib::Uuid& uuid) override;
  const bluetooth_v2_shlib::Addr& addr() const override;

 private:
  friend class GattClientManagerImpl;

  RemoteDeviceImpl(const bluetooth_v2_shlib::Addr& addr,
                   base::WeakPtr<GattClientManagerImpl> gatt_client_manager,
                   scoped_refptr<base::SingleThreadTaskRunner> io_task_runner);
  ~RemoteDeviceImpl() override;

  // Friend methods for GattClientManagerImpl
  void SetConnected(bool connected);
  void SetServicesDiscovered(bool discovered);
  bool GetServicesDiscovered();
  void SetMtu(int mtu);

  scoped_refptr<RemoteCharacteristic> CharacteristicFromHandle(uint16_t handle);
  scoped_refptr<RemoteDescriptor> DescriptorFromHandle(uint16_t handle);

  void OnGetServices(
      const std::vector<bluetooth_v2_shlib::Gatt::Service>& services);
  void OnServicesRemoved(uint16_t start_handle, uint16_t end_handle);
  void OnServicesAdded(
      const std::vector<bluetooth_v2_shlib::Gatt::Service>& services);
  void OnReadRemoteRssiComplete(bool status, int rssi);
  // end Friend methods for GattClientManagerImpl

  const base::WeakPtr<GattClientManagerImpl> gatt_client_manager_;
  const bluetooth_v2_shlib::Addr addr_;

  // All bluetooth_v2_shlib calls are run on this task_runner. Below members
  // should only be accessed on this task_runner.
  const scoped_refptr<base::SingleThreadTaskRunner> io_task_runner_;

  bool services_discovered_ = false;

  bool connect_pending_ = false;
  StatusCallback connect_cb_;

  bool disconnect_pending_ = false;
  StatusCallback disconnect_cb_;

  bool rssi_pending_ = false;
  RssiCallback rssi_cb_;

  bool mtu_pending_ = false;
  StatusCallback mtu_cb_;

  std::atomic<bool> connected_{false};
  std::atomic<int> mtu_{kDefaultMtu};
  std::map<bluetooth_v2_shlib::Uuid, scoped_refptr<RemoteService>>
      uuid_to_service_;
  std::map<uint16_t, scoped_refptr<RemoteCharacteristic>>
      handle_to_characteristic_;
  std::map<uint16_t, scoped_refptr<RemoteDescriptor>> handle_to_descriptor_;
  DISALLOW_COPY_AND_ASSIGN(RemoteDeviceImpl);
};

}  // namespace bluetooth
}  // namespace chromecast

#endif  // CHROMECAST_DEVICE_BLUETOOTH_LE_REMOTE_DEVICE_IMPL_H_
