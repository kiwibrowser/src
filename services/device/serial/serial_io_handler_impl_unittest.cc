// Copyright (c) 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "services/device/serial/serial_io_handler_impl.h"

#include "base/macros.h"
#include "mojo/public/cpp/bindings/interface_ptr.h"
#include "services/device/device_service_test_base.h"
#include "services/device/public/mojom/constants.mojom.h"
#include "services/device/public/mojom/serial.mojom.h"

namespace device {

namespace {

class SerialIoHandlerImplTest : public DeviceServiceTestBase {
 public:
  SerialIoHandlerImplTest() = default;
  ~SerialIoHandlerImplTest() override = default;

 private:
  DISALLOW_COPY_AND_ASSIGN(SerialIoHandlerImplTest);
};

// This is to simply test that on Linux/Mac/Windows a client can connect to
// Device Service and bind the serial SerialIoHandler interface
// correctly.
// TODO(leonhsl): figure out how to add more robust tests.
TEST_F(SerialIoHandlerImplTest, SimpleConnectTest) {
  mojom::SerialIoHandlerPtr io_handler;
  connector()->BindInterface(mojom::kServiceName, &io_handler);
  io_handler.FlushForTesting();
}

}  // namespace

}  // namespace device
