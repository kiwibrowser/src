// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/web_resource/resource_request_allowed_notifier.h"

#include "base/command_line.h"

namespace web_resource {

ResourceRequestAllowedNotifier::ResourceRequestAllowedNotifier(
    PrefService* local_state,
    const char* disable_network_switch)
    : disable_network_switch_(disable_network_switch),
      local_state_(local_state),
      observer_requested_permission_(false),
      waiting_for_user_to_accept_eula_(false),
      observer_(nullptr) {
}

ResourceRequestAllowedNotifier::~ResourceRequestAllowedNotifier() {
  if (observer_)
    net::NetworkChangeNotifier::RemoveNetworkChangeObserver(this);
}

void ResourceRequestAllowedNotifier::Init(Observer* observer) {
  DCHECK(!observer_);
  DCHECK(observer);
  observer_ = observer;

  net::NetworkChangeNotifier::AddNetworkChangeObserver(this);

  eula_notifier_.reset(CreateEulaNotifier());
  if (eula_notifier_) {
    eula_notifier_->Init(this);
    waiting_for_user_to_accept_eula_ = !eula_notifier_->IsEulaAccepted();
  }
}

ResourceRequestAllowedNotifier::State
ResourceRequestAllowedNotifier::GetResourceRequestsAllowedState() {
  if (disable_network_switch_ &&
      base::CommandLine::ForCurrentProcess()->HasSwitch(
          disable_network_switch_)) {
    return DISALLOWED_COMMAND_LINE_DISABLED;
  }

  // The observer requested permission. Return the current criteria state and
  // set a flag to remind this class to notify the observer once the criteria
  // is met.
  observer_requested_permission_ = waiting_for_user_to_accept_eula_ ||
                                   net::NetworkChangeNotifier::IsOffline();
  if (!observer_requested_permission_)
    return ALLOWED;
  return waiting_for_user_to_accept_eula_ ? DISALLOWED_EULA_NOT_ACCEPTED :
                                            DISALLOWED_NETWORK_DOWN;
}

bool ResourceRequestAllowedNotifier::ResourceRequestsAllowed() {
  return GetResourceRequestsAllowedState() == ALLOWED;
}

void ResourceRequestAllowedNotifier::SetWaitingForEulaForTesting(bool waiting) {
  waiting_for_user_to_accept_eula_ = waiting;
}

void ResourceRequestAllowedNotifier::SetObserverRequestedForTesting(
    bool requested) {
  observer_requested_permission_ = requested;
}

void ResourceRequestAllowedNotifier::MaybeNotifyObserver() {
  // Need to ensure that all criteria are met before notifying observers.
  if (observer_requested_permission_ && ResourceRequestsAllowed()) {
    DVLOG(1) << "Notifying observer of state change.";
    observer_->OnResourceRequestsAllowed();
    // Reset this so the observer is not informed again unless they check
    // ResourceRequestsAllowed again.
    observer_requested_permission_ = false;
  }
}

EulaAcceptedNotifier* ResourceRequestAllowedNotifier::CreateEulaNotifier() {
  return EulaAcceptedNotifier::Create(local_state_);
}

void ResourceRequestAllowedNotifier::OnEulaAccepted() {
  // This flag should have been set if this was waiting on the EULA
  // notification.
  DCHECK(waiting_for_user_to_accept_eula_);
  DVLOG(1) << "EULA was accepted.";
  waiting_for_user_to_accept_eula_ = false;
  MaybeNotifyObserver();
}

void ResourceRequestAllowedNotifier::OnNetworkChanged(
    net::NetworkChangeNotifier::ConnectionType type) {
  if (type != net::NetworkChangeNotifier::CONNECTION_NONE) {
    DVLOG(1) << "Network came online.";
    // MaybeNotifyObserver() internally guarantees that it will only notify the
    // observer if it's currently waiting for the network to come online.
    MaybeNotifyObserver();
  }
}

}  // namespace web_resource
