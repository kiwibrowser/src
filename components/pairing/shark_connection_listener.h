// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_PAIRING_SHARK_CONNECTION_LISTENER_H_
#define COMPONENTS_PAIRING_SHARK_CONNECTION_LISTENER_H_

#include <memory>
#include <string>

#include "base/callback.h"
#include "base/macros.h"
#include "components/pairing/host_pairing_controller.h"

namespace service_manager {
class Connector;
}

namespace pairing_chromeos {

class BluetoothHostPairingController;

// Listens for incoming connection from shark controller. If connection
// is established, invokes callback passing HostPairingController
// as an argument.
class SharkConnectionListener : public HostPairingController::Observer {
 public:
  using OnConnectedCallback =
      base::Callback<void(std::unique_ptr<HostPairingController>)>;

  SharkConnectionListener(service_manager::Connector* connector,
                          OnConnectedCallback callback);
  ~SharkConnectionListener() override;

  void ResetController();

  // This function is used for testing.
  BluetoothHostPairingController* GetControllerForTesting();

 private:
  typedef HostPairingController::Stage Stage;

  // HostPairingController::Observer overrides:
  void PairingStageChanged(Stage new_stage) override;

  OnConnectedCallback callback_;
  std::unique_ptr<HostPairingController> controller_;

  DISALLOW_COPY_AND_ASSIGN(SharkConnectionListener);
};

}  // namespace pairing_chromeos

#endif  // COMPONENTS_PAIRING_SHARK_CONNECTION_LISTENER_H_
