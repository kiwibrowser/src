// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROMEOS_COMPONENTS_ERROR_TOLERANT_BLE_ADVERTISEMENT_H_
#define CHROMEOS_COMPONENTS_ERROR_TOLERANT_BLE_ADVERTISEMENT_H_

#include "base/callback_forward.h"
#include "base/macros.h"
#include "base/memory/weak_ptr.h"

namespace chromeos {

namespace tether {

// Advertises to the device with the given ID. Due to issues in the Bluetooth
// stack, it is possible that registering or unregistering an advertisement can
// fail. If this class encounters an error, it retries until it succeeds. Once
// Stop() is called, the advertisement should not be considered unregistered
// until the stop callback is invoked.
class ErrorTolerantBleAdvertisement {
 public:
  ErrorTolerantBleAdvertisement(const std::string& device_id);
  virtual ~ErrorTolerantBleAdvertisement();

  // Stops advertising. Because BLE advertisements start and stop
  // asynchronously, clients must use this function to stop advertising instead
  // of simply deleting an ErrorTolerantBleAdvertisement object. Clients should
  // not assume that advertising has actually stopped until |callback| has been
  // invoked.
  virtual void Stop(const base::Closure& callback) = 0;

  // Returns whether Stop() has been called.
  virtual bool HasBeenStopped() = 0;

  const std::string& device_id() { return device_id_; }

 private:
  const std::string device_id_;

  DISALLOW_COPY_AND_ASSIGN(ErrorTolerantBleAdvertisement);
};

}  // namespace tether

}  // namespace chromeos

#endif  // CHROMEOS_COMPONENTS_ERROR_TOLERANT_BLE_ADVERTISEMENT_H_
