// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef DEVICE_BLUETOOTH_BLUEZ_BLUETOOTH_REMOTE_GATT_CHARACTERISTIC_BLUEZ_H_
#define DEVICE_BLUETOOTH_BLUEZ_BLUETOOTH_REMOTE_GATT_CHARACTERISTIC_BLUEZ_H_

#include <stddef.h>
#include <stdint.h>
#include <map>
#include <memory>
#include <queue>
#include <string>
#include <utility>
#include <vector>

#include "base/callback_forward.h"
#include "base/macros.h"
#include "base/memory/weak_ptr.h"
#include "dbus/object_path.h"
#include "device/bluetooth/bluetooth_remote_gatt_characteristic.h"
#include "device/bluetooth/bluetooth_remote_gatt_service.h"
#include "device/bluetooth/bluetooth_uuid.h"
#include "device/bluetooth/bluez/bluetooth_gatt_characteristic_bluez.h"
#include "device/bluetooth/dbus/bluetooth_gatt_descriptor_client.h"

namespace device {

class BluetoothRemoteGattDescriptor;
class BluetoothRemoteGattService;

}  // namespace device

namespace bluez {

class BluetoothRemoteGattDescriptorBlueZ;
class BluetoothRemoteGattServiceBlueZ;

// The BluetoothRemoteGattCharacteristicBlueZ class implements
// BluetoothRemoteGattCharacteristic for remote GATT characteristics for
// platforms
// that use BlueZ.
class BluetoothRemoteGattCharacteristicBlueZ
    : public BluetoothGattCharacteristicBlueZ,
      public BluetoothGattDescriptorClient::Observer,
      public device::BluetoothRemoteGattCharacteristic {
 public:
  // device::BluetoothGattCharacteristic overrides.
  ~BluetoothRemoteGattCharacteristicBlueZ() override;
  device::BluetoothUUID GetUUID() const override;
  Properties GetProperties() const override;
  Permissions GetPermissions() const override;

  // device::BluetoothRemoteGattCharacteristic overrides.
  const std::vector<uint8_t>& GetValue() const override;
  device::BluetoothRemoteGattService* GetService() const override;
  bool IsNotifying() const override;
  std::vector<device::BluetoothRemoteGattDescriptor*> GetDescriptors()
      const override;
  device::BluetoothRemoteGattDescriptor* GetDescriptor(
      const std::string& identifier) const override;
  void ReadRemoteCharacteristic(const ValueCallback& callback,
                                const ErrorCallback& error_callback) override;
  void WriteRemoteCharacteristic(const std::vector<uint8_t>& value,
                                 const base::Closure& callback,
                                 const ErrorCallback& error_callback) override;

 protected:
  void SubscribeToNotifications(
      device::BluetoothRemoteGattDescriptor* ccc_descriptor,
      const base::Closure& callback,
      const ErrorCallback& error_callback) override;
  void UnsubscribeFromNotifications(
      device::BluetoothRemoteGattDescriptor* ccc_descriptor,
      const base::Closure& callback,
      const ErrorCallback& error_callback) override;

 private:
  friend class BluetoothRemoteGattServiceBlueZ;

  BluetoothRemoteGattCharacteristicBlueZ(
      BluetoothRemoteGattServiceBlueZ* service,
      const dbus::ObjectPath& object_path);

  // bluez::BluetoothGattDescriptorClient::Observer overrides.
  void GattDescriptorAdded(const dbus::ObjectPath& object_path) override;
  void GattDescriptorRemoved(const dbus::ObjectPath& object_path) override;
  void GattDescriptorPropertyChanged(const dbus::ObjectPath& object_path,
                                     const std::string& property_name) override;

  // Called by dbus:: on successful completion of a request to start
  // notifications.
  void OnStartNotifySuccess(const base::Closure& callback);

  // Called by dbus:: on unsuccessful completion of a request to start
  // notifications.
  void OnStartNotifyError(const ErrorCallback& error_callback,
                          const std::string& error_name,
                          const std::string& error_message);

  // Called by dbus:: on successful completion of a request to stop
  // notifications.
  void OnStopNotifySuccess(const base::Closure& callback);

  // Called by dbus:: on unsuccessful completion of a request to stop
  // notifications.
  void OnStopNotifyError(const base::Closure& callback,
                         const std::string& error_name,
                         const std::string& error_message);

  // Called by dbus:: on unsuccessful completion of a request to read
  // the characteristic value.
  void OnReadError(const ErrorCallback& error_callback,
                   const std::string& error_name,
                   const std::string& error_message);

  // Called by dbus:: on unsuccessful completion of a request to write
  // the characteristic value.
  void OnWriteError(const ErrorCallback& error_callback,
                    const std::string& error_name,
                    const std::string& error_message);

  // True, if there exists a Bluez notify session.
  bool has_notify_session_;

  using DescriptorMap =
      std::map<dbus::ObjectPath,
               std::unique_ptr<BluetoothRemoteGattDescriptorBlueZ>>;

  // Mapping from GATT descriptor object paths to descriptor objects owned by
  // this characteristic. Since the BlueZ implementation uses object paths
  // as unique identifiers, we also use this mapping to return descriptors by
  // identifier.
  DescriptorMap descriptors_;

  // The GATT service this GATT characteristic belongs to.
  BluetoothRemoteGattServiceBlueZ* service_;

  // Number of gatt read requests in progress.
  int num_of_characteristic_value_read_in_progress_;

  // Note: This should remain the last member so it'll be destroyed and
  // invalidate its weak pointers before any other members are destroyed.
  base::WeakPtrFactory<BluetoothRemoteGattCharacteristicBlueZ>
      weak_ptr_factory_;

  DISALLOW_COPY_AND_ASSIGN(BluetoothRemoteGattCharacteristicBlueZ);
};

}  // namespace bluez

#endif  // DEVICE_BLUETOOTH_BLUEZ_BLUETOOTH_REMOTE_GATT_CHARACTERISTIC_BLUEZ_H_
