// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chromeos/components/tether/fake_error_tolerant_ble_advertisement.h"

#include "base/bind.h"

namespace chromeos {

namespace tether {

FakeErrorTolerantBleAdvertisement::FakeErrorTolerantBleAdvertisement(
    const std::string& device_id)
    : ErrorTolerantBleAdvertisement(device_id) {}

FakeErrorTolerantBleAdvertisement::FakeErrorTolerantBleAdvertisement(
    const std::string& device_id,
    const base::Callback<void(FakeErrorTolerantBleAdvertisement*)>&
        deletion_callback)
    : ErrorTolerantBleAdvertisement(device_id),
      deletion_callback_(deletion_callback) {}

FakeErrorTolerantBleAdvertisement::~FakeErrorTolerantBleAdvertisement() {
  if (!deletion_callback_.is_null())
    deletion_callback_.Run(this);
}

bool FakeErrorTolerantBleAdvertisement::HasBeenStopped() {
  return !stop_callback_.is_null();
}

void FakeErrorTolerantBleAdvertisement::InvokeStopCallback() {
  DCHECK(HasBeenStopped());
  stop_callback_.Run();
}

void FakeErrorTolerantBleAdvertisement::Stop(const base::Closure& callback) {
  DCHECK(!HasBeenStopped());
  stop_callback_ = callback;
}

}  // namespace tether

}  // namespace chromeos
