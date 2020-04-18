// Copyright (c) 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROMEOS_NETWORK_PROXY_PROXY_CONFIG_HANDLER_H_
#define CHROMEOS_NETWORK_PROXY_PROXY_CONFIG_HANDLER_H_

#include <memory>

#include "chromeos/chromeos_export.h"
#include "components/onc/onc_constants.h"

class PrefService;
class ProxyConfigDictionary;

namespace chromeos {

class NetworkState;

namespace proxy_config {

// Get the proxy configuration including per-network policies for network
// |network|. If |profile_prefs| is NULL, then only shared settings (and device
// policy) are respected. This is e.g. the case for the signin screen and the
// system request context.
CHROMEOS_EXPORT std::unique_ptr<ProxyConfigDictionary> GetProxyConfigForNetwork(
    const PrefService* profile_prefs,
    const PrefService* local_state_prefs,
    const NetworkState& network,
    ::onc::ONCSource* onc_source);

CHROMEOS_EXPORT void SetProxyConfigForNetwork(
    const ProxyConfigDictionary& proxy_config,
    const NetworkState& network);

}  // namespace proxy_config

}  // namespace chromeos

#endif  // CHROMEOS_NETWORK_PROXY_PROXY_CONFIG_HANDLER_H_
