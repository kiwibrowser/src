// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROMEOS_COMPONENTS_PROXIMITY_AUTH_FAKE_REMOTE_DEVICE_LIFE_CYCLE_H_
#define CHROMEOS_COMPONENTS_PROXIMITY_AUTH_FAKE_REMOTE_DEVICE_LIFE_CYCLE_H_

#include "base/macros.h"
#include "base/observer_list.h"
#include "chromeos/components/proximity_auth/remote_device_life_cycle.h"
#include "components/cryptauth/fake_connection.h"
#include "components/cryptauth/remote_device_ref.h"

namespace proximity_auth {

class FakeRemoteDeviceLifeCycle : public RemoteDeviceLifeCycle {
 public:
  explicit FakeRemoteDeviceLifeCycle(cryptauth::RemoteDeviceRef remote_device);
  ~FakeRemoteDeviceLifeCycle() override;

  // RemoteDeviceLifeCycle:
  void Start() override;
  cryptauth::RemoteDeviceRef GetRemoteDevice() const override;
  cryptauth::Connection* GetConnection() const override;
  State GetState() const override;
  Messenger* GetMessenger() override;
  void AddObserver(Observer* observer) override;
  void RemoveObserver(Observer* observer) override;

  // Changes state and notifies observers.
  void ChangeState(State new_state);

  void set_messenger(Messenger* messenger) { messenger_ = messenger; }

  void set_connection(cryptauth::Connection* connection) {
    connection_ = connection;
  }

  bool started() { return started_; }

  base::ObserverList<Observer>& observers() { return observers_; }

 private:
  cryptauth::RemoteDeviceRef remote_device_;

  base::ObserverList<Observer> observers_;

  bool started_;

  State state_;

  cryptauth::Connection* connection_;

  Messenger* messenger_;

  DISALLOW_COPY_AND_ASSIGN(FakeRemoteDeviceLifeCycle);
};

}  // namespace proximity_auth

#endif  // CHROMEOS_COMPONENTS_PROXIMITY_AUTH_FAKE_REMOTE_DEVICE_LIFE_CYCLE_H_
