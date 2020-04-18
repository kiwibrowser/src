// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_ANDROID_USB_WEB_USB_CHOOSER_SERVICE_ANDROID_H_
#define CHROME_BROWSER_ANDROID_USB_WEB_USB_CHOOSER_SERVICE_ANDROID_H_

#include <memory>
#include <vector>

#include "base/macros.h"
#include "chrome/browser/usb/web_usb_chooser_service.h"

class UsbChooserController;
class UsbChooserDialogAndroid;

// Implementation of the public device::usb::ChooserService interface.
// This interface can be used by a webpage to request permission from user
// to access a certain device.
class WebUsbChooserServiceAndroid : public WebUsbChooserService {
 public:
  explicit WebUsbChooserServiceAndroid(
      content::RenderFrameHost* render_frame_host);
  ~WebUsbChooserServiceAndroid() override;

  // WebUsbChooserService implementation
  void ShowChooser(std::unique_ptr<UsbChooserController> controller) override;

 private:
  void OnDialogClosed();

  // Only a single dialog can be shown at a time.
  std::unique_ptr<UsbChooserDialogAndroid> dialog_;

  DISALLOW_COPY_AND_ASSIGN(WebUsbChooserServiceAndroid);
};

#endif  // CHROME_BROWSER_ANDROID_USB_WEB_USB_CHOOSER_SERVICE_ANDROID_H_
