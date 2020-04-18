// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/sync_wifi/network_state_helper_chromeos.h"

#include <memory>

#include "base/logging.h"
#include "chromeos/network/network_state.h"
#include "chromeos/network/network_state_handler.h"
#include "chromeos/network/network_type_pattern.h"
#include "components/sync_wifi/wifi_security_class.h"

namespace sync_wifi {

WifiCredential::CredentialSet GetWifiCredentialsForShillProfile(
    chromeos::NetworkStateHandler* network_state_handler,
    const std::string& shill_profile_path) {
  DCHECK(network_state_handler);

  chromeos::NetworkStateHandler::NetworkStateList networks;
  network_state_handler->GetNetworkListByType(
      chromeos::NetworkTypePattern::WiFi(), true /* configured_only */,
      false /* visible_only */, 0 /* unlimited result size */, &networks);

  auto credentials(WifiCredential::MakeSet());
  for (const chromeos::NetworkState* network : networks) {
    if (network->profile_path() != shill_profile_path)
      continue;

    // TODO(quiche): Fill in the actual passphrase via an asynchronous
    // call to a chromeos::NetworkConfigurationHandler instance's
    // GetProperties method.
    std::unique_ptr<WifiCredential> credential = WifiCredential::Create(
        network->raw_ssid(),
        WifiSecurityClassFromShillSecurity(network->security_class()),
        "" /* empty passphrase */);
    if (!credential)
      LOG(ERROR) << "Failed to create credential";
    else
      credentials.insert(*credential);
  }
  return credentials;
}

}  // namespace sync_wifi
