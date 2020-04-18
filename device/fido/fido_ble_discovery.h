// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef DEVICE_FIDO_FIDO_BLE_DISCOVERY_H_
#define DEVICE_FIDO_FIDO_BLE_DISCOVERY_H_

#include <memory>

#include "base/component_export.h"
#include "base/macros.h"
#include "base/memory/weak_ptr.h"
#include "device/fido/fido_ble_discovery_base.h"

namespace device {

class BluetoothDevice;
class BluetoothUUID;

class COMPONENT_EXPORT(DEVICE_FIDO) FidoBleDiscovery
    : public FidoBleDiscoveryBase {
 public:
  FidoBleDiscovery();
  ~FidoBleDiscovery() override;

 private:
  static const BluetoothUUID& FidoServiceUUID();

  // FidoBleDiscoveryBase:
  void OnSetPowered() override;

  // BluetoothAdapter::Observer:
  void DeviceAdded(BluetoothAdapter* adapter, BluetoothDevice* device) override;
  void DeviceChanged(BluetoothAdapter* adapter,
                     BluetoothDevice* device) override;
  void DeviceRemoved(BluetoothAdapter* adapter,
                     BluetoothDevice* device) override;

  base::WeakPtrFactory<FidoBleDiscovery> weak_factory_;

  DISALLOW_COPY_AND_ASSIGN(FidoBleDiscovery);
};

}  // namespace device

#endif  // DEVICE_FIDO_FIDO_BLE_DISCOVERY_H_
