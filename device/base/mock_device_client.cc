// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "device/base/mock_device_client.h"

#include "device/usb/mock_usb_service.h"

namespace device {

MockDeviceClient::MockDeviceClient() = default;

MockDeviceClient::~MockDeviceClient() = default;

UsbService* MockDeviceClient::GetUsbService() {
  return usb_service();
}

MockUsbService* MockDeviceClient::usb_service() {
  if (!usb_service_)
    usb_service_.reset(new MockUsbService());
  return usb_service_.get();
}

}  // namespace device
