// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chromeos/network/prohibited_technologies_handler.h"

#include "chromeos/network/managed_network_configuration_handler.h"
#include "chromeos/network/network_state_handler.h"
#include "chromeos/network/network_util.h"
#include "third_party/cros_system_api/dbus/service_constants.h"

namespace chromeos {

ProhibitedTechnologiesHandler::ProhibitedTechnologiesHandler() = default;

ProhibitedTechnologiesHandler::~ProhibitedTechnologiesHandler() {
  if (managed_network_configuration_handler_)
    managed_network_configuration_handler_->RemoveObserver(this);
  if (LoginState::IsInitialized())
    LoginState::Get()->RemoveObserver(this);
}

void ProhibitedTechnologiesHandler::Init(
    ManagedNetworkConfigurationHandler* managed_network_configuration_handler,
    NetworkStateHandler* network_state_handler) {
  if (LoginState::IsInitialized())
    LoginState::Get()->AddObserver(this);

  managed_network_configuration_handler_ =
      managed_network_configuration_handler;
  if (managed_network_configuration_handler_)
    managed_network_configuration_handler_->AddObserver(this);
  network_state_handler_ = network_state_handler;

  // Clear the list of prohibited network technologies. As a user logout always
  // triggers a browser process restart, Init() is always invoked to reallow any
  // network technology forbidden for the previous user.
  network_state_handler_->SetProhibitedTechnologies(
      std::vector<std::string>(), chromeos::network_handler::ErrorCallback());

  if (LoginState::IsInitialized())
    LoggedInStateChanged();
}

void ProhibitedTechnologiesHandler::LoggedInStateChanged() {
  user_logged_in_ = LoginState::Get()->IsUserLoggedIn();
  EnforceProhibitedTechnologies();
}

void ProhibitedTechnologiesHandler::PoliciesChanged(
    const std::string& userhash) {}

void ProhibitedTechnologiesHandler::PoliciesApplied(
    const std::string& userhash) {
  if (userhash.empty())
    return;
  user_policy_applied_ = true;
  EnforceProhibitedTechnologies();
}

void ProhibitedTechnologiesHandler::SetProhibitedTechnologies(
    const base::ListValue* prohibited_list) {
  // Build up prohibited network type list and save it for furthur use when
  // enforced
  prohibited_technologies_.clear();
  for (const auto& item : *prohibited_list) {
    std::string prohibited_technology;
    bool item_is_string = item.GetAsString(&prohibited_technology);
    DCHECK(item_is_string);
    std::string translated_tech =
        network_util::TranslateONCTypeToShill(prohibited_technology);
    if (!translated_tech.empty())
      prohibited_technologies_.push_back(translated_tech);
  }
  EnforceProhibitedTechnologies();
}

void ProhibitedTechnologiesHandler::EnforceProhibitedTechnologies() {
  if (user_logged_in_ && user_policy_applied_) {
    network_state_handler_->SetProhibitedTechnologies(
        prohibited_technologies_, network_handler::ErrorCallback());
    if (std::find(prohibited_technologies_.begin(),
                  prohibited_technologies_.end(),
                  shill::kTypeEthernet) != prohibited_technologies_.end())
      return;
  } else {
    // This is done to make sure prohibited technologies are cleared
    // before user policy is applied.
    network_state_handler_->SetProhibitedTechnologies(
        std::vector<std::string>(), network_handler::ErrorCallback());
  }

  // Enable ethernet back as user doesn't have a place to enable it back
  // if user shuts down directly in a user session. As shill will persist
  // ProhibitedTechnologies which may include ethernet, making users can
  // not find Ethernet at next boot or logging out unless user log out first
  // and then shutdown.
  if (network_state_handler_->IsTechnologyAvailable(
          NetworkTypePattern::Ethernet()) &&
      !network_state_handler_->IsTechnologyEnabled(
          NetworkTypePattern::Ethernet()))
    network_state_handler_->SetTechnologyEnabled(
        NetworkTypePattern::Ethernet(), true, network_handler::ErrorCallback());
}

std::vector<std::string>
ProhibitedTechnologiesHandler::GetCurrentlyProhibitedTechnologies() {
  if (user_logged_in_ && user_policy_applied_)
    return prohibited_technologies_;
  return std::vector<std::string>();
}

}  // namespace chromeos
