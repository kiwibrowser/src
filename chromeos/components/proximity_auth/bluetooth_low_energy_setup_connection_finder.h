// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROMEOS_COMPONENTS_PROXIMITY_AUTH_BLUETOOTH_LOW_ENERGY_SETUP_CONNECTION_FINDER_H_
#define CHROMEOS_COMPONENTS_PROXIMITY_AUTH_BLUETOOTH_LOW_ENERGY_SETUP_CONNECTION_FINDER_H_

#include <string>

#include "base/macros.h"
#include "chromeos/components/proximity_auth/bluetooth_low_energy_connection_finder.h"
#include "components/cryptauth/remote_device_ref.h"
#include "device/bluetooth/bluetooth_device.h"

namespace proximity_auth {

// This cryptauth::ConnectionFinder implementation is specialized in finding a
// Bluetooth Low Energy remote device based on the advertised service UUID.
class BluetoothLowEnergySetupConnectionFinder
    : public BluetoothLowEnergyConnectionFinder {
 public:
  // Finds (and connects) to a Bluetooth low energy device, based on the UUID
  // advertised by the remote device.
  //
  // |remote_service_uuid|: The UUID of the service used to send/receive data in
  // remote device.
  BluetoothLowEnergySetupConnectionFinder(
      const std::string& remote_service_uuid);

 private:
  // Checks if |device| is the right device, that is is adversing tthe right
  // service UUID.
  bool IsRightDevice(device::BluetoothDevice* device) override;

  // The UUID of the service advertised by the remote device.
  device::BluetoothUUID remote_service_uuid_;

  DISALLOW_COPY_AND_ASSIGN(BluetoothLowEnergySetupConnectionFinder);
};

}  // namespace proximity_auth

#endif  // CHROMEOS_COMPONENTS_PROXIMITY_AUTH_BLUETOOTH_LOW_ENERGY_SETUP_CONNECTION_FINDER_H_
