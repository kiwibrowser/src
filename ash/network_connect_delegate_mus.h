// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ASH_NETWORK_CONNECT_DELEGATE_MUS_H_
#define ASH_NETWORK_CONNECT_DELEGATE_MUS_H_

#include "base/macros.h"
#include "chromeos/network/network_connect.h"

namespace ash {

// Routes requests to show network config UI over the mojom::SystemTrayClient
// interface.
// TODO(mash): Replace NetworkConnect::Delegate with a client interface on
// a mojo NetworkConfig service. http://crbug.com/644355
class NetworkConnectDelegateMus : public chromeos::NetworkConnect::Delegate {
 public:
  NetworkConnectDelegateMus();
  ~NetworkConnectDelegateMus() override;

  // chromeos::NetworkConnect::Delegate:
  void ShowNetworkConfigure(const std::string& network_id) override;
  void ShowNetworkSettings(const std::string& network_id) override;
  bool ShowEnrollNetwork(const std::string& network_id) override;
  void ShowMobileSetupDialog(const std::string& network_id) override;
  void ShowNetworkConnectError(const std::string& error_name,
                               const std::string& network_id) override;
  void ShowMobileActivationError(const std::string& network_id) override;

 private:
  DISALLOW_COPY_AND_ASSIGN(NetworkConnectDelegateMus);
};

}  // namespace ash

#endif  // ASH_NETWORK_CONNECT_DELEGATE_MUS_H_
