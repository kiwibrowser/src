// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chromecast/device/bluetooth/le/gatt_client_manager_impl.h"

#include <vector>

#include "base/bind.h"
#include "base/memory/ptr_util.h"
#include "base/run_loop.h"
#include "chromecast/base/bind_to_task_runner.h"
#include "chromecast/device/bluetooth/le/remote_characteristic_impl.h"
#include "chromecast/device/bluetooth/le/remote_descriptor_impl.h"
#include "chromecast/device/bluetooth/le/remote_device_impl.h"
#include "chromecast/device/bluetooth/le/remote_service_impl.h"

namespace chromecast {
namespace bluetooth {

namespace {

#define RUN_ON_IO_THREAD(method, ...)                                       \
  io_task_runner_->PostTask(                                                \
      FROM_HERE, base::BindOnce(&GattClientManagerImpl::method, weak_this_, \
                                ##__VA_ARGS__));

#define MAKE_SURE_IO_THREAD(method, ...)            \
  DCHECK(io_task_runner_);                          \
  if (!io_task_runner_->BelongsToCurrentThread()) { \
    RUN_ON_IO_THREAD(method, ##__VA_ARGS__)         \
    return;                                         \
  }

#define CHECK_DEVICE_EXISTS_IT(it)                  \
  do {                                              \
    if (it == addr_to_device_.end()) {              \
      LOG(ERROR) << __func__ << ": No such device"; \
      return;                                       \
    }                                               \
  } while (0)

}  // namespace

GattClientManagerImpl::GattClientManagerImpl(
    bluetooth_v2_shlib::GattClient* gatt_client)
    : gatt_client_(gatt_client),
      observers_(new base::ObserverListThreadSafe<Observer>()),
      weak_factory_(
          std::make_unique<base::WeakPtrFactory<GattClientManagerImpl>>(this)) {
  weak_this_ = weak_factory_->GetWeakPtr();
}

GattClientManagerImpl::~GattClientManagerImpl() {}

void GattClientManagerImpl::Initialize(
    scoped_refptr<base::SingleThreadTaskRunner> io_task_runner) {
  io_task_runner_ = std::move(io_task_runner);
}

void GattClientManagerImpl::Finalize() {
  io_task_runner_->PostTask(
      FROM_HERE, base::BindOnce(&GattClientManagerImpl::FinalizeOnIoThread,
                                std::move(weak_factory_)));
}

void GattClientManagerImpl::AddObserver(Observer* o) {
  observers_->AddObserver(o);
}

void GattClientManagerImpl::RemoveObserver(Observer* o) {
  observers_->RemoveObserver(o);
}

void GattClientManagerImpl::GetDevice(
    const bluetooth_v2_shlib::Addr& addr,
    base::OnceCallback<void(scoped_refptr<RemoteDevice>)> cb) {
  MAKE_SURE_IO_THREAD(GetDevice, addr, BindToCurrentSequence(std::move(cb)));
  DCHECK(cb);
  std::move(cb).Run(GetDeviceSync(addr));
}

scoped_refptr<RemoteDevice> GattClientManagerImpl::GetDeviceSync(
    const bluetooth_v2_shlib::Addr& addr) {
  DCHECK(io_task_runner_->BelongsToCurrentThread());
  auto it = addr_to_device_.find(addr);
  if (it != addr_to_device_.end()) {
    return it->second.get();
  }

  scoped_refptr<RemoteDeviceImpl> new_device(
      new RemoteDeviceImpl(addr, weak_this_, io_task_runner_));
  addr_to_device_[addr] = new_device;
  return new_device;
}

size_t GattClientManagerImpl::GetNumConnected() const {
  DCHECK(io_task_runner_->BelongsToCurrentThread());
  return connected_devices_.size();
}

void GattClientManagerImpl::NotifyConnect(
    const bluetooth_v2_shlib::Addr& addr) {
  observers_->Notify(FROM_HERE, &Observer::OnConnectInitated, addr);
}

scoped_refptr<base::SingleThreadTaskRunner>
GattClientManagerImpl::task_runner() {
  return io_task_runner_;
}

void GattClientManagerImpl::OnConnectChanged(
    const bluetooth_v2_shlib::Addr& addr,
    bool status,
    bool connected) {
  MAKE_SURE_IO_THREAD(OnConnectChanged, addr, status, connected);
  auto it = addr_to_device_.find(addr);
  CHECK_DEVICE_EXISTS_IT(it);

  it->second->SetConnected(connected);
  if (connected) {
    connected_devices_.insert(addr);
  } else {
    connected_devices_.erase(addr);
  }

  // We won't declare the device connected until service discovery completes.
  // Only report disconnect callback if the connect callback was called (
  // service discovery completed).
  if (!connected && it->second->GetServicesDiscovered()) {
    it->second->SetServicesDiscovered(false);
    observers_->Notify(FROM_HERE, &Observer::OnConnectChanged, it->second,
                       false);
  }
}

void GattClientManagerImpl::OnNotification(const bluetooth_v2_shlib::Addr& addr,
                                           uint16_t handle,
                                           const std::vector<uint8_t>& value) {
  MAKE_SURE_IO_THREAD(OnNotification, addr, handle, value);
  auto it = addr_to_device_.find(addr);
  CHECK_DEVICE_EXISTS_IT(it);
  auto characteristic = it->second->CharacteristicFromHandle(handle);
  if (!characteristic) {
    LOG(ERROR) << "No such characteristic";
    return;
  }

  observers_->Notify(FROM_HERE, &Observer::OnCharacteristicNotification,
                     it->second, characteristic, value);
}

void GattClientManagerImpl::OnCharacteristicReadResponse(
    const bluetooth_v2_shlib::Addr& addr,
    bool status,
    uint16_t handle,
    const std::vector<uint8_t>& value) {
  MAKE_SURE_IO_THREAD(OnCharacteristicReadResponse, addr, status, handle,
                      value);
  auto it = addr_to_device_.find(addr);
  CHECK_DEVICE_EXISTS_IT(it);
  auto characteristic = it->second->CharacteristicFromHandle(handle);
  if (!characteristic) {
    LOG(ERROR) << "No such characteristic";
    return;
  }

  auto* char_impl =
      static_cast<RemoteCharacteristicImpl*>(characteristic.get());
  char_impl->OnReadComplete(status, value);
}

void GattClientManagerImpl::OnCharacteristicWriteResponse(
    const bluetooth_v2_shlib::Addr& addr,
    bool status,
    uint16_t handle) {
  MAKE_SURE_IO_THREAD(OnCharacteristicWriteResponse, addr, status, handle);
  auto it = addr_to_device_.find(addr);
  CHECK_DEVICE_EXISTS_IT(it);
  auto characteristic = it->second->CharacteristicFromHandle(handle);
  if (!characteristic) {
    LOG(ERROR) << "No such characteristic";
    return;
  }

  auto* char_impl =
      static_cast<RemoteCharacteristicImpl*>(characteristic.get());
  char_impl->OnWriteComplete(status);
}

void GattClientManagerImpl::OnDescriptorReadResponse(
    const bluetooth_v2_shlib::Addr& addr,
    bool status,
    uint16_t handle,
    const std::vector<uint8_t>& value) {
  MAKE_SURE_IO_THREAD(OnDescriptorReadResponse, addr, status, handle, value);
  auto it = addr_to_device_.find(addr);
  CHECK_DEVICE_EXISTS_IT(it);
  auto descriptor = it->second->DescriptorFromHandle(handle);
  if (!descriptor) {
    LOG(ERROR) << "No such descriptor";
    return;
  }

  auto* desc_impl = static_cast<RemoteDescriptorImpl*>(descriptor.get());
  desc_impl->OnReadComplete(status, value);
}

void GattClientManagerImpl::OnDescriptorWriteResponse(
    const bluetooth_v2_shlib::Addr& addr,
    bool status,
    uint16_t handle) {
  MAKE_SURE_IO_THREAD(OnDescriptorWriteResponse, addr, status, handle);
  auto it = addr_to_device_.find(addr);
  CHECK_DEVICE_EXISTS_IT(it);
  auto descriptor = it->second->DescriptorFromHandle(handle);
  if (!descriptor) {
    LOG(ERROR) << "No such descriptor";
    return;
  }

  auto* desc_impl = static_cast<RemoteDescriptorImpl*>(descriptor.get());
  desc_impl->OnWriteComplete(status);
}

void GattClientManagerImpl::OnReadRemoteRssi(
    const bluetooth_v2_shlib::Addr& addr,
    bool status,
    int rssi) {
  MAKE_SURE_IO_THREAD(OnReadRemoteRssi, addr, status, rssi);
  auto it = addr_to_device_.find(addr);
  CHECK_DEVICE_EXISTS_IT(it);
  it->second->OnReadRemoteRssiComplete(status, rssi);
}

void GattClientManagerImpl::OnMtuChanged(const bluetooth_v2_shlib::Addr& addr,
                                         bool status,
                                         int mtu) {
  MAKE_SURE_IO_THREAD(OnMtuChanged, addr, status, mtu);
  auto it = addr_to_device_.find(addr);
  CHECK_DEVICE_EXISTS_IT(it);
  it->second->SetMtu(mtu);

  observers_->Notify(FROM_HERE, &Observer::OnMtuChanged, it->second, mtu);
}

void GattClientManagerImpl::OnGetServices(
    const bluetooth_v2_shlib::Addr& addr,
    const std::vector<bluetooth_v2_shlib::Gatt::Service>& services) {
  MAKE_SURE_IO_THREAD(OnGetServices, addr, services);
  auto it = addr_to_device_.find(addr);
  CHECK_DEVICE_EXISTS_IT(it);
  it->second->OnGetServices(services);

  if (!it->second->GetServicesDiscovered()) {
    it->second->SetServicesDiscovered(true);
    observers_->Notify(FROM_HERE, &Observer::OnConnectChanged, it->second,
                       true);
  }

  observers_->Notify(FROM_HERE, &Observer::OnServicesUpdated, it->second,
                     it->second->GetServicesSync());
}

void GattClientManagerImpl::OnServicesRemoved(
    const bluetooth_v2_shlib::Addr& addr,
    uint16_t start_handle,
    uint16_t end_handle) {
  MAKE_SURE_IO_THREAD(OnServicesRemoved, addr, start_handle, end_handle);
  auto it = addr_to_device_.find(addr);
  CHECK_DEVICE_EXISTS_IT(it);
  it->second->OnServicesRemoved(start_handle, end_handle);

  observers_->Notify(FROM_HERE, &Observer::OnServicesUpdated, it->second,
                     it->second->GetServicesSync());
}

void GattClientManagerImpl::OnServicesAdded(
    const bluetooth_v2_shlib::Addr& addr,
    const std::vector<bluetooth_v2_shlib::Gatt::Service>& services) {
  MAKE_SURE_IO_THREAD(OnServicesAdded, addr, services);
  auto it = addr_to_device_.find(addr);
  CHECK_DEVICE_EXISTS_IT(it);
  it->second->OnServicesAdded(services);
  observers_->Notify(FROM_HERE, &Observer::OnServicesUpdated, it->second,
                     it->second->GetServicesSync());
}

// static
void GattClientManagerImpl::FinalizeOnIoThread(
    std::unique_ptr<base::WeakPtrFactory<GattClientManagerImpl>> weak_factory) {
  weak_factory->InvalidateWeakPtrs();
}

}  // namespace bluetooth
}  // namespace chromecast
