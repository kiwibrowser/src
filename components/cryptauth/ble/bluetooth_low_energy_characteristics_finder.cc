// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/cryptauth/ble/bluetooth_low_energy_characteristics_finder.h"

#include "chromeos/components/proximity_auth/logging/logging.h"
#include "device/bluetooth/bluetooth_adapter.h"
#include "device/bluetooth/bluetooth_device.h"
#include "device/bluetooth/bluetooth_remote_gatt_characteristic.h"
#include "device/bluetooth/bluetooth_uuid.h"

using device::BluetoothAdapter;
using device::BluetoothDevice;
using device::BluetoothRemoteGattCharacteristic;
using device::BluetoothRemoteGattService;
using device::BluetoothUUID;

namespace cryptauth {

BluetoothLowEnergyCharacteristicsFinder::
    BluetoothLowEnergyCharacteristicsFinder(
        scoped_refptr<BluetoothAdapter> adapter,
        BluetoothDevice* device,
        const RemoteAttribute& remote_service,
        const RemoteAttribute& to_peripheral_char,
        const RemoteAttribute& from_peripheral_char,
        const SuccessCallback& success_callback,
        const ErrorCallback& error_callback)
    : adapter_(adapter),
      bluetooth_device_(device),
      remote_service_(remote_service),
      to_peripheral_char_(to_peripheral_char),
      from_peripheral_char_(from_peripheral_char),
      success_callback_(success_callback),
      error_callback_(error_callback) {
  adapter_->AddObserver(this);
  ScanRemoteCharacteristics(device, remote_service_.uuid);
}

BluetoothLowEnergyCharacteristicsFinder::
    BluetoothLowEnergyCharacteristicsFinder() {}

BluetoothLowEnergyCharacteristicsFinder::
    ~BluetoothLowEnergyCharacteristicsFinder() {
  if (adapter_) {
    adapter_->RemoveObserver(this);
  }
}

void BluetoothLowEnergyCharacteristicsFinder::GattCharacteristicAdded(
    BluetoothAdapter* adapter,
    BluetoothRemoteGattCharacteristic* characteristic) {
  // Ignore events about other devices.
  if (characteristic->GetService()->GetDevice() != bluetooth_device_)
    return;
  HandleCharacteristicUpdate(characteristic);
}

void BluetoothLowEnergyCharacteristicsFinder::GattDiscoveryCompleteForService(
    BluetoothAdapter* adapter,
    BluetoothRemoteGattService* service) {
  if (!service || service->GetUUID() != remote_service_.uuid)
    return;

  OnCharacteristicDiscoveryEnded(service->GetDevice());
}

void BluetoothLowEnergyCharacteristicsFinder::GattServicesDiscovered(
    BluetoothAdapter* adapter,
    BluetoothDevice* device) {
  OnCharacteristicDiscoveryEnded(device);
}

void BluetoothLowEnergyCharacteristicsFinder::OnCharacteristicDiscoveryEnded(
    BluetoothDevice* device) {
  // Ignore events about other devices.
  if (device != bluetooth_device_)
    return;

  if (!to_peripheral_char_.id.empty() && !from_peripheral_char_.id.empty())
    return;

  if (has_error_callback_been_invoked_)
    return;
  // If all GATT services have been discovered and we haven't found the
  // characteristics we are looking for, call the error callback.
  has_error_callback_been_invoked_ = true;
  error_callback_.Run(to_peripheral_char_, from_peripheral_char_);
}

void BluetoothLowEnergyCharacteristicsFinder::ScanRemoteCharacteristics(
    BluetoothDevice* device,
    const BluetoothUUID& service_uuid) {
  if (!device)
    return;

  for (const auto* service : device->GetGattServices()) {
    if (service->GetUUID() != service_uuid)
      continue;

    // Right service found, now scaning its characteristics.
    std::vector<device::BluetoothRemoteGattCharacteristic*> characteristics =
        service->GetCharacteristics();
    for (auto* characteristic : characteristics) {
      if (HandleCharacteristicUpdate(characteristic))
        return;
    }
    break;
  }
}

bool BluetoothLowEnergyCharacteristicsFinder::HandleCharacteristicUpdate(
    BluetoothRemoteGattCharacteristic* characteristic) {
  UpdateCharacteristicsStatus(characteristic);

  if (to_peripheral_char_.id.empty() || from_peripheral_char_.id.empty())
    return false;

  success_callback_.Run(remote_service_, to_peripheral_char_,
                        from_peripheral_char_);
  return true;
}

void BluetoothLowEnergyCharacteristicsFinder::UpdateCharacteristicsStatus(
    BluetoothRemoteGattCharacteristic* characteristic) {
  if (!characteristic ||
      characteristic->GetService()->GetUUID() != remote_service_.uuid) {
    return;
  }

  BluetoothUUID uuid = characteristic->GetUUID();
  if (to_peripheral_char_.uuid == uuid)
    to_peripheral_char_.id = characteristic->GetIdentifier();
  if (from_peripheral_char_.uuid == uuid)
    from_peripheral_char_.id = characteristic->GetIdentifier();

  BluetoothRemoteGattService* service = characteristic->GetService();
  if (service)
    remote_service_.id = service->GetIdentifier();
}

}  // namespace cryptauth
