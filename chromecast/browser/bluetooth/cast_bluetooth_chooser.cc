// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chromecast/browser/bluetooth/cast_bluetooth_chooser.h"

namespace chromecast {

CastBluetoothChooser::CastBluetoothChooser(
    content::BluetoothChooser::EventHandler event_handler,
    mojom::BluetoothDeviceAccessProviderPtr provider)
    : event_handler_(std::move(event_handler)), binding_(this) {
  DCHECK(event_handler_);

  mojom::BluetoothDeviceAccessProviderClientPtr client;
  binding_.Bind(mojo::MakeRequest(&client));
  binding_.set_connection_error_handler(base::BindOnce(
      &CastBluetoothChooser::OnClientConnectionError, base::Unretained(this)));
  provider->RequestDeviceAccess(std::move(client));
}

CastBluetoothChooser::~CastBluetoothChooser() = default;

void CastBluetoothChooser::GrantAccess(const std::string& address) {
  DCHECK(event_handler_);

  if (all_devices_approved_) {
    LOG(WARNING) << __func__ << " called after access granted to all devices!";
    return;
  }

  if (available_devices_.find(address) != available_devices_.end()) {
    RunEventHandlerAndResetBinding(Event::SELECTED, address);
    return;
  }
  approved_devices_.insert(address);
}

void CastBluetoothChooser::GrantAccessToAllDevices() {
  DCHECK(event_handler_);

  all_devices_approved_ = true;
  if (!available_devices_.empty()) {
    RunEventHandlerAndResetBinding(Event::SELECTED,
                                   *available_devices_.begin());
  }
}

void CastBluetoothChooser::AddOrUpdateDevice(const std::string& device_id,
                                             bool should_update_name,
                                             const base::string16& device_name,
                                             bool is_gatt_connected,
                                             bool is_paired,
                                             int signal_strength_level) {
  DCHECK(event_handler_);

  // Note: |device_id| is just a canonical Bluetooth address.
  if (all_devices_approved_ ||
      approved_devices_.find(device_id) != approved_devices_.end()) {
    RunEventHandlerAndResetBinding(Event::SELECTED, device_id);
    return;
  }
  available_devices_.insert(device_id);
}

void CastBluetoothChooser::RunEventHandlerAndResetBinding(
    content::BluetoothChooser::Event event,
    std::string address) {
  DCHECK(event_handler_);
  std::move(event_handler_).Run(event, std::move(address));
  binding_.Close();
}

void CastBluetoothChooser::OnClientConnectionError() {
  // If the DeviceAccessProvider has granted access to all devices, it may
  // tear down the client immediately. In this case, do not run the event
  // handler, as we may have not had the opportunity to select a device.
  if (!all_devices_approved_ && event_handler_) {
    RunEventHandlerAndResetBinding(Event::CANCELLED, "");
  }
}

}  // namespace chromecast
