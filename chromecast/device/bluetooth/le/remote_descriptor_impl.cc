// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chromecast/device/bluetooth/le/remote_descriptor_impl.h"

#include "chromecast/base/bind_to_task_runner.h"
#include "chromecast/device/bluetooth/le/gatt_client_manager_impl.h"
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
      base::BindOnce(&RemoteDescriptorImpl::method, this, ##__VA_ARGS__));

#define MAKE_SURE_IO_THREAD(method, ...)            \
  DCHECK(io_task_runner_);                          \
  if (!io_task_runner_->BelongsToCurrentThread()) { \
    RUN_ON_IO_THREAD(method, ##__VA_ARGS__)         \
    return;                                         \
  }

namespace chromecast {
namespace bluetooth {

RemoteDescriptorImpl::RemoteDescriptorImpl(
    RemoteDevice* device,
    base::WeakPtr<GattClientManagerImpl> gatt_client_manager,
    const bluetooth_v2_shlib::Gatt::Descriptor* descriptor,
    scoped_refptr<base::SingleThreadTaskRunner> io_task_runner)
    : device_(device),
      gatt_client_manager_(std::move(gatt_client_manager)),
      descriptor_(descriptor),
      io_task_runner_(std::move(io_task_runner)) {
  DCHECK(device);
  DCHECK(gatt_client_manager_);
  DCHECK(descriptor);
  DCHECK(io_task_runner_->BelongsToCurrentThread());
}

RemoteDescriptorImpl::~RemoteDescriptorImpl() = default;

void RemoteDescriptorImpl::ReadAuth(
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

  if (!gatt_client_manager_->gatt_client()->ReadDescriptor(
          device_->addr(), *descriptor_, auth_req)) {
    EXEC_CB_AND_RET(callback, false, {});
  }
  pending_read_ = true;
  read_callback_ = std::move(callback);
}

void RemoteDescriptorImpl::Read(ReadCallback callback) {
  ReadAuth(bluetooth_v2_shlib::Gatt::Client::AUTH_REQ_INVALID,
           std::move(callback));
}

void RemoteDescriptorImpl::WriteAuth(
    bluetooth_v2_shlib::Gatt::Client::AuthReq auth_req,
    const std::vector<uint8_t>& value,
    StatusCallback callback) {
  MAKE_SURE_IO_THREAD(WriteAuth, auth_req, value,
                      BindToCurrentSequence(std::move(callback)));
  if (!gatt_client_manager_) {
    LOG(ERROR) << __func__ << " failed: Destroyed";
    EXEC_CB_AND_RET(callback, false);
  }
  if (pending_write_) {
    LOG(ERROR) << "Write already pending";
    EXEC_CB_AND_RET(callback, false);
  }

  if (!gatt_client_manager_->gatt_client()->WriteDescriptor(
          device_->addr(), *descriptor_, auth_req, value)) {
    EXEC_CB_AND_RET(callback, false);
  }
  pending_write_ = true;
  write_callback_ = std::move(callback);
}

void RemoteDescriptorImpl::Write(const std::vector<uint8_t>& value,
                                 StatusCallback callback) {
  WriteAuth(bluetooth_v2_shlib::Gatt::Client::AUTH_REQ_INVALID, value,
            std::move(callback));
}

const bluetooth_v2_shlib::Gatt::Descriptor& RemoteDescriptorImpl::descriptor()
    const {
  return *descriptor_;
}

const bluetooth_v2_shlib::Uuid RemoteDescriptorImpl::uuid() const {
  return descriptor_->uuid;
}

uint16_t RemoteDescriptorImpl::handle() const {
  return descriptor_->handle;
}

bluetooth_v2_shlib::Gatt::Permissions RemoteDescriptorImpl::permissions()
    const {
  return descriptor_->permissions;
}

void RemoteDescriptorImpl::OnConnectChanged(bool connected) {
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

void RemoteDescriptorImpl::OnReadComplete(bool status,
                                          const std::vector<uint8_t>& value) {
  DCHECK(io_task_runner_->BelongsToCurrentThread());
  pending_read_ = false;
  if (read_callback_)
    std::move(read_callback_).Run(status, value);
}

void RemoteDescriptorImpl::OnWriteComplete(bool status) {
  DCHECK(io_task_runner_->BelongsToCurrentThread());
  pending_write_ = false;
  if (write_callback_)
    std::move(write_callback_).Run(status);
}

}  // namespace bluetooth
}  // namespace chromecast
