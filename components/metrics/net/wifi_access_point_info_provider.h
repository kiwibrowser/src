// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_METRICS_NET_WIFI_ACCESS_POINT_INFO_PROVIDER_H_
#define COMPONENTS_METRICS_NET_WIFI_ACCESS_POINT_INFO_PROVIDER_H_

#include <string>

#include "base/macros.h"

namespace metrics {

// Interface for accessing connected wireless access point information.
class WifiAccessPointInfoProvider {
 public:
  // Wifi access point security mode definitions.
  enum WifiSecurityMode {
    WIFI_SECURITY_UNKNOWN = 0,
    WIFI_SECURITY_WPA = 1,
    WIFI_SECURITY_WEP = 2,
    WIFI_SECURITY_RSN = 3,
    WIFI_SECURITY_802_1X = 4,
    WIFI_SECURITY_PSK = 5,
    WIFI_SECURITY_NONE
  };

  // Information of the currently connected wifi access point.
  struct WifiAccessPointInfo {
    WifiAccessPointInfo();
    ~WifiAccessPointInfo();
    WifiSecurityMode security;
    std::string bssid;
    std::string model_number;
    std::string model_name;
    std::string device_name;
    std::string oui_list;
  };

  WifiAccessPointInfoProvider();
  virtual ~WifiAccessPointInfoProvider();

  // Fill in the wifi access point info if device is currently connected to a
  // wifi access point. Return true if device is connected to a wifi access
  // point, false otherwise.
  virtual bool GetInfo(WifiAccessPointInfo *info);

 private:
  DISALLOW_COPY_AND_ASSIGN(WifiAccessPointInfoProvider);
};

}  // namespace metrics

#endif  // COMPONENTS_METRICS_NET_WIFI_ACCESS_POINT_INFO_PROVIDER_H_
