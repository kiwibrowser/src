// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROMEOS_NETWORK_PROHIBITED_TECHNOLOGIES_HANDLER_H_
#define CHROMEOS_NETWORK_PROHIBITED_TECHNOLOGIES_HANDLER_H_

#include <string>

#include "base/macros.h"
#include "base/values.h"
#include "chromeos/chromeos_export.h"
#include "chromeos/login/login_state.h"
#include "chromeos/network/network_handler.h"
#include "chromeos/network/network_policy_observer.h"

namespace chromeos {

class CHROMEOS_EXPORT ProhibitedTechnologiesHandler
    : public LoginState::Observer,
      public NetworkPolicyObserver {
 public:
  ~ProhibitedTechnologiesHandler() override;

  // LoginState::Observer
  void LoggedInStateChanged() override;

  // NetworkPolicyObserver
  void PoliciesChanged(const std::string& userhash) override;
  void PoliciesApplied(const std::string& userhash) override;

  void SetProhibitedTechnologies(const base::ListValue* prohibited_list);
  std::vector<std::string> GetCurrentlyProhibitedTechnologies();

 private:
  friend class NetworkHandler;
  friend class ProhibitedTechnologiesHandlerTest;

  ProhibitedTechnologiesHandler();

  void Init(
      ManagedNetworkConfigurationHandler* managed_network_configuration_handler,
      NetworkStateHandler* network_state_handler);

  void EnforceProhibitedTechnologies();

  std::vector<std::string> prohibited_technologies_;
  ManagedNetworkConfigurationHandler* managed_network_configuration_handler_ =
      nullptr;
  NetworkStateHandler* network_state_handler_ = nullptr;
  bool user_logged_in_ = false;
  bool user_policy_applied_ = false;

  DISALLOW_COPY_AND_ASSIGN(ProhibitedTechnologiesHandler);
};

}  // namespace chromeos

#endif  // CHROMEOS_NETWORK_PROHIBITED_TECHNOLOGIES_HANDLER_H_
