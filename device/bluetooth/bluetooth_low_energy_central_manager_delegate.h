// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef DEVICE_BLUETOOTH_BLUETOOTH_LOW_ENERGY_CENTRAL_MANAGER_DELEGATE_H_
#define DEVICE_BLUETOOTH_BLUETOOTH_LOW_ENERGY_CENTRAL_MANAGER_DELEGATE_H_

#include <memory>

#include "base/mac/sdk_forward_declarations.h"
#include "build/build_config.h"

#if defined(OS_IOS)
#import <CoreBluetooth/CoreBluetooth.h>
#else
#import <IOBluetooth/IOBluetooth.h>
#endif

namespace device {

class BluetoothAdapterMac;
class BluetoothLowEnergyCentralManagerBridge;
class BluetoothLowEnergyDiscoveryManagerMac;

}  // namespace device

// This class will serve as the Objective-C delegate of CBCentralManager.
@interface BluetoothLowEnergyCentralManagerDelegate
    : NSObject<CBCentralManagerDelegate> {
  std::unique_ptr<device::BluetoothLowEnergyCentralManagerBridge> bridge_;
}

- (id)initWithDiscoveryManager:
          (device::BluetoothLowEnergyDiscoveryManagerMac*)discovery_manager
                    andAdapter:(device::BluetoothAdapterMac*)adapter;

@end

#endif  // DEVICE_BLUETOOTH_BLUETOOTH_LOW_ENERGY_CENTRAL_MANAGER_DELEGATE_H_
