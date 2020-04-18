// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/bluetooth/bluetooth_chooser_desktop.h"

#include "base/logging.h"
#include "chrome/browser/ui/bluetooth/bluetooth_chooser_controller.h"

BluetoothChooserDesktop::BluetoothChooserDesktop(
    BluetoothChooserController* bluetooth_chooser_controller)
    : bluetooth_chooser_controller_(bluetooth_chooser_controller) {
  DCHECK(bluetooth_chooser_controller_);
}

BluetoothChooserDesktop::~BluetoothChooserDesktop() {
  // This satisfies the WebContentsDelegate::RunBluetoothChooser() requirement
  // that the EventHandler can be destroyed any time after the BluetoothChooser
  // instance.
  bluetooth_chooser_controller_->ResetEventHandler();
}

void BluetoothChooserDesktop::SetAdapterPresence(AdapterPresence presence) {
  bluetooth_chooser_controller_->OnAdapterPresenceChanged(presence);
}

void BluetoothChooserDesktop::ShowDiscoveryState(DiscoveryState state) {
  bluetooth_chooser_controller_->OnDiscoveryStateChanged(state);
}

void BluetoothChooserDesktop::AddOrUpdateDevice(
    const std::string& device_id,
    bool should_update_name,
    const base::string16& device_name,
    bool is_gatt_connected,
    bool is_paired,
    int signal_strength_level) {
  bluetooth_chooser_controller_->AddOrUpdateDevice(
      device_id, should_update_name, device_name, is_gatt_connected, is_paired,
      signal_strength_level);
}
