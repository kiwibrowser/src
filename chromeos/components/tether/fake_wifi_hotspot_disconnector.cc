// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chromeos/components/tether/fake_wifi_hotspot_disconnector.h"

namespace chromeos {

namespace tether {

FakeWifiHotspotDisconnector::FakeWifiHotspotDisconnector() = default;

FakeWifiHotspotDisconnector::~FakeWifiHotspotDisconnector() = default;

void FakeWifiHotspotDisconnector::DisconnectFromWifiHotspot(
    const std::string& wifi_network_guid,
    const base::Closure& success_callback,
    const network_handler::StringResultCallback& error_callback) {
  last_disconnected_wifi_network_guid_ = wifi_network_guid;

  if (disconnection_error_name_.empty())
    success_callback.Run();
  else
    error_callback.Run(disconnection_error_name_);
}

}  // namespace tether

}  // namespace chromeos
