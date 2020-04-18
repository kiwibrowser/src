// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "device/bluetooth/cast/bluetooth_remote_gatt_characteristic_cast.h"

#include "base/bind.h"
#include "base/callback.h"
#include "base/callback_forward.h"
#include "base/containers/queue.h"
#include "base/macros.h"
#include "base/memory/weak_ptr.h"
#include "chromecast/device/bluetooth/le/remote_characteristic.h"
#include "chromecast/device/bluetooth/le/remote_descriptor.h"
#include "device/bluetooth/bluetooth_uuid.h"
#include "device/bluetooth/cast/bluetooth_remote_gatt_descriptor_cast.h"
#include "device/bluetooth/cast/bluetooth_remote_gatt_service_cast.h"
#include "device/bluetooth/cast/bluetooth_utils.h"

namespace device {
namespace {

BluetoothGattCharacteristic::Permissions ConvertPermissions(
    chromecast::bluetooth_v2_shlib::Gatt::Permissions input) {
  BluetoothGattCharacteristic::Permissions output =
      BluetoothGattCharacteristic::PERMISSION_NONE;
  if (input & chromecast::bluetooth_v2_shlib::Gatt::PERMISSION_READ)
    output |= BluetoothGattCharacteristic::PERMISSION_READ;
  if (input & chromecast::bluetooth_v2_shlib::Gatt::PERMISSION_READ_ENCRYPTED)
    output |= BluetoothGattCharacteristic::PERMISSION_READ_ENCRYPTED;
  if (input & chromecast::bluetooth_v2_shlib::Gatt::PERMISSION_WRITE)
    output |= BluetoothGattCharacteristic::PERMISSION_WRITE;
  if (input & chromecast::bluetooth_v2_shlib::Gatt::PERMISSION_WRITE_ENCRYPTED)
    output |= BluetoothGattCharacteristic::PERMISSION_WRITE_ENCRYPTED;

  // NOTE(slan): Determine the proper mapping for these.
  // if (input & chromecast::bluetooth_v2_shlib::PERMISSION_READ_ENCRYPTED_MITM)
  //   output |= BluetoothGattCharacteristic::PERMISSION_READ_ENCRYPTED_MITM;
  // if (input &
  // chromecast::bluetooth_v2_shlib::PERMISSION_WRITE_ENCRYPTED_MITM)
  //   output |= BluetoothGattCharacteristic::PERMISSION_WRITE_ENCRYPTED_MITM;
  // if (input & chromecast::bluetooth_v2_shlib::PERMISSION_WRITE_SIGNED)
  //   output |= BluetoothGattCharacteristic::PERMISSION_WRITE_SIGNED;
  // if (input & chromecast::bluetooth_v2_shlib::PERMISSION_WRITE_SIGNED_MITM)
  //   output |= BluetoothGattCharacteristic::PERMISSION_WRITE_SIGNED_MITM;

  return output;
}

BluetoothGattCharacteristic::Properties ConvertProperties(
    chromecast::bluetooth_v2_shlib::Gatt::Properties input) {
  BluetoothGattCharacteristic::Properties output =
      BluetoothGattCharacteristic::PROPERTY_NONE;
  if (input & chromecast::bluetooth_v2_shlib::Gatt::PROPERTY_BROADCAST)
    output |= BluetoothGattCharacteristic::PROPERTY_BROADCAST;
  if (input & chromecast::bluetooth_v2_shlib::Gatt::PROPERTY_READ)
    output |= BluetoothGattCharacteristic::PROPERTY_READ;
  if (input & chromecast::bluetooth_v2_shlib::Gatt::PROPERTY_WRITE_NO_RESPONSE)
    output |= BluetoothGattCharacteristic::PROPERTY_WRITE_WITHOUT_RESPONSE;
  if (input & chromecast::bluetooth_v2_shlib::Gatt::PROPERTY_WRITE)
    output |= BluetoothGattCharacteristic::PROPERTY_WRITE;
  if (input & chromecast::bluetooth_v2_shlib::Gatt::PROPERTY_NOTIFY)
    output |= BluetoothGattCharacteristic::PROPERTY_NOTIFY;
  if (input & chromecast::bluetooth_v2_shlib::Gatt::PROPERTY_INDICATE)
    output |= BluetoothGattCharacteristic::PROPERTY_INDICATE;
  if (input & chromecast::bluetooth_v2_shlib::Gatt::PROPERTY_SIGNED_WRITE)
    output |= BluetoothGattCharacteristic::PROPERTY_AUTHENTICATED_SIGNED_WRITES;
  if (input & chromecast::bluetooth_v2_shlib::Gatt::PROPERTY_EXTENDED_PROPS)
    output |= BluetoothGattCharacteristic::PROPERTY_EXTENDED_PROPERTIES;

  return output;
}

// Called back when subscribing or unsubscribing to a remote characteristic.
// If |success| is true, run |callback|. Otherwise run |error_callback|.
void OnSubscribeOrUnsubscribe(
    const base::Closure& callback,
    const BluetoothGattCharacteristic::ErrorCallback& error_callback,
    bool success) {
  if (success)
    callback.Run();
  else
    error_callback.Run(BluetoothGattService::GATT_ERROR_FAILED);
}

}  // namespace

BluetoothRemoteGattCharacteristicCast::BluetoothRemoteGattCharacteristicCast(
    BluetoothRemoteGattServiceCast* service,
    scoped_refptr<chromecast::bluetooth::RemoteCharacteristic> characteristic)
    : service_(service),
      remote_characteristic_(std::move(characteristic)),
      weak_factory_(this) {
  auto descriptors = remote_characteristic_->GetDescriptors();
  descriptors_.reserve(descriptors.size());
  for (const auto& descriptor : descriptors) {
    descriptors_.push_back(
        std::make_unique<BluetoothRemoteGattDescriptorCast>(this, descriptor));
  }
}

BluetoothRemoteGattCharacteristicCast::
    ~BluetoothRemoteGattCharacteristicCast() {}

std::string BluetoothRemoteGattCharacteristicCast::GetIdentifier() const {
  return GetUUID().canonical_value();
}

BluetoothUUID BluetoothRemoteGattCharacteristicCast::GetUUID() const {
  return UuidToBluetoothUUID(remote_characteristic_->uuid());
}

BluetoothGattCharacteristic::Properties
BluetoothRemoteGattCharacteristicCast::GetProperties() const {
  return ConvertProperties(remote_characteristic_->properties());
}

BluetoothGattCharacteristic::Permissions
BluetoothRemoteGattCharacteristicCast::GetPermissions() const {
  return ConvertPermissions(remote_characteristic_->permissions());
}

const std::vector<uint8_t>& BluetoothRemoteGattCharacteristicCast::GetValue()
    const {
  return value_;
}

BluetoothRemoteGattService* BluetoothRemoteGattCharacteristicCast::GetService()
    const {
  return service_;
}

std::vector<BluetoothRemoteGattDescriptor*>
BluetoothRemoteGattCharacteristicCast::GetDescriptors() const {
  std::vector<BluetoothRemoteGattDescriptor*> descriptors;
  descriptors.reserve(descriptors_.size());
  for (auto& descriptor : descriptors_) {
    descriptors.push_back(descriptor.get());
  }
  return descriptors;
}

BluetoothRemoteGattDescriptor*
BluetoothRemoteGattCharacteristicCast::GetDescriptor(
    const std::string& identifier) const {
  for (auto& descriptor : descriptors_) {
    if (descriptor->GetIdentifier() == identifier) {
      return descriptor.get();
    }
  }
  return nullptr;
}

void BluetoothRemoteGattCharacteristicCast::ReadRemoteCharacteristic(
    const ValueCallback& callback,
    const ErrorCallback& error_callback) {
  remote_characteristic_->Read(base::BindOnce(
      &BluetoothRemoteGattCharacteristicCast::OnReadRemoteCharacteristic,
      weak_factory_.GetWeakPtr(), callback, error_callback));
}

void BluetoothRemoteGattCharacteristicCast::WriteRemoteCharacteristic(
    const std::vector<uint8_t>& value,
    const base::Closure& callback,
    const ErrorCallback& error_callback) {
  remote_characteristic_->Write(
      value,
      base::BindOnce(
          &BluetoothRemoteGattCharacteristicCast::OnWriteRemoteCharacteristic,
          weak_factory_.GetWeakPtr(), value, callback, error_callback));
}

void BluetoothRemoteGattCharacteristicCast::SubscribeToNotifications(
    BluetoothRemoteGattDescriptor* ccc_descriptor,
    const base::Closure& callback,
    const ErrorCallback& error_callback) {
  DVLOG(2) << __func__ << " " << GetIdentifier();

  // |remote_characteristic_| exposes a method which writes the CCCD after
  // subscribing the GATT client to the notification. This is syntactically
  // nicer and saves us a thread-hop, so we can ignore |ccc_descriptor|.
  (void)ccc_descriptor;

  remote_characteristic_->SetRegisterNotification(
      true,
      base::BindOnce(&OnSubscribeOrUnsubscribe, callback, error_callback));
}

void BluetoothRemoteGattCharacteristicCast::UnsubscribeFromNotifications(
    BluetoothRemoteGattDescriptor* ccc_descriptor,
    const base::Closure& callback,
    const ErrorCallback& error_callback) {
  DVLOG(2) << __func__ << " " << GetIdentifier();

  // |remote_characteristic_| exposes a method which writes the CCCD after
  // unsubscribing the GATT client from the notification. This is syntactically
  // nicer and saves us a thread-hop, so we can ignore |ccc_descriptor|.
  (void)ccc_descriptor;

  remote_characteristic_->SetRegisterNotification(
      false,
      base::BindOnce(&OnSubscribeOrUnsubscribe, callback, error_callback));
}

void BluetoothRemoteGattCharacteristicCast::OnReadRemoteCharacteristic(
    const ValueCallback& callback,
    const ErrorCallback& error_callback,
    bool success,
    const std::vector<uint8_t>& result) {
  if (success) {
    value_ = result;
    callback.Run(result);
    return;
  }
  error_callback.Run(BluetoothGattService::GATT_ERROR_FAILED);
}

void BluetoothRemoteGattCharacteristicCast::OnWriteRemoteCharacteristic(
    const std::vector<uint8_t>& written_value,
    const base::Closure& callback,
    const ErrorCallback& error_callback,
    bool success) {
  if (success) {
    value_ = written_value;
    callback.Run();
    return;
  }
  error_callback.Run(BluetoothGattService::GATT_ERROR_FAILED);
}

}  // namespace device
