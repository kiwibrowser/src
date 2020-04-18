// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "device/usb/public/cpp/filter_utils.h"

#include "device/usb/usb_device.h"

namespace device {

bool UsbDeviceFilterMatches(const mojom::UsbDeviceFilter& filter,
                            const UsbDevice& device) {
  if (filter.has_vendor_id) {
    if (device.vendor_id() != filter.vendor_id)
      return false;

    if (filter.has_product_id && device.product_id() != filter.product_id)
      return false;
  }

  if (filter.serial_number && device.serial_number() != *filter.serial_number)
    return false;

  if (filter.has_class_code) {
    for (const UsbConfigDescriptor& config : device.configurations()) {
      for (const UsbInterfaceDescriptor& iface : config.interfaces) {
        if (iface.interface_class == filter.class_code &&
            (!filter.has_subclass_code ||
             (iface.interface_subclass == filter.subclass_code &&
              (!filter.has_protocol_code ||
               iface.interface_protocol == filter.protocol_code)))) {
          return true;
        }
      }
    }

    return false;
  }

  return true;
}

bool UsbDeviceFilterMatchesAny(
    const std::vector<mojom::UsbDeviceFilterPtr>& filters,
    const UsbDevice& device) {
  if (filters.empty())
    return true;

  for (const auto& filter : filters) {
    if (UsbDeviceFilterMatches(*filter, device))
      return true;
  }
  return false;
}

}  // namespace device
