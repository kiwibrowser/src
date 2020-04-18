// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef DEVICE_BASE_MOCK_DEVICE_CLIENT_H_
#define DEVICE_BASE_MOCK_DEVICE_CLIENT_H_

#include <memory>

#include "build/build_config.h"
#include "device/base/device_client.h"

namespace device {

class MockUsbService;

class MockDeviceClient : device::DeviceClient {
 public:
  MockDeviceClient();
  ~MockDeviceClient() override;

  // device::DeviceClient implementation:
  UsbService* GetUsbService() override;

  // Accessors for the mock instances.
  MockUsbService* usb_service();

 private:
  std::unique_ptr<MockUsbService> usb_service_;
};

}  // namespace device

#endif  // DEVICE_BASE_MOCK_DEVICE_CLIENT_H_
