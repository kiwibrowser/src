// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chromecast/device/bluetooth/le/remote_device_impl.h"

#include "base/bind.h"
#include "chromecast/base/bind_to_task_runner.h"
#include "chromecast/device/bluetooth/le/gatt_client_manager_impl.h"
#include "chromecast/device/bluetooth/le/remote_characteristic_impl.h"
#include "chromecast/device/bluetooth/le/remote_descriptor_impl.h"
#include "chromecast/device/bluetooth/le/remote_service_impl.h"

namespace chromecast {
namespace bluetooth {

#define RUN_ON_IO_THREAD(method, ...) \
  io_task_runner_->PostTask(          \
      FROM_HERE,                      \
      base::BindOnce(&RemoteDeviceImpl::method, this, ##__VA_ARGS__));

#define MAKE_SURE_IO_THREAD(method, ...)            \
  DCHECK(io_task_runner_);                          \
  if (!io_task_runner_->BelongsToCurrentThread()) { \
    RUN_ON_IO_THREAD(method, ##__VA_ARGS__)         \
    return;                                         \
  }

#define EXEC_CB_AND_RET(cb, ret, ...)        \
  do {                                       \
    if (cb) {                                \
      std::move(cb).Run(ret, ##__VA_ARGS__); \
    }                                        \
    return;                                  \
  } while (0)

#define CHECK_CONNECTED(cb)                              \
  do {                                                   \
    if (!connected_) {                                   \
      LOG(ERROR) << __func__ << "failed: Not connected"; \
      EXEC_CB_AND_RET(cb, false);                        \
    }                                                    \
  } while (0)

#define LOG_EXEC_CB_AND_RET(cb, ret)      \
  do {                                    \
    if (!ret) {                           \
      LOG(ERROR) << __func__ << "failed"; \
    }                                     \
    EXEC_CB_AND_RET(cb, ret);             \
  } while (0)

RemoteDeviceImpl::RemoteDeviceImpl(
    const bluetooth_v2_shlib::Addr& addr,
    base::WeakPtr<GattClientManagerImpl> gatt_client_manager,
    scoped_refptr<base::SingleThreadTaskRunner> io_task_runner)
    : gatt_client_manager_(gatt_client_manager),
      addr_(addr),
      io_task_runner_(io_task_runner) {
  DCHECK(gatt_client_manager);
  DCHECK(io_task_runner_->BelongsToCurrentThread());
}

RemoteDeviceImpl::~RemoteDeviceImpl() = default;

void RemoteDeviceImpl::Connect(StatusCallback cb) {
  MAKE_SURE_IO_THREAD(Connect, BindToCurrentSequence(std::move(cb)));
  if (!ConnectSync()) {
    // Error logged.
    EXEC_CB_AND_RET(cb, false);
  }

  connect_cb_ = std::move(cb);
}

bool RemoteDeviceImpl::ConnectSync() {
  DCHECK(io_task_runner_->BelongsToCurrentThread());
  if (!gatt_client_manager_) {
    LOG(ERROR) << __func__ << " failed: Destroyed";
    return false;
  }
  if (connect_pending_) {
    LOG(ERROR) << __func__ << " failed: Connection pending";
    return false;
  }

  gatt_client_manager_->NotifyConnect(addr_);
  if (!gatt_client_manager_->gatt_client()->Connect(addr_)) {
    LOG(ERROR) << __func__ << " failed";
    return false;
  }
  connect_pending_ = true;
  return true;
}

void RemoteDeviceImpl::Disconnect(StatusCallback cb) {
  MAKE_SURE_IO_THREAD(Disconnect, BindToCurrentSequence(std::move(cb)));
  if (!DisconnectSync()) {
    // Error logged.
    EXEC_CB_AND_RET(cb, false);
  }

  disconnect_cb_ = std::move(cb);
}

bool RemoteDeviceImpl::DisconnectSync() {
  DCHECK(io_task_runner_->BelongsToCurrentThread());
  if (!gatt_client_manager_) {
    LOG(ERROR) << __func__ << " failed: Destroyed";
    return false;
  }

  if (!connected_) {
    LOG(ERROR) << "Not connected";
    return false;
  }

  if (!gatt_client_manager_->gatt_client()->Disconnect(addr_)) {
    LOG(ERROR) << __func__ << " failed";
    return false;
  }
  disconnect_pending_ = true;

  return true;
}

void RemoteDeviceImpl::ReadRemoteRssi(RssiCallback cb) {
  MAKE_SURE_IO_THREAD(ReadRemoteRssi, BindToCurrentSequence(std::move(cb)));
  if (!gatt_client_manager_) {
    LOG(ERROR) << __func__ << " failed: Destroyed";
    EXEC_CB_AND_RET(cb, false, 0);
  }

  if (rssi_pending_) {
    LOG(ERROR) << "Read remote RSSI already pending";
    EXEC_CB_AND_RET(cb, false, 0);
  }
  if (!gatt_client_manager_->gatt_client()->ReadRemoteRssi(addr_)) {
    LOG(ERROR) << __func__ << " failed";
    EXEC_CB_AND_RET(cb, false, 0);
  }
  rssi_pending_ = true;
  rssi_cb_ = std::move(cb);
}

void RemoteDeviceImpl::RequestMtu(int mtu, StatusCallback cb) {
  MAKE_SURE_IO_THREAD(RequestMtu, mtu, BindToCurrentSequence(std::move(cb)));
  if (!gatt_client_manager_) {
    LOG(ERROR) << __func__ << " failed: Destroyed";
    EXEC_CB_AND_RET(cb, false);
  }
  CHECK_CONNECTED(cb);
  if (mtu_pending_) {
    LOG(ERROR) << "MTU change already pending";
    EXEC_CB_AND_RET(cb, false);
  }

  if (!gatt_client_manager_->gatt_client()->RequestMtu(addr_, mtu)) {
    LOG(ERROR) << __func__ << " failed";
    EXEC_CB_AND_RET(cb, false);
  }

  mtu_pending_ = true;
  mtu_cb_ = std::move(cb);
}

void RemoteDeviceImpl::ConnectionParameterUpdate(int min_interval,
                                                 int max_interval,
                                                 int latency,
                                                 int timeout,
                                                 StatusCallback cb) {
  MAKE_SURE_IO_THREAD(ConnectionParameterUpdate, min_interval, max_interval,
                      latency, timeout, BindToCurrentSequence(std::move(cb)));
  if (!gatt_client_manager_) {
    LOG(ERROR) << __func__ << " failed: Destroyed";
    EXEC_CB_AND_RET(cb, false);
  }
  CHECK_CONNECTED(cb);
  bool ret = gatt_client_manager_->gatt_client()->ConnectionParameterUpdate(
      addr_, min_interval, max_interval, latency, timeout);
  LOG_EXEC_CB_AND_RET(cb, ret);
}

bool RemoteDeviceImpl::IsConnected() {
  return connected_;
}

int RemoteDeviceImpl::GetMtu() {
  return mtu_;
}

void RemoteDeviceImpl::GetServices(
    base::OnceCallback<void(std::vector<scoped_refptr<RemoteService>>)> cb) {
  MAKE_SURE_IO_THREAD(GetServices, BindToCurrentSequence(std::move(cb)));
  auto ret = GetServicesSync();
  EXEC_CB_AND_RET(cb, std::move(ret));
}

std::vector<scoped_refptr<RemoteService>> RemoteDeviceImpl::GetServicesSync() {
  DCHECK(io_task_runner_->BelongsToCurrentThread());
  std::vector<scoped_refptr<RemoteService>> services;
  services.reserve(uuid_to_service_.size());
  for (const auto& pair : uuid_to_service_)
    services.push_back(pair.second);

  return services;
}

void RemoteDeviceImpl::GetServiceByUuid(
    const bluetooth_v2_shlib::Uuid& uuid,
    base::OnceCallback<void(scoped_refptr<RemoteService>)> cb) {
  MAKE_SURE_IO_THREAD(GetServiceByUuid, uuid,
                      BindToCurrentSequence(std::move(cb)));
  auto ret = GetServiceByUuidSync(uuid);
  EXEC_CB_AND_RET(cb, std::move(ret));
}

const bluetooth_v2_shlib::Addr& RemoteDeviceImpl::addr() const {
  return addr_;
}

scoped_refptr<RemoteService> RemoteDeviceImpl::GetServiceByUuidSync(
    const bluetooth_v2_shlib::Uuid& uuid) {
  DCHECK(io_task_runner_->BelongsToCurrentThread());
  auto it = uuid_to_service_.find(uuid);
  if (it == uuid_to_service_.end())
    return nullptr;

  return it->second;
}

void RemoteDeviceImpl::SetConnected(bool connected) {
  DCHECK(io_task_runner_->BelongsToCurrentThread());
  // We only set connected = true after services are discovered.
  if (!connected) {
    connected_ = false;
  }
  if (connect_pending_) {
    connect_pending_ = false;
    if (connect_cb_) {
      std::move(connect_cb_).Run(connected);
    }
  }

  if (disconnect_pending_) {
    disconnect_pending_ = false;
    if (disconnect_cb_) {
      std::move(disconnect_cb_).Run(!connected);
    }
  }

  if (!connected && rssi_pending_) {
    LOG(ERROR) << "Read remote RSSI failed: disconnected";
    if (rssi_cb_) {
      std::move(rssi_cb_).Run(false, 0);
    }
    rssi_pending_ = false;
  }

  if (!connected && mtu_pending_) {
    LOG(ERROR) << "Set MTU failed: disconnected";
    if (mtu_cb_) {
      std::move(mtu_cb_).Run(false);
    }

    mtu_pending_ = false;
  }

  for (const auto& characteristic : handle_to_characteristic_) {
    auto* char_impl =
        static_cast<RemoteCharacteristicImpl*>(characteristic.second.get());
    char_impl->OnConnectChanged(connected);
  }

  for (const auto& descriptor : handle_to_descriptor_) {
    auto* desc_impl =
        static_cast<RemoteDescriptorImpl*>(descriptor.second.get());
    desc_impl->OnConnectChanged(connected);
  }

  if (connected) {
    if (!gatt_client_manager_) {
      LOG(ERROR) << "Couldn't discover services: Destroyed";
      return;
    }

    if (!gatt_client_manager_->gatt_client()->GetServices(addr_)) {
      LOG(ERROR) << "Couldn't discover services, disconnecting";
      Disconnect({});
    }
  } else {
    uuid_to_service_.clear();
    handle_to_characteristic_.clear();
    handle_to_descriptor_.clear();
  }
}

void RemoteDeviceImpl::SetServicesDiscovered(bool discovered) {
  DCHECK(io_task_runner_->BelongsToCurrentThread());
  services_discovered_ = discovered;
  if (!discovered) {
    return;
  }
  connected_ = true;
  if (connect_cb_) {
    std::move(connect_cb_).Run(true);
  }
}

bool RemoteDeviceImpl::GetServicesDiscovered() {
  DCHECK(io_task_runner_->BelongsToCurrentThread());
  return services_discovered_;
}

void RemoteDeviceImpl::SetMtu(int mtu) {
  DCHECK(io_task_runner_->BelongsToCurrentThread());
  mtu_pending_ = false;
  mtu_ = mtu;

  if (mtu_cb_) {
    std::move(mtu_cb_).Run(true);
  }
}

scoped_refptr<RemoteCharacteristic> RemoteDeviceImpl::CharacteristicFromHandle(
    uint16_t handle) {
  DCHECK(io_task_runner_->BelongsToCurrentThread());
  auto it = handle_to_characteristic_.find(handle);
  if (it == handle_to_characteristic_.end())
    return nullptr;

  return it->second;
}

scoped_refptr<RemoteDescriptor> RemoteDeviceImpl::DescriptorFromHandle(
    uint16_t handle) {
  DCHECK(io_task_runner_->BelongsToCurrentThread());
  auto it = handle_to_descriptor_.find(handle);
  if (it == handle_to_descriptor_.end())
    return nullptr;

  return it->second;
}

void RemoteDeviceImpl::OnGetServices(
    const std::vector<bluetooth_v2_shlib::Gatt::Service>& services) {
  DCHECK(io_task_runner_->BelongsToCurrentThread());
  uuid_to_service_.clear();
  handle_to_characteristic_.clear();
  handle_to_descriptor_.clear();
  OnServicesAdded(services);
}

void RemoteDeviceImpl::OnServicesRemoved(uint16_t start_handle,
                                         uint16_t end_handle) {
  DCHECK(io_task_runner_->BelongsToCurrentThread());
  for (auto it = uuid_to_service_.begin(); it != uuid_to_service_.end();) {
    if (it->second->handle() >= start_handle &&
        it->second->handle() <= end_handle) {
      for (auto& characteristic : it->second->GetCharacteristics()) {
        for (auto& descriptor : characteristic->GetDescriptors())
          handle_to_descriptor_.erase(descriptor->handle());

        handle_to_characteristic_.erase(characteristic->handle());
      }
      it = uuid_to_service_.erase(it);
    } else {
      ++it;
    }
  }
}

void RemoteDeviceImpl::OnServicesAdded(
    const std::vector<bluetooth_v2_shlib::Gatt::Service>& services) {
  DCHECK(io_task_runner_->BelongsToCurrentThread());
  for (const auto& service : services) {
    uuid_to_service_[service.uuid] = new RemoteServiceImpl(
        this, gatt_client_manager_, service, io_task_runner_);
  }

  for (const auto& pair : uuid_to_service_) {
    for (auto& characteristic : pair.second->GetCharacteristics()) {
      handle_to_characteristic_.emplace(characteristic->handle(),
                                        characteristic);
      for (auto& descriptor : characteristic->GetDescriptors()) {
        handle_to_descriptor_.emplace(descriptor->handle(), descriptor);
      }
    }
  }
}

void RemoteDeviceImpl::OnReadRemoteRssiComplete(bool status, int rssi) {
  DCHECK(io_task_runner_->BelongsToCurrentThread());
  rssi_pending_ = false;
  if (rssi_cb_) {
    std::move(rssi_cb_).Run(true, rssi);
  }
}

}  // namespace bluetooth
}  // namespace chromecast
