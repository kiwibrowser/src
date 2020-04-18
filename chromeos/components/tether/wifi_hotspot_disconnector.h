// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROMEOS_COMPONENTS_TETHER_WIFI_HOTSPOT_DISCONNECTOR_H_
#define CHROMEOS_COMPONENTS_TETHER_WIFI_HOTSPOT_DISCONNECTOR_H_

#include "base/callback.h"
#include "base/macros.h"
#include "chromeos/network/network_handler_callbacks.h"

namespace chromeos {

namespace tether {

// Disconnects from Wi-Fi hotspots provided by Tether hosts.
class WifiHotspotDisconnector {
 public:
  WifiHotspotDisconnector() {}
  virtual ~WifiHotspotDisconnector() {}

  // Disconnects from the Wi-Fi network with GUID |wifi_network_guid| and
  // removes the corresponding network configuration (i.e., removes the "known
  // network" from network settings).
  virtual void DisconnectFromWifiHotspot(
      const std::string& wifi_network_guid,
      const base::Closure& success_callback,
      const network_handler::StringResultCallback& error_callback) = 0;

 private:
  DISALLOW_COPY_AND_ASSIGN(WifiHotspotDisconnector);
};

}  // namespace tether

}  // namespace chromeos

#endif  // CHROMEOS_COMPONENTS_TETHER_WIFI_HOTSPOT_DISCONNECTOR_H_
