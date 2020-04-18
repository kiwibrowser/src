// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "device/fido/fido_ble_discovery.h"

#include <utility>

#include "base/bind.h"
#include "base/callback_helpers.h"
#include "device/bluetooth/bluetooth_adapter.h"
#include "device/bluetooth/bluetooth_common.h"
#include "device/bluetooth/bluetooth_discovery_session.h"
#include "device/bluetooth/bluetooth_uuid.h"
#include "device/fido/fido_ble_device.h"
#include "device/fido/fido_ble_uuids.h"

namespace device {

FidoBleDiscovery::FidoBleDiscovery() : weak_factory_(this) {}

FidoBleDiscovery::~FidoBleDiscovery() = default;

// static
const BluetoothUUID& FidoBleDiscovery::FidoServiceUUID() {
  static const BluetoothUUID service_uuid(kFidoServiceUUID);
  return service_uuid;
}

void FidoBleDiscovery::OnSetPowered() {
  DCHECK(adapter());
  VLOG(2) << "Adapter " << adapter()->GetAddress() << " is powered on.";

  for (BluetoothDevice* device : adapter()->GetDevices()) {
    if (base::ContainsKey(device->GetUUIDs(), FidoServiceUUID())) {
      VLOG(2) << "U2F BLE device: " << device->GetAddress();
      AddDevice(std::make_unique<FidoBleDevice>(device->GetAddress()));
    }
  }

  auto filter = std::make_unique<BluetoothDiscoveryFilter>(
      BluetoothTransport::BLUETOOTH_TRANSPORT_LE);
  filter->AddUUID(FidoServiceUUID());

  adapter()->StartDiscoverySessionWithFilter(
      std::move(filter),
      base::AdaptCallbackForRepeating(
          base::BindOnce(&FidoBleDiscovery::OnStartDiscoverySessionWithFilter,
                         weak_factory_.GetWeakPtr())),
      base::AdaptCallbackForRepeating(
          base::BindOnce(&FidoBleDiscovery::OnStartDiscoverySessionError,
                         weak_factory_.GetWeakPtr())));
}

void FidoBleDiscovery::DeviceAdded(BluetoothAdapter* adapter,
                                   BluetoothDevice* device) {
  if (base::ContainsKey(device->GetUUIDs(), FidoServiceUUID())) {
    VLOG(2) << "Discovered U2F BLE device: " << device->GetAddress();
    AddDevice(std::make_unique<FidoBleDevice>(device->GetAddress()));
  }
}

void FidoBleDiscovery::DeviceChanged(BluetoothAdapter* adapter,
                                     BluetoothDevice* device) {
  if (base::ContainsKey(device->GetUUIDs(), FidoServiceUUID()) &&
      !GetDevice(FidoBleDevice::GetId(device->GetAddress()))) {
    VLOG(2) << "Discovered U2F service on existing BLE device: "
            << device->GetAddress();
    AddDevice(std::make_unique<FidoBleDevice>(device->GetAddress()));
  }
}

void FidoBleDiscovery::DeviceRemoved(BluetoothAdapter* adapter,
                                     BluetoothDevice* device) {
  if (base::ContainsKey(device->GetUUIDs(), FidoServiceUUID())) {
    VLOG(2) << "U2F BLE device removed: " << device->GetAddress();
    RemoveDevice(FidoBleDevice::GetId(device->GetAddress()));
  }
}

}  // namespace device
