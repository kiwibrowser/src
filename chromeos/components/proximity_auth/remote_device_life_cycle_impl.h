// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROMEOS_COMPONENTS_PROXIMITY_AUTH_REMOTE_DEVICE_LIFE_CYCLE_IMPL_H_
#define CHROMEOS_COMPONENTS_PROXIMITY_AUTH_REMOTE_DEVICE_LIFE_CYCLE_IMPL_H_

#include <memory>

#include "base/macros.h"
#include "base/memory/weak_ptr.h"
#include "base/observer_list.h"
#include "base/timer/timer.h"
#include "chromeos/components/proximity_auth/messenger_observer.h"
#include "chromeos/components/proximity_auth/remote_device_life_cycle.h"
#include "components/cryptauth/authenticator.h"
#include "components/cryptauth/connection.h"
#include "components/cryptauth/connection_finder.h"
#include "components/cryptauth/remote_device_ref.h"

namespace cryptauth {
class SecureContext;
}

namespace proximity_auth {

class Messenger;

// Implementation of RemoteDeviceLifeCycle.
class RemoteDeviceLifeCycleImpl : public RemoteDeviceLifeCycle,
                                  public MessengerObserver {
 public:
  // Creates the life cycle for controlling the given |remote_device|.
  // |proximity_auth_client| is not owned.
  explicit RemoteDeviceLifeCycleImpl(cryptauth::RemoteDeviceRef remote_device);
  ~RemoteDeviceLifeCycleImpl() override;

  // RemoteDeviceLifeCycle:
  void Start() override;
  cryptauth::RemoteDeviceRef GetRemoteDevice() const override;
  cryptauth::Connection* GetConnection() const override;
  RemoteDeviceLifeCycle::State GetState() const override;
  Messenger* GetMessenger() override;
  void AddObserver(Observer* observer) override;
  void RemoveObserver(Observer* observer) override;

 protected:
  // Creates and returns a cryptauth::ConnectionFinder instance for
  // |remote_device_|.
  // Exposed for testing.
  virtual std::unique_ptr<cryptauth::ConnectionFinder> CreateConnectionFinder();

  // Creates and returns an Authenticator instance for |connection_|.
  // Exposed for testing.
  virtual std::unique_ptr<cryptauth::Authenticator> CreateAuthenticator();

 private:
  // Transitions to |new_state|, and notifies observers.
  void TransitionToState(RemoteDeviceLifeCycle::State new_state);

  // Transtitions to FINDING_CONNECTION state. Creates and starts
  // |connection_finder_|.
  void FindConnection();

  // Called when |connection_finder_| finds a connection.
  void OnConnectionFound(std::unique_ptr<cryptauth::Connection> connection);

  // Callback when |authenticator_| completes authentication.
  void OnAuthenticationResult(
      cryptauth::Authenticator::Result result,
      std::unique_ptr<cryptauth::SecureContext> secure_context);

  // Creates the messenger which parses status updates.
  void CreateMessenger();

  // MessengerObserver:
  void OnDisconnected() override;

  // The remote device being controlled.
  const cryptauth::RemoteDeviceRef remote_device_;

  // The current state in the life cycle.
  RemoteDeviceLifeCycle::State state_;

  // Observers added to the life cycle.
  base::ObserverList<Observer> observers_{
      base::ObserverListPolicy::EXISTING_ONLY};

  // The connection that is established by |connection_finder_|.
  std::unique_ptr<cryptauth::Connection> connection_;

  // Context for encrypting and decrypting messages. Created after
  // authentication succeeds. Ownership is eventually passed to |messenger_|.
  std::unique_ptr<cryptauth::SecureContext> secure_context_;

  // The messenger for sending and receiving messages in the
  // SECURE_CHANNEL_ESTABLISHED state.
  std::unique_ptr<Messenger> messenger_;

  // Authenticates the remote device after it is connected. Used in the
  // AUTHENTICATING state.
  std::unique_ptr<cryptauth::Authenticator> authenticator_;

  // Used in the FINDING_CONNECTION state to establish a connection to the
  // remote device.
  std::unique_ptr<cryptauth::ConnectionFinder> connection_finder_;

  // After authentication fails, this timer waits for a period of time before
  // retrying the connection.
  base::OneShotTimer authentication_recovery_timer_;

  base::WeakPtrFactory<RemoteDeviceLifeCycleImpl> weak_ptr_factory_;

  DISALLOW_COPY_AND_ASSIGN(RemoteDeviceLifeCycleImpl);
};

}  // namespace proximity_auth

#endif  // CHROMEOS_COMPONENTS_PROXIMITY_AUTH_REMOTE_DEVICE_LIFE_CYCLE_IMPL_H_
