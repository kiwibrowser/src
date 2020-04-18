// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROMEOS_COMPONENTS_TETHER_CONNECTION_PRESERVER_IMPL_H_
#define CHROMEOS_COMPONENTS_TETHER_CONNECTION_PRESERVER_IMPL_H_

#include <memory>

#include "base/timer/timer.h"
#include "chromeos/components/tether/active_host.h"
#include "chromeos/components/tether/connection_preserver.h"

namespace chromeos {

class NetworkStateHandler;

namespace tether {

class BleConnectionManager;
class TetherHostResponseRecorder;

// Concrete implementation of ConnectionPreserver.
class ConnectionPreserverImpl : public ConnectionPreserver,
                                public ActiveHost::Observer {
 public:
  // The maximum duration of time that a BLE Connection should be preserved.
  // A preserved BLE Connection will be torn down if not used within this time.
  // If the connection is used for a host connection before this time runs out,
  // the Connection will be torn down.
  static constexpr const uint32_t kTimeoutSeconds = 60;

  ConnectionPreserverImpl(
      BleConnectionManager* ble_connection_manager,
      NetworkStateHandler* network_state_handler,
      ActiveHost* active_host,
      TetherHostResponseRecorder* tether_host_response_recorder);
  ~ConnectionPreserverImpl() override;

  // ConnectionPreserver:
  void HandleSuccessfulTetherAvailabilityResponse(
      const std::string& device_id) override;

 protected:
  // ActiveHost::Observer:
  void OnActiveHostChanged(
      const ActiveHost::ActiveHostChangeInfo& change_info) override;

 private:
  friend class ConnectionPreserverImplTest;

  bool IsConnectedToInternet();
  // Between |preserved_connection_device_id_| and |device_id|, return which is
  // the "preferred" preserved Connection, i.e., which is higher priority.
  std::string GetPreferredPreservedConnectionDeviceId(
      const std::string& device_id);
  void SetPreservedConnection(const std::string& device_id);
  void RemovePreservedConnectionIfPresent();

  void SetTimerForTesting(std::unique_ptr<base::Timer> timer_for_test);

  BleConnectionManager* ble_connection_manager_;
  NetworkStateHandler* network_state_handler_;
  ActiveHost* active_host_;
  TetherHostResponseRecorder* tether_host_response_recorder_;
  std::unique_ptr<base::Timer> preserved_connection_timer_;

  std::string preserved_connection_device_id_;

  base::WeakPtrFactory<ConnectionPreserverImpl> weak_ptr_factory_;

  DISALLOW_COPY_AND_ASSIGN(ConnectionPreserverImpl);
};

}  // namespace tether

}  // namespace chromeos

#endif  // CHROMEOS_COMPONENTS_TETHER_CONNECTION_PRESERVER_IMPL_H_