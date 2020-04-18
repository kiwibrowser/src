// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "device/usb/mock_usb_device.h"

#include "base/strings/utf_string_conversions.h"

namespace device {

MockUsbDevice::MockUsbDevice(uint16_t vendor_id, uint16_t product_id)
    : MockUsbDevice(vendor_id, product_id, "", "", "") {}

MockUsbDevice::MockUsbDevice(uint16_t vendor_id,
                             uint16_t product_id,
                             const std::string& manufacturer_string,
                             const std::string& product_string,
                             const std::string& serial_number)
    : UsbDevice(0x0200,  // usb_version
                0xff,    // device_class
                0xff,    // device_subclass
                0xff,    // device_protocol
                vendor_id,
                product_id,
                0x0100,  // device_version
                base::UTF8ToUTF16(manufacturer_string),
                base::UTF8ToUTF16(product_string),
                base::UTF8ToUTF16(serial_number)) {}

MockUsbDevice::MockUsbDevice(uint16_t vendor_id,
                             uint16_t product_id,
                             const std::string& manufacturer_string,
                             const std::string& product_string,
                             const std::string& serial_number,
                             const GURL& webusb_landing_page)
    : UsbDevice(0x0200,  // usb_version
                0xff,    // device_class
                0xff,    // device_subclass
                0xff,    // device_protocol
                vendor_id,
                product_id,
                0x0100,  // device_version
                base::UTF8ToUTF16(manufacturer_string),
                base::UTF8ToUTF16(product_string),
                base::UTF8ToUTF16(serial_number)) {
  webusb_landing_page_ = webusb_landing_page;
}

MockUsbDevice::MockUsbDevice(uint16_t vendor_id,
                             uint16_t product_id,
                             const UsbConfigDescriptor& configuration)
    : MockUsbDevice(vendor_id, product_id) {
  descriptor_.configurations.push_back(configuration);
}

MockUsbDevice::MockUsbDevice(
    uint16_t vendor_id,
    uint16_t product_id,
    uint8_t device_class,
    const std::vector<UsbConfigDescriptor>& configurations)
    : UsbDevice(0x0200,  // usb_version
                device_class,
                0xff,  // device_subclass
                0xff,  // device_protocol
                vendor_id,
                product_id,
                0x0100,  // device_version
                base::string16(),
                base::string16(),
                base::string16()) {
  descriptor_.configurations = configurations;
}

MockUsbDevice::MockUsbDevice(
    uint16_t vendor_id,
    uint16_t product_id,
    const std::string& manufacturer_string,
    const std::string& product_string,
    const std::string& serial_number,
    const std::vector<UsbConfigDescriptor>& configurations)
    : MockUsbDevice(vendor_id,
                    product_id,
                    manufacturer_string,
                    product_string,
                    serial_number) {
  descriptor_.configurations = configurations;
}

MockUsbDevice::~MockUsbDevice() = default;

void MockUsbDevice::AddMockConfig(const UsbConfigDescriptor& config) {
  descriptor_.configurations.push_back(config);
}

void MockUsbDevice::ActiveConfigurationChanged(int configuration_value) {
  UsbDevice::ActiveConfigurationChanged(configuration_value);
}

void MockUsbDevice::NotifyDeviceRemoved() {
  UsbDevice::NotifyDeviceRemoved();
}

}  // namespace device
