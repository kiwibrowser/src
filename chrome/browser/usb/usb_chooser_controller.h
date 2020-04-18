// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_USB_USB_CHOOSER_CONTROLLER_H_
#define CHROME_BROWSER_USB_USB_CHOOSER_CONTROLLER_H_

#include <unordered_map>
#include <utility>
#include <vector>

#include "base/macros.h"
#include "base/memory/ref_counted.h"
#include "base/memory/weak_ptr.h"
#include "base/scoped_observer.h"
#include "chrome/browser/chooser_controller/chooser_controller.h"
#include "device/usb/public/mojom/chooser_service.mojom.h"
#include "device/usb/usb_service.h"
#include "url/gurl.h"

namespace content {
class RenderFrameHost;
class WebContents;
}

namespace device {
class UsbDevice;
}

class UsbChooserContext;

// UsbChooserController creates a chooser for WebUSB.
// It is owned by ChooserBubbleDelegate.
class UsbChooserController : public ChooserController,
                             public device::UsbService::Observer {
 public:
  UsbChooserController(
      content::RenderFrameHost* render_frame_host,
      std::vector<device::mojom::UsbDeviceFilterPtr> device_filters,
      device::mojom::UsbChooserService::GetPermissionCallback callback);
  ~UsbChooserController() override;

  // ChooserController:
  base::string16 GetNoOptionsText() const override;
  base::string16 GetOkButtonLabel() const override;
  size_t NumOptions() const override;
  base::string16 GetOption(size_t index) const override;
  bool IsPaired(size_t index) const override;
  void Select(const std::vector<size_t>& indices) override;
  void Cancel() override;
  void Close() override;
  void OpenHelpCenterUrl() const override;

  // device::UsbService::Observer:
  void OnDeviceAdded(scoped_refptr<device::UsbDevice> device) override;
  void OnDeviceRemoved(scoped_refptr<device::UsbDevice> device) override;

 private:
  void GotUsbDeviceList(
      const std::vector<scoped_refptr<device::UsbDevice>>& devices);
  bool DisplayDevice(scoped_refptr<device::UsbDevice> device) const;

  std::vector<device::mojom::UsbDeviceFilterPtr> filters_;
  device::mojom::UsbChooserService::GetPermissionCallback callback_;
  GURL requesting_origin_;
  GURL embedding_origin_;

  content::WebContents* const web_contents_;
  base::WeakPtr<UsbChooserContext> chooser_context_;
  ScopedObserver<device::UsbService, device::UsbService::Observer>
      usb_service_observer_;

  // Each pair is a (device, device name).
  std::vector<std::pair<scoped_refptr<device::UsbDevice>, base::string16>>
      devices_;
  // Maps from device name to number of devices.
  std::unordered_map<base::string16, int> device_name_map_;
  base::WeakPtrFactory<UsbChooserController> weak_factory_;

  DISALLOW_COPY_AND_ASSIGN(UsbChooserController);
};

#endif  // CHROME_BROWSER_USB_USB_CHOOSER_CONTROLLER_H_
