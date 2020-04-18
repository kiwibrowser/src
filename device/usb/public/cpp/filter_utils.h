// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef DEVICE_USB_PUBLIC_CPP_FILTER_UTILS_H_
#define DEVICE_USB_PUBLIC_CPP_FILTER_UTILS_H_

#include <vector>

#include "device/usb/public/mojom/device_manager.mojom.h"

namespace device {

class UsbDevice;

bool UsbDeviceFilterMatches(const mojom::UsbDeviceFilter& filter,
                            const UsbDevice& device);

bool UsbDeviceFilterMatchesAny(
    const std::vector<mojom::UsbDeviceFilterPtr>& filters,
    const UsbDevice& device);

}  // namespace device

#endif  // DEVICE_USB_PUBLIC_CPP_FILTER_UTILS_H_
