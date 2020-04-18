// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef DEVICE_TEST_TEST_DEVICE_CLIENT_H_
#define DEVICE_TEST_TEST_DEVICE_CLIENT_H_

#include <memory>

#include "build/build_config.h"
#include "device/base/device_client.h"

namespace device {

class UsbService;

class TestDeviceClient : public DeviceClient {
 public:
  TestDeviceClient();

  // Must be destroyed when tasks can still be posted to |task_runner|.
  ~TestDeviceClient() override;

  UsbService* GetUsbService() override;

 private:
  std::unique_ptr<UsbService> usb_service_;
};

}  // namespace device

#endif  // DEVICE_TEST_TEST_DEVICE_CLIENT_H_
