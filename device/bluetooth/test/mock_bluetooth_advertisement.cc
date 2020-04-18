// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "device/bluetooth/test/mock_bluetooth_advertisement.h"

namespace device {

MockBluetoothAdvertisement::MockBluetoothAdvertisement() = default;

MockBluetoothAdvertisement::~MockBluetoothAdvertisement() = default;

void MockBluetoothAdvertisement::Unregister(
    const SuccessCallback& success_callback,
    const ErrorCallback& error_callback) {
  success_callback.Run();
}

}  // namespace device
