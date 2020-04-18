// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROMEOS_COMPONENTS_FAKE_ERROR_TOLERANT_BLE_ADVERTISEMENT_H_
#define CHROMEOS_COMPONENTS_FAKE_ERROR_TOLERANT_BLE_ADVERTISEMENT_H_

#include "base/callback.h"
#include "base/macros.h"
#include "chromeos/components/tether/error_tolerant_ble_advertisement.h"

namespace chromeos {

namespace tether {

// Test double for ErrorTolerantBleAdvertisement.
class FakeErrorTolerantBleAdvertisement : public ErrorTolerantBleAdvertisement {
 public:
  FakeErrorTolerantBleAdvertisement(const std::string& device_id);
  FakeErrorTolerantBleAdvertisement(
      const std::string& device_id,
      const base::Callback<void(FakeErrorTolerantBleAdvertisement*)>&
          deletion_callback);
  ~FakeErrorTolerantBleAdvertisement() override;

  void InvokeStopCallback();

  // ErrorTolerantBleAdvertisement:
  void Stop(const base::Closure& callback) override;
  bool HasBeenStopped() override;

 private:
  const base::Callback<void(FakeErrorTolerantBleAdvertisement*)>
      deletion_callback_;
  base::Closure stop_callback_;

  DISALLOW_COPY_AND_ASSIGN(FakeErrorTolerantBleAdvertisement);
};

}  // namespace tether

}  // namespace chromeos

#endif  // CHROMEOS_COMPONENTS_FAKE_ERROR_TOLERANT_BLE_ADVERTISEMENT_H_
