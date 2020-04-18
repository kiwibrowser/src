// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef DEVICE_BLUETOOTH_DBUS_BLUETOOTH_GATT_CHARACTERISTIC_DELEGATE_WRAPPER_H_
#define DEVICE_BLUETOOTH_DBUS_BLUETOOTH_GATT_CHARACTERISTIC_DELEGATE_WRAPPER_H_

#include <cstdint>
#include <vector>

#include "base/callback_forward.h"
#include "base/macros.h"
#include "device/bluetooth/bluetooth_local_gatt_service.h"
#include "device/bluetooth/bluez/bluetooth_gatt_service_bluez.h"
#include "device/bluetooth/bluez/bluetooth_local_gatt_service_bluez.h"
#include "device/bluetooth/dbus/bluetooth_gatt_attribute_value_delegate.h"

namespace bluez {

class BluetoothLocalGattCharacteristicBlueZ;

// Wrapper class around AttributeValueDelegate to handle characteristics.
class BluetoothGattCharacteristicDelegateWrapper
    : public BluetoothGattAttributeValueDelegate {
 public:
  BluetoothGattCharacteristicDelegateWrapper(
      BluetoothLocalGattServiceBlueZ* service,
      BluetoothLocalGattCharacteristicBlueZ* characteristic);

  // BluetoothGattAttributeValueDelegate overrides:
  void GetValue(
      const dbus::ObjectPath& device_path,
      const device::BluetoothLocalGattService::Delegate::ValueCallback&
          callback,
      const device::BluetoothLocalGattService::Delegate::ErrorCallback&
          error_callback) override;
  void SetValue(
      const dbus::ObjectPath& device_path,
      const std::vector<uint8_t>& value,
      const base::Closure& callback,
      const device::BluetoothLocalGattService::Delegate::ErrorCallback&
          error_callback) override;
  void StartNotifications() override;
  void StopNotifications() override;

 private:
  BluetoothLocalGattCharacteristicBlueZ* characteristic_;

  DISALLOW_COPY_AND_ASSIGN(BluetoothGattCharacteristicDelegateWrapper);
};

}  // namespace bluez

#endif  // DEVICE_BLUETOOTH_DBUS_BLUETOOTH_GATT_CHARACTERISTIC_DELEGATE_WRAPPER_H_
