// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/sync_wifi/wifi_config_delegate_chromeos.h"

#include <memory>

#include "base/bind.h"
#include "base/logging.h"
#include "base/values.h"
#include "chromeos/network/managed_network_configuration_handler.h"
#include "components/sync_wifi/wifi_credential.h"

namespace sync_wifi {

namespace {

void OnCreateConfigurationFailed(
    const WifiCredential& wifi_credential,
    const std::string& config_handler_error_message,
    std::unique_ptr<base::DictionaryValue> error_data) {
  LOG(ERROR) << "Create configuration failed";
  // TODO(quiche): check if there is a matching network already. If
  // so, try to configure it with |wifi_credential|.
}

}  // namespace

WifiConfigDelegateChromeOs::WifiConfigDelegateChromeOs(
    const std::string& user_hash,
    chromeos::ManagedNetworkConfigurationHandler* managed_net_config_handler)
    : user_hash_(user_hash),
      managed_network_configuration_handler_(managed_net_config_handler) {
  DCHECK(!user_hash_.empty());
  DCHECK(managed_network_configuration_handler_);
}

WifiConfigDelegateChromeOs::~WifiConfigDelegateChromeOs() {}

void WifiConfigDelegateChromeOs::AddToLocalNetworks(
    const WifiCredential& network_credential) {
  std::unique_ptr<base::DictionaryValue> onc_properties(
      network_credential.ToOncProperties());
  // TODO(quiche): Replace with DCHECK, once ONC supports non-UTF-8 SSIDs.
  // crbug.com/432546
  if (!onc_properties) {
    LOG(ERROR) << "Failed to generate ONC properties for "
               << network_credential.ToString();
    return;
  }

  managed_network_configuration_handler_->CreateConfiguration(
      user_hash_, *onc_properties,
      chromeos::network_handler::ServiceResultCallback(),
      base::Bind(OnCreateConfigurationFailed, network_credential));
}

}  // namespace sync_wifi
