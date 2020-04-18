// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROMEOS_COMPONENTS_TETHER_FAKE_WIFI_HOTSPOT_DISCONNECTOR_H_
#define CHROMEOS_COMPONENTS_TETHER_FAKE_WIFI_HOTSPOT_DISCONNECTOR_H_

#include "base/callback.h"
#include "base/macros.h"
#include "chromeos/components/tether/wifi_hotspot_disconnector.h"

namespace chromeos {

namespace tether {

// Test double for WifiHotspotDisconnector.
class FakeWifiHotspotDisconnector : public WifiHotspotDisconnector {
 public:
  FakeWifiHotspotDisconnector();
  ~FakeWifiHotspotDisconnector() override;

  std::string last_disconnected_wifi_network_guid() {
    return last_disconnected_wifi_network_guid_;
  }

  void set_disconnection_error_name(
      const std::string& disconnection_error_name) {
    disconnection_error_name_ = disconnection_error_name;
  }

  // WifiHotspotDisconnector:
  void DisconnectFromWifiHotspot(
      const std::string& wifi_network_guid,
      const base::Closure& success_callback,
      const network_handler::StringResultCallback& error_callback) override;

 private:
  std::string last_disconnected_wifi_network_guid_;
  std::string disconnection_error_name_;

  DISALLOW_COPY_AND_ASSIGN(FakeWifiHotspotDisconnector);
};

}  // namespace tether

}  // namespace chromeos

#endif  // CHROMEOS_COMPONENTS_TETHER_FAKE_WIFI_HOTSPOT_DISCONNECTOR_H_
