// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/shell/browser/layout_test/fake_bluetooth_chooser.h"

#include <string>
#include <utility>

#include "content/public/browser/bluetooth_chooser.h"
#include "content/public/test/layouttest_support.h"
#include "content/shell/common/layout_test/fake_bluetooth_chooser.mojom.h"

namespace content {

FakeBluetoothChooser::~FakeBluetoothChooser() {
  SetTestBluetoothScanDuration(
      BluetoothTestScanDurationSetting::kImmediateTimeout);
}

// static
std::unique_ptr<FakeBluetoothChooser> FakeBluetoothChooser::Create(
    mojom::FakeBluetoothChooserRequest request) {
  SetTestBluetoothScanDuration(BluetoothTestScanDurationSetting::kNeverTimeout);
  return std::unique_ptr<FakeBluetoothChooser>(
      new FakeBluetoothChooser(std::move(request)));
}

void FakeBluetoothChooser::SetEventHandler(const EventHandler& event_handler) {
  event_handler_ = event_handler;
}

// mojom::FakeBluetoothChooser overrides

void FakeBluetoothChooser::WaitForEvents(uint32_t num_of_events,
                                         WaitForEventsCallback callback) {
  // TODO(https://crbug.com/719826): Implement this function according to the
  // Web Bluetooth Test Scanning design document.
  // https://docs.google.com/document/d/1XFl_4ZAgO8ddM6U53A9AfUuZeWgJnlYD5wtbXqEpzeg
  NOTREACHED();
}

void FakeBluetoothChooser::SelectPeripheral(
    const std::string& peripheral_address,
    SelectPeripheralCallback callback) {
  // TODO(https://crbug.com/719826): Record the event and send a
  // BluetoothChooser::SELECTED event to |event_handler_|.
  NOTREACHED();
}

void FakeBluetoothChooser::Cancel(CancelCallback callback) {
  // TODO(https://crbug.com/719826): Record the event and send a
  // BluetoothChooser::CANCELLED event to |event_handler_|.
  NOTREACHED();
}

void FakeBluetoothChooser::Rescan(RescanCallback callback) {
  // TODO(https://crbug.com/719826): Record the event and send a
  // BluetoothChooser::RESCAN event to |event_handler_|.
  NOTREACHED();
}

// BluetoothChooser overrides

void FakeBluetoothChooser::SetAdapterPresence(AdapterPresence presence) {
  // TODO(https://crbug.com/719826): Record the event.
  NOTREACHED();
}

void FakeBluetoothChooser::ShowDiscoveryState(DiscoveryState state) {
  // TODO(https://crbug.com/719826): Record the event.
  NOTREACHED();
}

void FakeBluetoothChooser::AddOrUpdateDevice(const std::string& device_id,
                                             bool should_update_name,
                                             const base::string16& device_name,
                                             bool is_gatt_connected,
                                             bool is_paired,
                                             int signal_strength_level) {
  // TODO(https://crbug.com/719826): Record the event.
  NOTREACHED();
}

// private

FakeBluetoothChooser::FakeBluetoothChooser(
    mojom::FakeBluetoothChooserRequest request)
    : binding_(this, std::move(request)) {}

}  // namespace content
