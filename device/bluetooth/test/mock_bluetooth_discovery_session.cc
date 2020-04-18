// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "device/bluetooth/test/mock_bluetooth_discovery_session.h"

#include <memory>

#include "device/bluetooth/test/mock_bluetooth_adapter.h"

namespace device {

// Note: Because |this| class mocks out all the interesting method calls, the
// mock BluetoothAdapter will not be used, except for a trivial call from the
// destructor. It's passed in simply because the base class expects one, and
// it's nice not to need to complicate production code for the sake of simpler
// test code.
MockBluetoothDiscoverySession::MockBluetoothDiscoverySession()
    : BluetoothDiscoverySession(
          scoped_refptr<BluetoothAdapter>(
              new testing::NiceMock<MockBluetoothAdapter>()),
          nullptr) {
}
MockBluetoothDiscoverySession::~MockBluetoothDiscoverySession() = default;

void MockBluetoothDiscoverySession::SetDiscoveryFilter(
    std::unique_ptr<BluetoothDiscoveryFilter> discovery_filter,
    const base::Closure& callback,
    const ErrorCallback& error_callback) {
  SetDiscoveryFilterRaw(discovery_filter.get(), callback, error_callback);
}

}  // namespace device
