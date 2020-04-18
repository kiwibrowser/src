// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_USB_WEB_USB_HISTOGRAMS_H_
#define CHROME_BROWSER_USB_WEB_USB_HISTOGRAMS_H_

// Reasons the chooser may be closed. These are used in histograms so do not
// remove/reorder entries. Only add at the end just before
// WEBUSB_CHOOSER_CLOSED_MAX. Also remember to update the enum listing in
// tools/metrics/histograms/histograms.xml.
enum WebUsbChooserClosed {
  // The user cancelled the permission prompt without selecting a device.
  WEBUSB_CHOOSER_CLOSED_CANCELLED = 0,
  // The user probably cancelled the permission prompt without selecting a
  // device because there were no devices to select.
  WEBUSB_CHOOSER_CLOSED_CANCELLED_NO_DEVICES,
  // The user granted permission to access a device.
  WEBUSB_CHOOSER_CLOSED_PERMISSION_GRANTED,
  // The user granted permission to access a device but that permission will be
  // revoked when the device is disconnected.
  WEBUSB_CHOOSER_CLOSED_EPHEMERAL_PERMISSION_GRANTED,
  // Maximum value for the enum.
  WEBUSB_CHOOSER_CLOSED_MAX
};

void RecordWebUsbChooserClosure(WebUsbChooserClosed disposition);

#endif  // CHROME_BROWSER_USB_WEB_USB_HISTOGRAMS_H_
