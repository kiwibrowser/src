// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_USB_WEB_USB_CHOOSER_SERVICE_DESKTOP_H_
#define CHROME_BROWSER_USB_WEB_USB_CHOOSER_SERVICE_DESKTOP_H_

#include "base/macros.h"
#include "chrome/browser/usb/web_usb_chooser_service.h"
#include "components/bubble/bubble_reference.h"

// Implementation of WebUsbChooserService for desktop browsers that uses a
// bubble to display the permission prompt.
class WebUsbChooserServiceDesktop : public WebUsbChooserService {
 public:
  explicit WebUsbChooserServiceDesktop(
      content::RenderFrameHost* render_frame_host);
  ~WebUsbChooserServiceDesktop() override;

  // WebUsbChooserServiceDesktop implementation
  void ShowChooser(std::unique_ptr<UsbChooserController> controller) override;

 private:
  BubbleReference bubble_;

  DISALLOW_COPY_AND_ASSIGN(WebUsbChooserServiceDesktop);
};

#endif  // CHROME_BROWSER_USB_WEB_USB_CHOOSER_SERVICE_DESKTOP_H_
