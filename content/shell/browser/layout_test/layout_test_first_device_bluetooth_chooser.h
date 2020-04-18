// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_SHELL_BROWSER_LAYOUT_TEST_LAYOUT_TEST_FIRST_DEVICE_BLUETOOTH_CHOOSER_H_
#define CONTENT_SHELL_BROWSER_LAYOUT_TEST_LAYOUT_TEST_FIRST_DEVICE_BLUETOOTH_CHOOSER_H_

#include <string>

#include "base/macros.h"
#include "content/public/browser/bluetooth_chooser.h"

namespace content {

// Implements a Bluetooth chooser that selects the first added device, or
// cancels if no device is added before discovery stops. This is used as a
// default chooser implementation for testing.
class LayoutTestFirstDeviceBluetoothChooser : public BluetoothChooser {
 public:
  // See the BluetoothChooser::EventHandler comments for how |event_handler| is
  // used.
  explicit LayoutTestFirstDeviceBluetoothChooser(
      const EventHandler& event_handler);
  ~LayoutTestFirstDeviceBluetoothChooser() override;

  // BluetoothChooser:
  void SetAdapterPresence(AdapterPresence presence) override;
  void ShowDiscoveryState(DiscoveryState state) override;
  void AddOrUpdateDevice(const std::string& device_id,
                         bool should_update_name,
                         const base::string16& device_name,
                         bool is_gatt_connected,
                         bool is_paired,
                         int signal_strength_level) override;

 private:
  EventHandler event_handler_;

  DISALLOW_COPY_AND_ASSIGN(LayoutTestFirstDeviceBluetoothChooser);
};

}  // namespace content

#endif  // CONTENT_SHELL_BROWSER_LAYOUT_TEST_LAYOUT_TEST_FIRST_DEVICE_BLUETOOTH_CHOOSER_H_
