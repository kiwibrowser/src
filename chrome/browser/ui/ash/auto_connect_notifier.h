// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_ASH_AUTO_CONNECT_NOTIFIER_H_
#define CHROME_BROWSER_UI_ASH_AUTO_CONNECT_NOTIFIER_H_

#include <memory>
#include <string>

#include "base/macros.h"
#include "chromeos/network/auto_connect_handler.h"
#include "chromeos/network/network_connection_observer.h"
#include "chromeos/network/network_state_handler_observer.h"

class Profile;

namespace base {
class Timer;
}  // namespace base

namespace chromeos {
class NetworkStateHandler;
}  // namespace chromeos

// Notifies the user when a managed device policy auto-connects to a secure
// network after the user has explicitly requested another network connection.
// See https://crbug.com/764000 for details.
class AutoConnectNotifier : public chromeos::NetworkConnectionObserver,
                            public chromeos::NetworkStateHandlerObserver,
                            public chromeos::AutoConnectHandler::Observer {
 public:
  AutoConnectNotifier(
      Profile* profile,
      chromeos::NetworkConnectionHandler* network_connection_handler,
      chromeos::NetworkStateHandler* network_state_handler,
      chromeos::AutoConnectHandler* auto_connect_handler);
  ~AutoConnectNotifier() override;

 protected:
  // chromeos::NetworkConnectionObserver:
  void ConnectToNetworkRequested(const std::string& service_path) override;

  // chromeos::NetworkStateHandlerObserver:
  void NetworkConnectionStateChanged(
      const chromeos::NetworkState* network) override;

  // chromeos::AutoConnectHandler::Observer:
  void OnAutoConnectedInitiated(int auto_connect_reasons) override;

 private:
  friend class AutoConnectNotifierTest;

  void DisplayNotification();

  void SetTimerForTesting(std::unique_ptr<base::Timer> test_timer);

  Profile* profile_;
  chromeos::NetworkConnectionHandler* network_connection_handler_;
  chromeos::NetworkStateHandler* network_state_handler_;
  chromeos::AutoConnectHandler* auto_connect_handler_;

  bool has_user_explicitly_requested_connection_ = false;
  std::unique_ptr<base::Timer> timer_;

  DISALLOW_COPY_AND_ASSIGN(AutoConnectNotifier);
};

#endif  // CHROME_BROWSER_UI_ASH_AUTO_CONNECT_NOTIFIER_H_
