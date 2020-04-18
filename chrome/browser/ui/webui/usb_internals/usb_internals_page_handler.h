// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_WEBUI_USB_INTERNALS_USB_INTERNALS_PAGE_HANDLER_H_
#define CHROME_BROWSER_UI_WEBUI_USB_INTERNALS_USB_INTERNALS_PAGE_HANDLER_H_

#include "base/macros.h"
#include "chrome/browser/ui/webui/usb_internals/usb_internals.mojom.h"
#include "mojo/public/cpp/bindings/binding.h"

class UsbInternalsPageHandler : public mojom::UsbInternalsPageHandler {
 public:
  explicit UsbInternalsPageHandler(
      mojom::UsbInternalsPageHandlerRequest request);
  ~UsbInternalsPageHandler() override;

  // mojom::UsbInternalsPageHandler overrides:
  void AddDeviceForTesting(const std::string& name,
                           const std::string& serial_number,
                           const std::string& landing_page,
                           AddDeviceForTestingCallback callback) override;
  void RemoveDeviceForTesting(const std::string& guid,
                              RemoveDeviceForTestingCallback callback) override;
  void GetTestDevices(GetTestDevicesCallback callback) override;

 private:
  mojo::Binding<mojom::UsbInternalsPageHandler> binding_;

  DISALLOW_COPY_AND_ASSIGN(UsbInternalsPageHandler);
};

#endif  // CHROME_BROWSER_UI_WEBUI_USB_INTERNALS_USB_INTERNALS_PAGE_HANDLER_H_
