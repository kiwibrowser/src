// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROMEOS_COMPONENTS_TETHER_AD_HOC_BLE_ADVERTISER_H_
#define CHROMEOS_COMPONENTS_TETHER_AD_HOC_BLE_ADVERTISER_H_

#include "base/macros.h"
#include "base/observer_list.h"

namespace chromeos {

namespace tether {

// Works around crbug.com/784968. This bug causes the tether host's GATT server
// to shut down incorrectly, leaving behind a "stale" advertisement which does
// not have any registered GATT services. This prevents a BLE connection from
// being created. When this situation occurs, AdHocBleAdvertiser can work
// around the issue by advertising to the device again.
//
// This class is *different* from BleAdvertiser because it advertises for an
// extended period of time, regardless of whether the device to which it is
// advertising is part of the BleAdvertisementDeviceQueue. The normal flow
// (i.e., BleConnectionManager and BleAdvertiser) stops advertising to a device
// once an advertisement is received from that same device; instead, this class
// keeps advertising regardless. This ensures that the remote device receives
// the advertisement.
class AdHocBleAdvertiser {
 public:
  class Observer {
   public:
    virtual void OnAsynchronousShutdownComplete() = 0;
    virtual ~Observer() {}
  };

  AdHocBleAdvertiser();
  virtual ~AdHocBleAdvertiser();

  // Requests that |remote_device| add GATT services. This should only be called
  // when the device has a stale advertisement with no GATT services. See
  // crbug.com/784968.
  virtual void RequestGattServicesForDevice(const std::string& device_id) = 0;

  // Returns whether there are any pending requests for GATT services.
  virtual bool HasPendingRequests() = 0;

  void AddObserver(Observer* observer);
  void RemoveObserver(Observer* observer);

 protected:
  void NotifyAsynchronousShutdownComplete();

 private:
  base::ObserverList<Observer> observer_list_;

  DISALLOW_COPY_AND_ASSIGN(AdHocBleAdvertiser);
};

}  // namespace tether

}  // namespace chromeos

#endif  // CHROMEOS_COMPONENTS_TETHER_AD_HOC_BLE_ADVERTISER_H_
