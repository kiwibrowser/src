// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chromeos/components/proximity_auth/remote_device_life_cycle_impl.h"

#include <memory>
#include <utility>

#include "base/bind.h"
#include "base/command_line.h"
#include "base/location.h"
#include "base/threading/thread_task_runner_handle.h"
#include "base/time/default_tick_clock.h"
#include "chromeos/components/proximity_auth/bluetooth_low_energy_connection_finder.h"
#include "chromeos/components/proximity_auth/logging/logging.h"
#include "chromeos/components/proximity_auth/messenger_impl.h"
#include "chromeos/components/proximity_auth/switches.h"
#include "components/cryptauth/connection_finder.h"
#include "components/cryptauth/device_to_device_authenticator.h"
#include "components/cryptauth/secure_context.h"
#include "components/cryptauth/secure_message_delegate_impl.h"

namespace proximity_auth {

namespace {

// The time to wait, in seconds, after authentication fails, before retrying
// another connection.
const int kAuthenticationRecoveryTimeSeconds = 10;

}  // namespace

RemoteDeviceLifeCycleImpl::RemoteDeviceLifeCycleImpl(
    cryptauth::RemoteDeviceRef remote_device)
    : remote_device_(remote_device),
      state_(RemoteDeviceLifeCycle::State::STOPPED),
      weak_ptr_factory_(this) {}

RemoteDeviceLifeCycleImpl::~RemoteDeviceLifeCycleImpl() {}

void RemoteDeviceLifeCycleImpl::Start() {
  PA_LOG(INFO) << "Life cycle for " << remote_device_.name() << " started.";
  DCHECK(state_ == RemoteDeviceLifeCycle::State::STOPPED);
  FindConnection();
}

cryptauth::RemoteDeviceRef RemoteDeviceLifeCycleImpl::GetRemoteDevice() const {
  return remote_device_;
}

cryptauth::Connection* RemoteDeviceLifeCycleImpl::GetConnection() const {
  if (connection_)
    return connection_.get();
  if (messenger_)
    return messenger_->GetConnection();
  return nullptr;
}

RemoteDeviceLifeCycle::State RemoteDeviceLifeCycleImpl::GetState() const {
  return state_;
}

Messenger* RemoteDeviceLifeCycleImpl::GetMessenger() {
  return messenger_.get();
}

void RemoteDeviceLifeCycleImpl::AddObserver(Observer* observer) {
  observers_.AddObserver(observer);
}

void RemoteDeviceLifeCycleImpl::RemoveObserver(Observer* observer) {
  observers_.RemoveObserver(observer);
}

std::unique_ptr<cryptauth::ConnectionFinder>
RemoteDeviceLifeCycleImpl::CreateConnectionFinder() {
  return std::make_unique<BluetoothLowEnergyConnectionFinder>(remote_device_);
}

std::unique_ptr<cryptauth::Authenticator>
RemoteDeviceLifeCycleImpl::CreateAuthenticator() {
  return std::make_unique<cryptauth::DeviceToDeviceAuthenticator>(
      connection_.get(), remote_device_.user_id(),
      cryptauth::SecureMessageDelegateImpl::Factory::NewInstance());
}

void RemoteDeviceLifeCycleImpl::TransitionToState(
    RemoteDeviceLifeCycle::State new_state) {
  PA_LOG(INFO) << "Life cycle transition: " << state_ << " => " << new_state;
  RemoteDeviceLifeCycle::State old_state = state_;
  state_ = new_state;
  for (auto& observer : observers_)
    observer.OnLifeCycleStateChanged(old_state, new_state);
}

void RemoteDeviceLifeCycleImpl::FindConnection() {
  connection_finder_ = CreateConnectionFinder();
  if (!connection_finder_) {
    // TODO(tengs): We need to introduce a failed state if the RemoteDevice data
    // is invalid.
    TransitionToState(RemoteDeviceLifeCycleImpl::State::FINDING_CONNECTION);
    return;
  }

  connection_finder_->Find(
      base::Bind(&RemoteDeviceLifeCycleImpl::OnConnectionFound,
                 weak_ptr_factory_.GetWeakPtr()));
  TransitionToState(RemoteDeviceLifeCycle::State::FINDING_CONNECTION);
}

void RemoteDeviceLifeCycleImpl::OnConnectionFound(
    std::unique_ptr<cryptauth::Connection> connection) {
  DCHECK(state_ == RemoteDeviceLifeCycle::State::FINDING_CONNECTION);
  connection_ = std::move(connection);
  authenticator_ = CreateAuthenticator();
  authenticator_->Authenticate(
      base::Bind(&RemoteDeviceLifeCycleImpl::OnAuthenticationResult,
                 weak_ptr_factory_.GetWeakPtr()));
  TransitionToState(RemoteDeviceLifeCycle::State::AUTHENTICATING);
}

void RemoteDeviceLifeCycleImpl::OnAuthenticationResult(
    cryptauth::Authenticator::Result result,
    std::unique_ptr<cryptauth::SecureContext> secure_context) {
  DCHECK(state_ == RemoteDeviceLifeCycle::State::AUTHENTICATING);
  authenticator_.reset();
  if (result != cryptauth::Authenticator::Result::SUCCESS) {
    PA_LOG(WARNING) << "Waiting " << kAuthenticationRecoveryTimeSeconds
                    << " seconds to retry after authentication failure.";
    connection_->Disconnect();
    authentication_recovery_timer_.Start(
        FROM_HERE,
        base::TimeDelta::FromSeconds(kAuthenticationRecoveryTimeSeconds), this,
        &RemoteDeviceLifeCycleImpl::FindConnection);
    TransitionToState(RemoteDeviceLifeCycle::State::AUTHENTICATION_FAILED);
    return;
  }

  // Create the MessengerImpl asynchronously. |messenger_| registers itself as
  // an observer of |connection_|, so creating it synchronously would trigger
  // |OnSendCompleted()| as an observer call for |messenger_|.
  secure_context_ = std::move(secure_context);
  base::ThreadTaskRunnerHandle::Get()->PostTask(
      FROM_HERE, base::BindOnce(&RemoteDeviceLifeCycleImpl::CreateMessenger,
                                weak_ptr_factory_.GetWeakPtr()));
}

void RemoteDeviceLifeCycleImpl::CreateMessenger() {
  DCHECK(state_ == RemoteDeviceLifeCycle::State::AUTHENTICATING);
  DCHECK(secure_context_);
  messenger_.reset(
      new MessengerImpl(std::move(connection_), std::move(secure_context_)));
  messenger_->AddObserver(this);

  TransitionToState(RemoteDeviceLifeCycle::State::SECURE_CHANNEL_ESTABLISHED);
}

void RemoteDeviceLifeCycleImpl::OnDisconnected() {
  DCHECK(state_ == RemoteDeviceLifeCycle::State::SECURE_CHANNEL_ESTABLISHED);
  messenger_.reset();
  FindConnection();
}

}  // namespace proximity_auth
