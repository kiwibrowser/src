// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROMEOS_NETWORK_PROXY_UI_PROXY_CONFIG_SERVICE_H_
#define CHROMEOS_NETWORK_PROXY_UI_PROXY_CONFIG_SERVICE_H_

#include <string>

#include "base/macros.h"
#include "chromeos/chromeos_export.h"
#include "chromeos/network/proxy/ui_proxy_config.h"
#include "components/prefs/pref_change_registrar.h"

class PrefService;

namespace chromeos {

class NetworkState;

// This class provides an interface to the UI for getting and setting a proxy
// configuration. Primarily this class caches the "current" requested config
// and populates UIProxyConfig for convenient UI consumption.
// NOTE: This class must be rebuilt when the logged in profile changes.
// ALSO NOTE: The provided PrefService instances are used both to retreive proxy
// configurations set by an extension, and for ONC policy information associated
// with a network. (Per-network proxy configurations are stored in Shill,
// but ONC policy configuration is stored in PrefService).
class CHROMEOS_EXPORT UIProxyConfigService {
 public:
  // |local_state_prefs| must not be null. |profile_prefs| can be
  // null if there is no logged in user, in which case only the local state
  // (device) prefs will be used. See note above.
  UIProxyConfigService(PrefService* profile_prefs,
                       PrefService* local_state_prefs);
  ~UIProxyConfigService();

  // Called when ::proxy_config::prefs change or to force an update.
  void UpdateFromPrefs(const std::string& guid);

  // Called from UI to retrieve the active proxy configuration. This will be,
  // in highest to lowest priority order:
  // * A policy enforced proxy associated with |network_guid|.
  // * A proxy set by an extension in the active PrefService.
  // * A user specified proxy associated with |network_guid|.
  void GetProxyConfig(const std::string& network_guid, UIProxyConfig* config);

  // Called from the UI to update the user proxy configuration for
  // |network_guid|. The proxy specified by |config| is stored by Shill.
  void SetProxyConfig(const std::string& network_guid,
                      const UIProxyConfig& config);

  // Returns true if there is a default network and it has a proxy configuration
  // with mode == MODE_FIXED_SERVERS.
  bool HasDefaultNetworkProxyConfigured();

 private:
  // Determines effective proxy config based on prefs from config tracker,
  // |network| and if user is using shared proxies.  The effective config is
  // stored in |current_ui_config_| but not activated on network stack, and
  // hence, not picked up by observers.
  void DetermineEffectiveConfig(const NetworkState& network);

  void OnPreferenceChanged(const std::string& pref_name);

  // GUID of network used for current_ui_config_.
  std::string current_ui_network_guid_;

  // Proxy configuration for |current_ui_network_guid_|.
  UIProxyConfig current_ui_config_;

  PrefService* profile_prefs_;  // unowned
  PrefChangeRegistrar profile_registrar_;

  PrefService* local_state_prefs_;  // unowned
  PrefChangeRegistrar local_state_registrar_;

  DISALLOW_COPY_AND_ASSIGN(UIProxyConfigService);
};

}  // namespace chromeos

#endif  // CHROMEOS_NETWORK_PROXY_UI_PROXY_CONFIG_SERVICE_H_
