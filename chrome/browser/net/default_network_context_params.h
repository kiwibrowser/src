// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_NET_DEFAULT_NETWORK_CONTEXT_PARAMS_H_
#define CHROME_BROWSER_NET_DEFAULT_NETWORK_CONTEXT_PARAMS_H_

#include "services/network/public/mojom/network_service.mojom.h"

class PrefRegistrySimple;

// Returns default set of parameters for configuring the network service.
network::mojom::NetworkContextParamsPtr CreateDefaultNetworkContextParams();

// Registers prefs used in creating the default NetworkContextParams.
void RegisterNetworkContextCreationPrefs(PrefRegistrySimple* registry);

#endif  // CHROME_BROWSER_NET_DEFAULT_NETWORK_CONTEXT_PARAMS_H_
