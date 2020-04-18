// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_USB_WEB_USB_DETECTOR_H_
#define CHROME_BROWSER_USB_WEB_USB_DETECTOR_H_

#include "base/macros.h"
#include "base/scoped_observer.h"
#include "device/usb/usb_service.h"

namespace device {
class UsbDevice;
}

class WebUsbDetector : public device::UsbService::Observer {
 public:
  WebUsbDetector();
  ~WebUsbDetector() override;

  // Initializes the WebUsbDetector.
  void Initialize();

 private:
  // device::UsbService::observer:
  void OnDeviceAdded(scoped_refptr<device::UsbDevice> device) override;
  void OnDeviceRemoved(scoped_refptr<device::UsbDevice> device) override;

  ScopedObserver<device::UsbService, device::UsbService::Observer> observer_;

  DISALLOW_COPY_AND_ASSIGN(WebUsbDetector);
};

#endif  // CHROME_BROWSER_USB_WEB_USB_DETECTOR_H_
