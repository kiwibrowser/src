// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef DEVICE_BLUETOOTH_TEST_MOCK_BLUETOOTH_DISCOVERY_SESSION_H_
#define DEVICE_BLUETOOTH_TEST_MOCK_BLUETOOTH_DISCOVERY_SESSION_H_

#include <memory>

#include "base/callback.h"
#include "base/macros.h"
#include "device/bluetooth/bluetooth_discovery_session.h"
#include "testing/gmock/include/gmock/gmock.h"

namespace device {

class MockBluetoothDiscoverySession : public BluetoothDiscoverySession {
 public:
  MockBluetoothDiscoverySession();
  ~MockBluetoothDiscoverySession() override;

  MOCK_CONST_METHOD0(IsActive, bool());
  MOCK_METHOD2(Stop,
               void(const base::Closure& callback,
                    const ErrorCallback& error_callback));
  MOCK_METHOD3(SetDiscoveryFilterRaw,
               void(BluetoothDiscoveryFilter* discovery_filter,
                    const base::Closure& callback,
                    const ErrorCallback& error_callback));

 protected:
  void SetDiscoveryFilter(
      std::unique_ptr<BluetoothDiscoveryFilter> discovery_filter,
      const base::Closure& callback,
      const ErrorCallback& error_callback) override;

 private:
  DISALLOW_COPY_AND_ASSIGN(MockBluetoothDiscoverySession);
};

}  // namespac device

#endif  // DEVICE_BLUETOOTH_TEST_MOCK_BLUETOOTH_DISCOVERY_SESSION_H_
