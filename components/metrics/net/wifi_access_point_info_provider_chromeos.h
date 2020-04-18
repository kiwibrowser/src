// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_METRICS_NET_WIFI_ACCESS_POINT_INFO_PROVIDER_CHROMEOS_H_
#define COMPONENTS_METRICS_NET_WIFI_ACCESS_POINT_INFO_PROVIDER_CHROMEOS_H_

#include <string>

#include "base/macros.h"
#include "base/memory/weak_ptr.h"
#include "base/values.h"
#include "chromeos/network/network_state_handler_observer.h"
#include "components/metrics/net/wifi_access_point_info_provider.h"

namespace metrics {

// WifiAccessPointInfoProviderChromeos provides the connected wifi
// acccess point information for chromeos.
class WifiAccessPointInfoProviderChromeos
    : public WifiAccessPointInfoProvider,
      public chromeos::NetworkStateHandlerObserver,
      public base::SupportsWeakPtr<WifiAccessPointInfoProviderChromeos> {
 public:
  WifiAccessPointInfoProviderChromeos();
  ~WifiAccessPointInfoProviderChromeos() override;

  // WifiAccessPointInfoProvider:
  bool GetInfo(WifiAccessPointInfo* info) override;

  // NetworkStateHandlerObserver:
  void DefaultNetworkChanged(
      const chromeos::NetworkState* default_network) override;

 private:
  // Callback from Shill.Service.GetProperties. Parses |properties| to obtain
  // the wifi access point information.
  void ParseInfo(const std::string& service_path,
                 const base::DictionaryValue& properties);

  WifiAccessPointInfo wifi_access_point_info_;

  DISALLOW_COPY_AND_ASSIGN(WifiAccessPointInfoProviderChromeos);
};

}  // namespace metrics

#endif  // COMPONENTS_METRICS_NET_WIFI_ACCESS_POINT_INFO_PROVIDER_CHROMEOS_H_
