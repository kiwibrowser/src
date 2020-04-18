// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chromecast/device/bluetooth/le/remote_characteristic_impl.h"

#include "chromecast/base/bind_to_task_runner.h"
#include "chromecast/device/bluetooth/le/gatt_client_manager_impl.h"
#include "chromecast/device/bluetooth/le/remote_descriptor_impl.h"
#include "chromecast/device/bluetooth/le/remote_device.h"

#define EXEC_CB_AND_RET(cb, ret, ...)        \
  do {                                       \
    if (cb) {                                \
      std::move(cb).Run(ret, ##__VA_ARGS__); \
    }                                        \
    return;                                  \
  } while (0)

#define RUN_ON_IO_THREAD(method, ...) \
  io_task_runner_->PostTask(          \
      FROM_HERE,                      \
      base::BindOnce(&RemoteCharacteristicImpl::method, this, ##__VA_ARGS__));

#define MAKE_SURE_IO_THREAD(method, ...)            \
  DCHECK(io_task_runner_);                          \
  if (!io_task_runner_->BelongsToCurrentThread()) { \
    RUN_ON_IO_THREAD(method, ##__VA_ARGS__)         \
    return;                                         \
  }

namespace chromecast {
namespace bluetooth {

namespace {
std::vector<uint8_t> GetDescriptorNotificationValue(bool enable) {
  if (enable) {
    return std::vector<uint8_t>(
        std::begin(bluetooth::RemoteDescriptor::kEnableNotificationValue),
        std::end(bluetooth::RemoteDescriptor::kEnableNotificationValue));
  }
  return std::vector<uint8_t>(
      std::begin(bluetooth::RemoteDescriptor::kDisableNotificationValue),
      std::end(bluetooth::RemoteDescriptor::kDisableNotificationValue));
}

bool CharacteristicHasNotify(
    const bluetooth_v2_shlib::Gatt::Characteristic* characteristic) {
  return characteristic->properties &
             bluetooth_v2_shlib::Gatt::PROPERTY_NOTIFY ||
         characteristic->properties &
             bluetooth_v2_shlib::Gatt::PROPERTY_INDICATE;
}

std::unique_ptr<bluetooth_v2_shlib::Gatt::Descriptor> MaybeCreateFakeCccd(
    const bluetooth_v2_shlib::Gatt::Characteristic* characteristic) {
  if (!CharacteristicHasNotify(characteristic)) {
    return nullptr;
  }

  for (const auto& descriptor : characteristic->descriptors) {
    if (descriptor.uuid == RemoteDescriptor::kCccdUuid) {
      return nullptr;
    }
  }

  auto cccd = std::make_unique<bluetooth_v2_shlib::Gatt::Descriptor>();
  cccd->uuid = RemoteDescriptor::kCccdUuid;
  cccd->permissions = static_cast<bluetooth_v2_shlib::Gatt::Permissions>(
      bluetooth_v2_shlib::Gatt::PERMISSION_READ |
      bluetooth_v2_shlib::Gatt::PERMISSION_WRITE);
  return cccd;
}

}  // namespace

RemoteCharacteristicImpl::RemoteCharacteristicImpl(
    RemoteDevice* device,
    base::WeakPtr<GattClientManagerImpl> gatt_client_manager,
    const bluetooth_v2_shlib::Gatt::Characteristic* characteristic,
    scoped_refptr<base::SingleThreadTaskRunner> io_task_runner)
    : device_(device),
      gatt_client_manager_(gatt_client_manager),
      characteristic_(characteristic),
      io_task_runner_(io_task_runner),
      fake_cccd_(MaybeCreateFakeCccd(characteristic)),
      uuid_to_descriptor_(CreateDescriptorMap()) {
  DCHECK(gatt_client_manager);
  DCHECK(characteristic);
  DCHECK(io_task_runner_->BelongsToCurrentThread());
}

RemoteCharacteristicImpl::~RemoteCharacteristicImpl() = default;

std::map<bluetooth_v2_shlib::Uuid, scoped_refptr<RemoteDescriptor>>
RemoteCharacteristicImpl::CreateDescriptorMap() {
  std::map<bluetooth_v2_shlib::Uuid, scoped_refptr<RemoteDescriptor>> ret;
  for (const auto& descriptor : characteristic_->descriptors) {
    ret[descriptor.uuid] = new RemoteDescriptorImpl(
        device_, gatt_client_manager_, &descriptor, io_task_runner_);
  }

  if (fake_cccd_) {
    DCHECK(ret.find(RemoteDescriptor::kCccdUuid) == ret.end());
    ret[fake_cccd_->uuid] = new RemoteDescriptorImpl(
        device_, gatt_client_manager_, fake_cccd_.get(), io_task_runner_);
  }

  return ret;
}

std::vector<scoped_refptr<RemoteDescriptor>>
RemoteCharacteristicImpl::GetDescriptors() {
  std::vector<scoped_refptr<RemoteDescriptor>> ret;
  ret.reserve(uuid_to_descriptor_.size());
  for (const auto& pair : uuid_to_descriptor_) {
    ret.push_back(pair.second);
  }

  return ret;
}

scoped_refptr<RemoteDescriptor> RemoteCharacteristicImpl::GetDescriptorByUuid(
    const bluetooth_v2_shlib::Uuid& uuid) {
  auto it = uuid_to_descriptor_.find(uuid);
  if (it == uuid_to_descriptor_.end()) {
    return nullptr;
  }

  return it->second;
}

void RemoteCharacteristicImpl::SetRegisterNotification(bool enable,
                                                       StatusCallback cb) {
  MAKE_SURE_IO_THREAD(SetRegisterNotification, enable,
                      BindToCurrentSequence(std::move(cb)));
  if (!gatt_client_manager_) {
    LOG(ERROR) << __func__ << " failed: Destroyed";
    EXEC_CB_AND_RET(cb, false);
  }

  if (!CharacteristicHasNotify(characteristic_)) {
    LOG(ERROR) << __func__
               << " failed: Characteristic doesn't support notifications";
    EXEC_CB_AND_RET(cb, false);
  }

  if (!gatt_client_manager_->gatt_client()->SetCharacteristicNotification(
          device_->addr(), *characteristic_, enable)) {
    LOG(ERROR) << "Set characteristic notification failed";
    EXEC_CB_AND_RET(cb, false);
  }

  // If device has no CCCD and we needed to create a fake one, just return
  // success.
  if (fake_cccd_) {
    EXEC_CB_AND_RET(cb, true);
  }

  auto it = uuid_to_descriptor_.find(RemoteDescriptor::kCccdUuid);

  // CCCD must exist. |fake_cccd_| should have been created if it doesn't exist.
  DCHECK(it != uuid_to_descriptor_.end());
  it->second->WriteAuth(bluetooth_v2_shlib::Gatt::Client::AUTH_REQ_NONE,
                        GetDescriptorNotificationValue(enable), std::move(cb));
}

void RemoteCharacteristicImpl::SetNotification(bool enable, StatusCallback cb) {
  MAKE_SURE_IO_THREAD(SetNotification, enable,
                      BindToCurrentSequence(std::move(cb)));
  if (!gatt_client_manager_) {
    LOG(ERROR) << __func__ << " failed: Destroyed";
    EXEC_CB_AND_RET(cb, false);
  }
  if (!gatt_client_manager_->gatt_client()->SetCharacteristicNotification(
          device_->addr(), *characteristic_, enable)) {
    LOG(ERROR) << "Set characteristic notification failed";
    EXEC_CB_AND_RET(cb, false);
  }

  notification_enabled_ = enable;
  EXEC_CB_AND_RET(cb, true);
}

void RemoteCharacteristicImpl::ReadAuth(
    bluetooth_v2_shlib::Gatt::Client::AuthReq auth_req,
    ReadCallback callback) {
  MAKE_SURE_IO_THREAD(ReadAuth, auth_req,
                      BindToCurrentSequence(std::move(callback)));
  if (!gatt_client_manager_) {
    LOG(ERROR) << __func__ << " failed: Destroyed";
    EXEC_CB_AND_RET(callback, false, {});
  }
  if (pending_read_) {
    LOG(ERROR) << "Read already pending";
    EXEC_CB_AND_RET(callback, false, {});
  }

  if (!gatt_client_manager_->gatt_client()->ReadCharacteristic(
          device_->addr(), *characteristic_, auth_req)) {
    EXEC_CB_AND_RET(callback, false, {});
  }
  pending_read_ = true;
  read_callback_ = std::move(callback);
}

void RemoteCharacteristicImpl::Read(ReadCallback callback) {
  ReadAuth(bluetooth_v2_shlib::Gatt::Client::AUTH_REQ_INVALID,
           std::move(callback));
}

void RemoteCharacteristicImpl::WriteAuth(
    bluetooth_v2_shlib::Gatt::Client::AuthReq auth_req,
    bluetooth_v2_shlib::Gatt::WriteType write_type,
    const std::vector<uint8_t>& value,
    StatusCallback callback) {
  MAKE_SURE_IO_THREAD(WriteAuth, auth_req, write_type, value,
                      BindToCurrentSequence(std::move(callback)));
  if (!gatt_client_manager_) {
    LOG(ERROR) << __func__ << " failed: Destroyed";
    EXEC_CB_AND_RET(callback, false);
  }
  if (pending_write_) {
    LOG(ERROR) << "Write already pending";
    EXEC_CB_AND_RET(callback, false);
  }

  if (!gatt_client_manager_->gatt_client()->WriteCharacteristic(
          device_->addr(), *characteristic_, auth_req, write_type, value)) {
    EXEC_CB_AND_RET(callback, false);
  }

  pending_write_ = true;
  write_callback_ = std::move(callback);
}

void RemoteCharacteristicImpl::Write(const std::vector<uint8_t>& value,
                                     StatusCallback callback) {
  using WriteType = bluetooth_v2_shlib::Gatt::WriteType;
  using Properties = bluetooth_v2_shlib::Gatt::Properties;

  WriteType write_type = WriteType::WRITE_TYPE_NONE;
  if (properties() & Properties::PROPERTY_WRITE) {
    write_type = WriteType::WRITE_TYPE_DEFAULT;
  } else if (properties() & Properties::PROPERTY_WRITE_NO_RESPONSE) {
    write_type = WriteType::WRITE_TYPE_NO_RESPONSE;
  } else if (properties() & Properties::PROPERTY_SIGNED_WRITE) {
    write_type = WriteType::WRITE_TYPE_SIGNED;
  } else {
    LOG(ERROR) << "Write not supported. Properties: "
               << static_cast<int>(properties());
    EXEC_CB_AND_RET(callback, false);
  }

  WriteAuth(bluetooth_v2_shlib::Gatt::Client::AUTH_REQ_NONE, write_type, value,
            std::move(callback));
}

bool RemoteCharacteristicImpl::NotificationEnabled() {
  return notification_enabled_;
}

const bluetooth_v2_shlib::Gatt::Characteristic&
RemoteCharacteristicImpl::characteristic() const {
  return *characteristic_;
}

const bluetooth_v2_shlib::Uuid& RemoteCharacteristicImpl::uuid() const {
  return characteristic_->uuid;
}

uint16_t RemoteCharacteristicImpl::handle() const {
  return characteristic_->handle;
}

bluetooth_v2_shlib::Gatt::Permissions RemoteCharacteristicImpl::permissions()
    const {
  return characteristic_->permissions;
}

bluetooth_v2_shlib::Gatt::Properties RemoteCharacteristicImpl::properties()
    const {
  return characteristic_->properties;
}

void RemoteCharacteristicImpl::OnConnectChanged(bool connected) {
  DCHECK(io_task_runner_->BelongsToCurrentThread());
  if (connected) {
    return;
  }

  pending_read_ = false;
  pending_write_ = false;

  if (read_callback_) {
    LOG(ERROR) << "Read failed: Device disconnected";
    std::move(read_callback_).Run(false, std::vector<uint8_t>());
  }

  if (write_callback_) {
    LOG(ERROR) << "Write failed: Device disconnected";
    std::move(write_callback_).Run(false);
  }
}

void RemoteCharacteristicImpl::OnReadComplete(
    bool status,
    const std::vector<uint8_t>& value) {
  DCHECK(io_task_runner_->BelongsToCurrentThread());
  pending_read_ = false;
  if (read_callback_) {
    std::move(read_callback_).Run(status, value);
  }
}

void RemoteCharacteristicImpl::OnWriteComplete(bool status) {
  DCHECK(io_task_runner_->BelongsToCurrentThread());
  pending_write_ = false;
  if (write_callback_)
    std::move(write_callback_).Run(status);
}

}  // namespace bluetooth
}  // namespace chromecast
