// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROMEOS_COMPONENTS_TETHER_BLE_ADVERTISER_H_
#define CHROMEOS_COMPONENTS_TETHER_BLE_ADVERTISER_H_

#include "base/macros.h"
#include "base/observer_list.h"
#include "components/cryptauth/remote_device_ref.h"

namespace chromeos {

namespace tether {

// Advertises to remote devices.
class BleAdvertiser {
 public:
  class Observer {
   public:
    virtual void OnAllAdvertisementsUnregistered() = 0;

   protected:
    virtual ~Observer() {}
  };

  BleAdvertiser();
  virtual ~BleAdvertiser();

  // Starts advertising to |remote_device| by generating a device-specific EID
  // and setting it as the service data for the advertisement. Returns whether
  // the advertisement could be generated.
  virtual bool StartAdvertisingToDevice(const std::string& device_id) = 0;

  // Stops advertising to |remote_device|. Returns whether the advertising was
  // stopped successfully.
  virtual bool StopAdvertisingToDevice(const std::string& device_id) = 0;

  // Returns whether there is currently an advertisement registered. Note that
  // registering and unregistering advertisements are asynchronous operations,
  // so this function can return true if StopAdvertisingToDevice() has been
  // called for all devices if a previous advertisement is in the process of
  // becoming unregistered.
  virtual bool AreAdvertisementsRegistered() = 0;

  void AddObserver(Observer* observer);
  void RemoveObserver(Observer* observer);

 protected:
  void NotifyAllAdvertisementsUnregistered();

 private:
  base::ObserverList<Observer> observer_list_;

  DISALLOW_COPY_AND_ASSIGN(BleAdvertiser);
};

}  // namespace tether

}  // namespace chromeos

#endif  // CHROMEOS_COMPONENTS_TETHER_BLE_ADVERTISER_H_
