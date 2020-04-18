// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "device/test/test_device_client.h"

#include "base/single_thread_task_runner.h"
#include "device/usb/usb_service.h"

namespace device {

TestDeviceClient::TestDeviceClient() = default;

TestDeviceClient::~TestDeviceClient() = default;

UsbService* TestDeviceClient::GetUsbService() {
  if (!usb_service_)
    usb_service_ = UsbService::Create();
  return usb_service_.get();
}

}  // namespace device
