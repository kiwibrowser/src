// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef EXTENSIONS_SHELL_BROWSER_SHELL_DEVICE_CLIENT_H_
#define EXTENSIONS_SHELL_BROWSER_SHELL_DEVICE_CLIENT_H_

#include <memory>

#include "base/macros.h"
#include "device/base/device_client.h"

namespace extensions {

// Implementation of device::DeviceClient that returns //device API service
// singletons appropriate for use within app shell.
class ShellDeviceClient : device::DeviceClient {
 public:
  ShellDeviceClient();
  ~ShellDeviceClient() override;

  // device::DeviceClient implementation
  device::UsbService* GetUsbService() override;

 private:
  std::unique_ptr<device::UsbService> usb_service_;

  DISALLOW_COPY_AND_ASSIGN(ShellDeviceClient);
};

}

#endif  // EXTENSIONS_SHELL_BROWSER_SHELL_DEVICE_CLIENT_H_
