// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_CRYPTAUTH_BLE_BLUETOOTH_LOW_ENERGY_CHARACTERISTICS_FINDER_H_
#define COMPONENTS_CRYPTAUTH_BLE_BLUETOOTH_LOW_ENERGY_CHARACTERISTICS_FINDER_H_

#include "base/callback.h"
#include "base/macros.h"
#include "base/memory/ref_counted.h"
#include "components/cryptauth/ble/remote_attribute.h"
#include "device/bluetooth/bluetooth_adapter.h"
#include "device/bluetooth/bluetooth_device.h"
#include "device/bluetooth/bluetooth_remote_gatt_characteristic.h"
#include "device/bluetooth/bluetooth_remote_gatt_service.h"
#include "device/bluetooth/bluetooth_uuid.h"

namespace cryptauth {

// Looks for given characteristics in a remote device, for which a GATT
// connection was already established. In the current BLE connection protocol
// (device::BluetoothDevice::CreateGattConnection), remote characteristic
// discovery starts immediatelly after a GATT connection was established. So,
// this class simply adds an observer for a characteristic discovery event and
// call |success_callback_| once all necessary characteristics were discovered.
class BluetoothLowEnergyCharacteristicsFinder
    : public device::BluetoothAdapter::Observer {
 public:
  // This callbacks takes as arguments (in this order): |remote_service_|,
  // |to_peripheral_char_| and |from_peripheral_char_|. Note that, since this is
  // called after the characteristics were discovered, their id field (e.g.
  // to_peripheral_char_.id) will be non-blank.
  typedef base::Callback<void(const RemoteAttribute&,
                              const RemoteAttribute&,
                              const RemoteAttribute&)>
      SuccessCallback;

  // This callback takes as arguments (in this order): |to_peripheral_char_| and
  // |from_peripheral_char_|. A blank id field in the characteristics indicate
  // that the characteristics was not found in the remote service.
  // TODO(sacomoto): Remove RemoteAttributes and add an error message instead.
  // The caller of this object should not care if only a subset of the
  // characteristics was found. See crbug.com/495511.
  typedef base::Callback<void(const RemoteAttribute&, const RemoteAttribute&)>
      ErrorCallback;

  // Constructs the object and registers itself as an observer for |adapter|,
  // waiting for |to_peripheral_char| and |from_peripheral_char| to be found.
  // When both characteristics were found |success_callback| is called. After
  // all characteristics of |service| were discovered, if |from_periphral_char|
  // or |to_peripheral| was not found, it calls |error_callback|. The object
  // will perform at most one call of the callbacks.
  BluetoothLowEnergyCharacteristicsFinder(
      scoped_refptr<device::BluetoothAdapter> adapter,
      device::BluetoothDevice* device,
      const RemoteAttribute& remote_service,
      const RemoteAttribute& to_peripheral_char,
      const RemoteAttribute& from_peripheral_char,
      const SuccessCallback& success_callback,
      const ErrorCallback& error_callback);

  ~BluetoothLowEnergyCharacteristicsFinder() override;

 protected:
  // device::BluetoothAdapter::Observer:
  void GattDiscoveryCompleteForService(
      device::BluetoothAdapter* adapter,
      device::BluetoothRemoteGattService* service) override;
  void GattServicesDiscovered(device::BluetoothAdapter* adapter,
                              device::BluetoothDevice* device) override;
  void GattCharacteristicAdded(
      device::BluetoothAdapter* adapter,
      device::BluetoothRemoteGattCharacteristic* characteristic) override;

  // For testing. Used to mock this class.
  BluetoothLowEnergyCharacteristicsFinder();

 private:
  // Handles the discovery of a new characteristic. Returns whether all
  // characteristics were found.
  bool HandleCharacteristicUpdate(
      device::BluetoothRemoteGattCharacteristic* characteristic);

  // Ends the characteristic discovery and calls error callback if necessary.
  void OnCharacteristicDiscoveryEnded(device::BluetoothDevice* device);

  // Scans the remote chracteristics of the service with |uuid| in |device|
  // calling HandleCharacteristicUpdate() for each of them.
  void ScanRemoteCharacteristics(device::BluetoothDevice* device,
                                 const device::BluetoothUUID& uuid);

  // Updates the value of |to_peripheral_char_| and
  // |from_peripheral_char_|
  // when |characteristic| was found.
  void UpdateCharacteristicsStatus(
      device::BluetoothRemoteGattCharacteristic* characteristic);

  // The Bluetooth adapter where the connection was established.
  scoped_refptr<device::BluetoothAdapter> adapter_;

  // The Bluetooth device to which the connection was established.
  device::BluetoothDevice* bluetooth_device_;

  // Remote service the |connection_| was established with.
  RemoteAttribute remote_service_;

  // Characteristic used to receive data from the remote device.
  RemoteAttribute to_peripheral_char_;

  // Characteristic used to receive data from the remote device.
  RemoteAttribute from_peripheral_char_;

  // Called when all characteristics were found.
  SuccessCallback success_callback_;

  // Keeps track whether we have ever call the error callback.
  bool has_error_callback_been_invoked_ = false;

  // Called when there is an error.
  ErrorCallback error_callback_;

  DISALLOW_COPY_AND_ASSIGN(BluetoothLowEnergyCharacteristicsFinder);
};

}  // namespace cryptauth

#endif  // COMPONENTS_CRYPTAUTH_BLE_BLUETOOTH_CHARACTERISTICS_FINDER_H_
