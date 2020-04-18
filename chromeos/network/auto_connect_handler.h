// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROMEOS_NETWORK_AUTO_CONNECT_HANDLER_H_
#define CHROMEOS_NETWORK_AUTO_CONNECT_HANDLER_H_

#include <string>

#include "base/macros.h"
#include "base/memory/weak_ptr.h"
#include "base/observer_list.h"
#include "chromeos/chromeos_export.h"
#include "chromeos/login/login_state.h"
#include "chromeos/network/client_cert_resolver.h"
#include "chromeos/network/network_connection_observer.h"
#include "chromeos/network/network_handler.h"
#include "chromeos/network/network_policy_observer.h"
#include "chromeos/network/network_state_handler_observer.h"

class AutoConnectNotifierTest;

namespace chromeos {

class CHROMEOS_EXPORT AutoConnectHandler : public LoginState::Observer,
                                           public NetworkPolicyObserver,
                                           public NetworkConnectionObserver,
                                           public NetworkStateHandlerObserver,
                                           public ClientCertResolver::Observer {
 public:
  enum AutoConnectReason {
    AUTO_CONNECT_REASON_LOGGED_IN = 1,
    AUTO_CONNECT_REASON_POLICY_APPLIED = 1 << 1,
    AUTO_CONNECT_REASON_CERTIFICATE_RESOLVED = 1 << 2
  };

  class Observer {
   public:
    // Note: |auto_connect_reasons| is computed by applying the bitwise OR
    // operation to all AutoConnectReasons which triggered auto-connect.
    virtual void OnAutoConnectedInitiated(int auto_connect_reasons) = 0;
  };

  ~AutoConnectHandler() override;

  // LoginState::Observer
  void LoggedInStateChanged() override;

  // NetworkConnectionObserver
  void ConnectToNetworkRequested(const std::string& service_path) override;

  // NetworkPolicyObserver
  void PoliciesChanged(const std::string& userhash) override;
  void PoliciesApplied(const std::string& userhash) override;

  // NetworkStateHandlerObserver
  void ScanCompleted(const DeviceState* device) override;

  // ClientCertResolver::Observer
  void ResolveRequestCompleted(bool network_properties_changed) override;

  void AddObserver(Observer* observer);
  void RemoveObserver(Observer* observer);

 protected:
  void NotifyAutoConnectInitiated(int auto_connect_reasons);

 private:
  friend class NetworkHandler;
  friend class AutoConnectHandlerTest;
  friend class ::AutoConnectNotifierTest;

  AutoConnectHandler();

  void Init(ClientCertResolver* client_cert_resolver,
            NetworkConnectionHandler* network_connection_handler,
            NetworkStateHandler* network_state_handler,
            ManagedNetworkConfigurationHandler*
                managed_network_configuration_handler);

  // If the user logged in already and the policy to prevent unmanaged & shared
  // networks to autoconnect is enabled, then disconnects all such networks
  // except wired networks. It will do this only once after the user logged in
  // and the device policy and user policy was available.
  // This is enforced once after a user logs in 1) to allow mananged networks to
  // autoconnect and 2) to prevent a previous user from foisting a network on
  // the new user. Therefore, this function is called at login and when the
  // device policy is changed after user policy is fetched and applied.
  void DisconnectIfPolicyRequires();

  // Disconnects from all unmanaged and shared WiFi networks that are currently
  // connected or connecting.
  void DisconnectFromUnmanagedSharedWiFiNetworks();

  // Requests and if possible connects to the 'best' available network, see
  // CheckBestConnection().
  void RequestBestConnection(AutoConnectReason auto_connect_reason);

  // If a request to connect to the best network is pending and all requirements
  // are fulfilled (like policy loaded, certificate patterns being resolved),
  // then this will call ConnectToBestWifiNetwork of |network_state_handler_|.
  void CheckBestConnection();

  // Calls Shill.Manager.ConnectToBestServices().
  void CallShillConnectToBestServices();

  // Local references to the associated handler instances.
  ClientCertResolver* client_cert_resolver_;
  NetworkConnectionHandler* network_connection_handler_;
  NetworkStateHandler* network_state_handler_;
  ManagedNetworkConfigurationHandler* managed_configuration_handler_;

  // Whether a request to connect to the best network is pending. If true, once
  // all requirements are met (like policy loaded, certificate patterns being
  // resolved), a scan will be requested and ConnectToBestServices will be
  // triggered once it completes.
  bool request_best_connection_pending_;

  // Whether the device policy, which might be empty, is already applied.
  bool device_policy_applied_;

  // Whether the user policy of the first user who logged in is already applied.
  // The policy might be empty.
  bool user_policy_applied_;

  // Whether at least once client certificate patterns were checked and if any
  // existed resolved. Even if there are no certificate patterns, this will be
  // eventually true.
  bool client_certs_resolved_;

  // Whether the autoconnect policy was applied already, see
  // DisconnectIfPolicyRequires().
  bool applied_autoconnect_policy_;

  // When true, trigger ConnectToBestServices after the next scan completion.
  bool connect_to_best_services_after_scan_;

  // The bitwise OR of all AutoConnectReason which have triggered auto-
  // connection.
  int auto_connect_reasons_;

  base::ObserverList<Observer> observer_list_;

  base::WeakPtrFactory<AutoConnectHandler> weak_ptr_factory_;

  DISALLOW_COPY_AND_ASSIGN(AutoConnectHandler);
};

}  // namespace chromeos

#endif  // CHROMEOS_NETWORK_AUTO_CONNECT_HANDLER_H_
