// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "extensions/shell/browser/shell_device_client.h"

#include "content/public/browser/browser_thread.h"
#include "device/usb/usb_service.h"

using content::BrowserThread;

namespace extensions {

ShellDeviceClient::ShellDeviceClient() = default;

ShellDeviceClient::~ShellDeviceClient() = default;

device::UsbService* ShellDeviceClient::GetUsbService() {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  if (!usb_service_)
    usb_service_ = device::UsbService::Create();
  return usb_service_.get();
}

}  // namespace extensions
