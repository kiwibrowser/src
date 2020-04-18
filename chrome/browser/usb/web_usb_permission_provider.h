// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_USB_WEB_USB_PERMISSION_PROVIDER_H_
#define CHROME_BROWSER_USB_WEB_USB_PERMISSION_PROVIDER_H_

#include <stdint.h>

#include "base/memory/weak_ptr.h"
#include "device/usb/mojo/permission_provider.h"

namespace content {
class RenderFrameHost;
}

class GURL;
class UsbChooserContext;

// This implementation of the permission provider interface enforces the rules
// of the WebUSB permission model. Devices are checked for WebUSB descriptors
// granting access to the render frame's current origin as well as permission
// granted by the user through a device chooser UI.
class WebUSBPermissionProvider : public device::usb::PermissionProvider {
 public:
  static bool HasDevicePermission(
      UsbChooserContext* chooser_context,
      const GURL& requesting_origin,
      const GURL& embedding_origin,
      scoped_refptr<const device::UsbDevice> device);

  explicit WebUSBPermissionProvider(
      content::RenderFrameHost* render_frame_host);
  ~WebUSBPermissionProvider() override;

  base::WeakPtr<PermissionProvider> GetWeakPtr();

  // device::usb::PermissionProvider implementation.
  bool HasDevicePermission(
      scoped_refptr<const device::UsbDevice> device) const override;
  void IncrementConnectionCount() override;
  void DecrementConnectionCount() override;

 private:
  content::RenderFrameHost* const render_frame_host_;
  base::WeakPtrFactory<PermissionProvider> weak_factory_;
};

#endif  // CHROME_BROWSER_USB_WEB_USB_PERMISSION_PROVIDER_H_
