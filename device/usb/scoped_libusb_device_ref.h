// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef DEVICE_USB_SCOPED_LIBUSB_DEVICE_REF_H_
#define DEVICE_USB_SCOPED_LIBUSB_DEVICE_REF_H_

#include "base/scoped_generic.h"
#include "third_party/libusb/src/libusb/libusb.h"

namespace device {

struct LibusbDeviceRefTraits {
  static libusb_device* InvalidValue() { return nullptr; }

  static void Free(libusb_device* device) { libusb_unref_device(device); }
};

using ScopedLibusbDeviceRef =
    base::ScopedGeneric<libusb_device*, LibusbDeviceRefTraits>;

}  // namespace device

#endif  // DEVICE_USB_SCOPED_LIBUSB_DEVICE_REF_H_
