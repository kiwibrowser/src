// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROMEOS_COMPONENTS_TETHER_BLE_SCANNER_H_
#define CHROMEOS_COMPONENTS_TETHER_BLE_SCANNER_H_

#include "base/macros.h"
#include "base/observer_list.h"
#include "components/cryptauth/remote_device_ref.h"

namespace device {
class BluetoothDevice;
}

namespace chromeos {

namespace tether {

// Performs BLE scans for devices which are advertising to this device.
class BleScanner {
 public:
  class Observer {
   public:
    virtual void OnReceivedAdvertisementFromDevice(
        cryptauth::RemoteDeviceRef remote_device,
        device::BluetoothDevice* bluetooth_device,
        bool is_background_advertisement) {}
    virtual void OnDiscoverySessionStateChanged(bool discovery_session_active) {
    }
  };

  BleScanner();
  virtual ~BleScanner();

  // Registers a scan filter for the given device. The scan filter will remain
  // active until a subsequent call to UnregisterScanFilterForDevice() is made.
  // Returns whether the scan filter was successfully registered.
  virtual bool RegisterScanFilterForDevice(const std::string& device_id) = 0;

  // Unregisters a scan filter for |device|. Returns whether the scan filter was
  // successfully unregistered.
  virtual bool UnregisterScanFilterForDevice(const std::string& device_id) = 0;

  // A discovery session should be active if at least one device has been
  // registered. However, discovery sessions are started and stopped
  // asynchronously, so these two functions may return different values.
  virtual bool ShouldDiscoverySessionBeActive() = 0;
  virtual bool IsDiscoverySessionActive() = 0;

  void AddObserver(Observer* observer);
  void RemoveObserver(Observer* observer);

 protected:
  void NotifyReceivedAdvertisementFromDevice(
      cryptauth::RemoteDeviceRef remote_device,
      device::BluetoothDevice* bluetooth_device,
      bool is_background_advertisement);
  void NotifyDiscoverySessionStateChanged(bool discovery_session_active);

 private:
  base::ObserverList<Observer> observer_list_;

  DISALLOW_COPY_AND_ASSIGN(BleScanner);
};

}  // namespace tether

}  // namespace chromeos

#endif  // CHROMEOS_COMPONENTS_TETHER_BLE_SCANNER_H_
