// Copyright (c) 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "services/device/serial/serial_device_enumerator_impl.h"

#include "base/macros.h"
#include "mojo/public/cpp/bindings/interface_ptr.h"
#include "services/device/device_service_test_base.h"
#include "services/device/public/mojom/constants.mojom.h"
#include "services/device/public/mojom/serial.mojom.h"

namespace device {

namespace {

class SerialDeviceEnumeratorImplTest : public DeviceServiceTestBase {
 public:
  SerialDeviceEnumeratorImplTest() = default;
  ~SerialDeviceEnumeratorImplTest() override = default;

 protected:
  void SetUp() override {
    DeviceServiceTestBase::SetUp();
    connector()->BindInterface(mojom::kServiceName, &enumerator_);
  }

  void TearDown() override { enumerator_.reset(); }

  mojom::SerialDeviceEnumeratorPtr enumerator_;

  DISALLOW_COPY_AND_ASSIGN(SerialDeviceEnumeratorImplTest);
};

// This is to simply test that on Linux/Mac/Windows a client can connect to
// Device Service and bind the serial SerialDeviceEnumerator interface
// correctly.
// TODO(leonhsl): figure out how to add more robust tests.
TEST_F(SerialDeviceEnumeratorImplTest, SimpleConnectTest) {
  enumerator_.FlushForTesting();
}

}  // namespace

}  // namespace device
