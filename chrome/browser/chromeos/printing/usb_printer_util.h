// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Utility routines for working with UsbDevices that are printers.

#ifndef CHROME_BROWSER_CHROMEOS_PRINTING_USB_PRINTER_UTIL_H__
#define CHROME_BROWSER_CHROMEOS_PRINTING_USB_PRINTER_UTIL_H__

#include <memory>
#include <string>

#include "base/memory/ref_counted.h"

namespace device {
class UsbDevice;
}

namespace chromeos {

class Printer;

bool UsbDeviceIsPrinter(const device::UsbDevice& usb_device);

bool UsbDeviceSupportsIppusb(const device::UsbDevice& usb_device);

// Convert the interesting details of a device to a string, for
// logging/debugging.
std::string UsbPrinterDeviceDetailsAsString(const device::UsbDevice& device);

// Attempt to gather all the information we need to work with this printer by
// querying the USB device.  This should only be called using devices we believe
// are printers, not arbitrary USB devices, as we may get weird partial results
// from arbitrary devices.
//
// Returns nullptr and logs an error on failure.
std::unique_ptr<Printer> UsbDeviceToPrinter(const device::UsbDevice& device);

// Gets the URI CUPS would use to refer to this USB device.  Assumes device
// is a printer.
std::string UsbPrinterUri(const device::UsbDevice& device);

}  // namespace chromeos
#endif  // CHROME_BROWSER_CHROMEOS_PRINTING_USB_PRINTER_UTIL_H__
