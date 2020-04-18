// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_SYNC_WIFI_WIFI_CONFIG_DELEGATE_CHROMEOS_H_
#define COMPONENTS_SYNC_WIFI_WIFI_CONFIG_DELEGATE_CHROMEOS_H_

#include <string>

#include "base/macros.h"
#include "components/sync_wifi/wifi_config_delegate.h"

namespace chromeos {
class ManagedNetworkConfigurationHandler;
}

namespace sync_wifi {

// ChromeOS-specific implementation of the WifiConfigDelegate interface.
class WifiConfigDelegateChromeOs : public WifiConfigDelegate {
 public:
  // Constructs a delegate, which is responsible for applying changes
  // to the local network configuration. Any changes made by this
  // delegate will be applied to the Shill profile that corresponds to
  // |user_hash|. The caller must ensure that the
  // |managed_net_config_handler| outlives the delegate.
  WifiConfigDelegateChromeOs(
      const std::string& user_hash,
      chromeos::ManagedNetworkConfigurationHandler* managed_net_config_handler);
  ~WifiConfigDelegateChromeOs() override;

  // WifiConfigDelegate implementation.
  void AddToLocalNetworks(const WifiCredential& network_credential) override;

 private:
  // The ChromeOS user-hash that should be used when modifying network
  // configurations.
  const std::string user_hash_;

  // The object we use to configure ChromeOS network state.
  chromeos::ManagedNetworkConfigurationHandler* const
      managed_network_configuration_handler_;

  DISALLOW_COPY_AND_ASSIGN(WifiConfigDelegateChromeOs);
};

}  // namespace sync_wifi

#endif  // COMPONENTS_SYNC_WIFI_WIFI_CONFIG_DELEGATE_CHROMEOS_H_
