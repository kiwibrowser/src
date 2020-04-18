// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ASH_SYSTEM_BLUETOOTH_TRAY_BLUETOOTH_HELPER_H_
#define ASH_SYSTEM_BLUETOOTH_TRAY_BLUETOOTH_HELPER_H_

#include <memory>
#include <vector>

#include "ash/ash_export.h"
#include "base/macros.h"
#include "base/memory/ref_counted.h"
#include "base/memory/weak_ptr.h"
#include "device/bluetooth/bluetooth_adapter.h"

namespace device {
class BluetoothDiscoverySession;
}

namespace ash {

// Cached info from device::BluetoothDevice used for display in the UI.
// Exists because it is not safe to cache pointers to device::BluetoothDevice
// instances.
struct ASH_EXPORT BluetoothDeviceInfo {
  BluetoothDeviceInfo();
  BluetoothDeviceInfo(const BluetoothDeviceInfo& other);
  ~BluetoothDeviceInfo();

  std::string address;
  base::string16 display_name;
  bool connected;
  bool connecting;
  bool paired;
  device::BluetoothDeviceType device_type;
};

using BluetoothDeviceList = std::vector<BluetoothDeviceInfo>;

// Maps UI concepts from the Bluetooth system tray (e.g. "Bluetooth is on") into
// device concepts ("Bluetooth adapter enabled"). Note that most Bluetooth
// device operations are asynchronous, hence the two step initialization.
// Exported for test.
class ASH_EXPORT TrayBluetoothHelper
    : public device::BluetoothAdapter::Observer {
 public:
  TrayBluetoothHelper();
  ~TrayBluetoothHelper() override;

  // Initializes and gets the adapter asynchronously.
  void Initialize();

  // Completes initialization after the Bluetooth adapter is ready.
  void InitializeOnAdapterReady(
      scoped_refptr<device::BluetoothAdapter> adapter);

  // Returns a list of available bluetooth devices.
  BluetoothDeviceList GetAvailableBluetoothDevices() const;

  // Requests bluetooth start discovering devices, which happens asynchronously.
  void StartBluetoothDiscovering();

  // Requests bluetooth stop discovering devices.
  void StopBluetoothDiscovering();

  // Connect to a specific bluetooth device.
  void ConnectToBluetoothDevice(const std::string& address);

  // Returns whether bluetooth capability is available (e.g. the device has
  // hardware support).
  bool GetBluetoothAvailable();

  // Returns whether bluetooth is enabled.
  bool GetBluetoothEnabled();

  // Changes bluetooth state to |enabled|. If the current state and |enabled|
  // are same, it does nothing. If they're different, it toggles the state and
  // records UMA.
  void SetBluetoothEnabled(bool enabled);

  // Returns whether the delegate has initiated a bluetooth discovery session.
  bool HasBluetoothDiscoverySession();

  // BluetoothAdapter::Observer:
  void AdapterPresentChanged(device::BluetoothAdapter* adapter,
                             bool present) override;
  void AdapterPoweredChanged(device::BluetoothAdapter* adapter,
                             bool powered) override;
  void AdapterDiscoveringChanged(device::BluetoothAdapter* adapter,
                                 bool discovering) override;
  void DeviceAdded(device::BluetoothAdapter* adapter,
                   device::BluetoothDevice* device) override;
  void DeviceChanged(device::BluetoothAdapter* adapter,
                     device::BluetoothDevice* device) override;
  void DeviceRemoved(device::BluetoothAdapter* adapter,
                     device::BluetoothDevice* device) override;

 private:
  void OnStartDiscoverySession(
      std::unique_ptr<device::BluetoothDiscoverySession> discovery_session);

  bool should_run_discovery_ = false;
  scoped_refptr<device::BluetoothAdapter> adapter_;
  std::unique_ptr<device::BluetoothDiscoverySession> discovery_session_;

  // Object could be deleted during a prolonged Bluetooth operation.
  base::WeakPtrFactory<TrayBluetoothHelper> weak_ptr_factory_;

  DISALLOW_COPY_AND_ASSIGN(TrayBluetoothHelper);
};

}  // namespace ash

#endif  // ASH_SYSTEM_BLUETOOTH_TRAY_BLUETOOTH_HELPER_H_
