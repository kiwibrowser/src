// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef DEVICE_USB_MOJO_PERMISSION_PROVIDER_H_
#define DEVICE_USB_MOJO_PERMISSION_PROVIDER_H_

#include "base/memory/ref_counted.h"

namespace device {

class UsbDevice;

namespace usb {

// An implementation of this interface must be provided to a DeviceManager in
// order to implement device permission checks.
class PermissionProvider {
 public:
  PermissionProvider();
  virtual ~PermissionProvider();

  virtual bool HasDevicePermission(
      scoped_refptr<const UsbDevice> device) const = 0;
  virtual void IncrementConnectionCount() = 0;
  virtual void DecrementConnectionCount() = 0;
};

}  // namespace usb
}  // namespace device

#endif  // DEVICE_USB_MOJO_PERMISSION_PROVIDER_H_
