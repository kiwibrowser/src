// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "device/bluetooth/dbus/bluetooth_gatt_service_service_provider.h"

#include "device/bluetooth/dbus/bluetooth_gatt_service_service_provider_impl.h"
#include "device/bluetooth/dbus/bluez_dbus_manager.h"
#include "device/bluetooth/dbus/fake_bluetooth_gatt_service_service_provider.h"

namespace bluez {

BluetoothGattServiceServiceProvider::BluetoothGattServiceServiceProvider() =
    default;

BluetoothGattServiceServiceProvider::~BluetoothGattServiceServiceProvider() =
    default;

// static
BluetoothGattServiceServiceProvider*
BluetoothGattServiceServiceProvider::Create(
    dbus::Bus* bus,
    const dbus::ObjectPath& object_path,
    const std::string& uuid,
    bool is_primary,
    const std::vector<dbus::ObjectPath>& includes) {
  if (!bluez::BluezDBusManager::Get()->IsUsingFakes()) {
    return new BluetoothGattServiceServiceProviderImpl(bus, object_path, uuid,
                                                       is_primary, includes);
  }
  return new FakeBluetoothGattServiceServiceProvider(object_path, uuid,
                                                     includes);
}

}  // namespace bluez
