// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef EXTENSIONS_BROWSER_API_USB_USB_DEVICE_RESOURCE_H_
#define EXTENSIONS_BROWSER_API_USB_USB_DEVICE_RESOURCE_H_

#include <string>

#include "base/macros.h"
#include "base/memory/ref_counted.h"
#include "base/synchronization/lock.h"
#include "content/public/browser/browser_thread.h"
#include "device/usb/usb_device_handle.h"
#include "extensions/browser/api/api_resource.h"
#include "extensions/browser/api/api_resource_manager.h"

namespace usb_service {
class UsbDeviceHandle;
}

namespace extensions {

// A UsbDeviceResource is an ApiResource wrapper for a UsbDevice.
class UsbDeviceResource : public ApiResource {
 public:
  static const content::BrowserThread::ID kThreadId =
      content::BrowserThread::UI;

  UsbDeviceResource(const std::string& owner_extension_id,
                    scoped_refptr<device::UsbDeviceHandle> device);
  ~UsbDeviceResource() override;

  scoped_refptr<device::UsbDeviceHandle> device() { return device_; }

  bool IsPersistent() const override;

 private:
  friend class ApiResourceManager<UsbDeviceResource>;
  static const char* service_name() { return "UsbDeviceResourceManager"; }

  scoped_refptr<device::UsbDeviceHandle> device_;

  DISALLOW_COPY_AND_ASSIGN(UsbDeviceResource);
};

}  // namespace extensions

#endif  // EXTENSIONS_BROWSER_API_USB_USB_DEVICE_RESOURCE_H_
