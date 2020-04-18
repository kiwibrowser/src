// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef DEVICE_USB_MOCK_USB_DEVICE_H_
#define DEVICE_USB_MOCK_USB_DEVICE_H_

#include <stdint.h>

#include <string>
#include <vector>

#include "device/usb/usb_device.h"
#include "device/usb/usb_device_handle.h"
#include "testing/gmock/include/gmock/gmock.h"

namespace device {

class MockUsbDevice : public UsbDevice {
 public:
  MockUsbDevice(uint16_t vendor_id, uint16_t product_id);
  MockUsbDevice(uint16_t vendor_id,
                uint16_t product_id,
                const std::string& manufacturer_string,
                const std::string& product_string,
                const std::string& serial_number);
  MockUsbDevice(uint16_t vendor_id,
                uint16_t product_id,
                const std::string& manufacturer_string,
                const std::string& product_string,
                const std::string& serial_number,
                const GURL& webusb_landing_page);
  MockUsbDevice(uint16_t vendor_id,
                uint16_t product_id,
                const UsbConfigDescriptor& configuration);
  MockUsbDevice(uint16_t vendor_id,
                uint16_t product_id,
                uint8_t device_class,
                const std::vector<UsbConfigDescriptor>& configurations);
  MockUsbDevice(uint16_t vendor_id,
                uint16_t product_id,
                const std::string& manufacturer_string,
                const std::string& product_string,
                const std::string& serial_number,
                const std::vector<UsbConfigDescriptor>& configurations);

  void Open(OpenCallback callback) override { OpenInternal(callback); }
  MOCK_METHOD1(OpenInternal, void(OpenCallback&));

  void AddMockConfig(const UsbConfigDescriptor& config);

  // Public wrappers around protected functions.
  void ActiveConfigurationChanged(int configuration_value);
  void NotifyDeviceRemoved();

 private:
  ~MockUsbDevice() override;
};

}  // namespace device

#endif  // DEVICE_USB_MOCK_USB_DEVICE_H_
