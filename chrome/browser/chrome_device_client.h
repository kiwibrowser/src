// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_CHROME_DEVICE_CLIENT_H_
#define CHROME_BROWSER_CHROME_DEVICE_CLIENT_H_

#include "device/base/device_client.h"

#include <memory>

#include "base/macros.h"
#include "build/build_config.h"

// Implementation of device::DeviceClient that returns //device service
// singletons appropriate for use within the Chrome application.
class ChromeDeviceClient : device::DeviceClient {
 public:
  ChromeDeviceClient();
  ~ChromeDeviceClient() override;

  // device::DeviceClient implementation
  device::UsbService* GetUsbService() override;

 private:
  std::unique_ptr<device::UsbService> usb_service_;

  DISALLOW_COPY_AND_ASSIGN(ChromeDeviceClient);
};

#endif  // CHROME_BROWSER_CHROME_DEVICE_CLIENT_H_
