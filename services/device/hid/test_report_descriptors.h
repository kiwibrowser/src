// Copyright (c) 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SERVICES_DEVICE_HID_TEST_REPORT_DESCRIPTORS_H_
#define SERVICES_DEVICE_HID_TEST_REPORT_DESCRIPTORS_H_

#include <stddef.h>
#include <stdint.h>

namespace device {

// Digitizer descriptor from HID descriptor tool
// http://www.usb.org/developers/hidpage/dt2_4.zip
extern const uint8_t kDigitizer[];
extern const size_t kDigitizerSize;

// Keyboard descriptor from HID descriptor tool
// http://www.usb.org/developers/hidpage/dt2_4.zip
extern const uint8_t kKeyboard[];
extern const size_t kKeyboardSize;

// Monitor descriptor from HID descriptor tool
// http://www.usb.org/developers/hidpage/dt2_4.zip
extern const uint8_t kMonitor[];
extern const size_t kMonitorSize;

// Mouse descriptor from HID descriptor tool
// http://www.usb.org/developers/hidpage/dt2_4.zip
extern const uint8_t kMouse[];
extern const size_t kMouseSize;

// Logitech Unifying receiver descriptor
extern const uint8_t kLogitechUnifyingReceiver[];
extern const size_t kLogitechUnifyingReceiverSize;

}  // namespace device

#endif  // SERVICES_DEVICE_HID_TEST_REPORT_DESCRIPTORS_H_
