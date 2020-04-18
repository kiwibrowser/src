// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef DEVICE_USB_MOJO_MOCK_PERMISSION_PROVIDER_H_
#define DEVICE_USB_MOJO_MOCK_PERMISSION_PROVIDER_H_

#include <stdint.h>

#include "base/memory/weak_ptr.h"
#include "device/usb/mojo/permission_provider.h"
#include "device/usb/usb_device.h"
#include "testing/gmock/include/gmock/gmock.h"

namespace device {
namespace usb {

class MockPermissionProvider : public PermissionProvider {
 public:
  MockPermissionProvider();
  ~MockPermissionProvider() override;

  base::WeakPtr<PermissionProvider> GetWeakPtr();
  MOCK_CONST_METHOD1(HasDevicePermission,
                     bool(scoped_refptr<const UsbDevice> device));

  MOCK_METHOD0(IncrementConnectionCount, void());
  MOCK_METHOD0(DecrementConnectionCount, void());

 private:
  base::WeakPtrFactory<PermissionProvider> weak_factory_;
};

}  // namespace usb
}  // namespace device

#endif  // DEVICE_USB_MOCK_MOJO_PERMISSION_PROVIDER_H_
