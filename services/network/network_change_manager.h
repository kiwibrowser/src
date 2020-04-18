// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SERVICES_NETWORK_NETWORK_CHANGE_MANAGER_H_
#define SERVICES_NETWORK_NETWORK_CHANGE_MANAGER_H_

#include <memory>
#include <string>
#include <vector>

#include "base/component_export.h"
#include "base/macros.h"
#include "build/build_config.h"
#include "mojo/public/cpp/bindings/binding_set.h"
#include "net/base/network_change_notifier.h"
#include "services/network/public/mojom/network_change_manager.mojom.h"

namespace network {

// Implementation of mojom::NetworkChangeManager. All accesses to this class are
// done through mojo on the main thread. This registers itself to receive
// broadcasts from net::NetworkChangeNotifier and rebroadcasts the notifications
// to mojom::NetworkChangeManagerClients through mojo pipes.
class COMPONENT_EXPORT(NETWORK_SERVICE) NetworkChangeManager
    : public mojom::NetworkChangeManager,
      public net::NetworkChangeNotifier::NetworkChangeObserver {
 public:
  // If |network_change_notifier| is not null, |this| will take ownership of it.
  // Otherwise, the global net::NetworkChangeNotifier will be used.
  explicit NetworkChangeManager(
      std::unique_ptr<net::NetworkChangeNotifier> network_change_notifier);

  ~NetworkChangeManager() override;

  // Binds a NetworkChangeManager request to this object. Mojo messages
  // coming through the associated pipe will be served by this object.
  void AddRequest(mojom::NetworkChangeManagerRequest request);

  // mojom::NetworkChangeManager implementation:
  void RequestNotifications(
      mojom::NetworkChangeManagerClientPtr client_ptr) override;

  size_t GetNumClientsForTesting() const;

 private:
  // Handles connection errors on notification pipes.
  void NotificationPipeBroken(mojom::NetworkChangeManagerClient* client);

  // net::NetworkChangeNotifier::NetworkChangeObserver implementation:
  void OnNetworkChanged(
      net::NetworkChangeNotifier::ConnectionType type) override;

  std::unique_ptr<net::NetworkChangeNotifier> network_change_notifier_;
  mojo::BindingSet<mojom::NetworkChangeManager> bindings_;
  std::vector<mojom::NetworkChangeManagerClientPtr> clients_;
  mojom::ConnectionType connection_type_;

  DISALLOW_COPY_AND_ASSIGN(NetworkChangeManager);
};

}  // namespace network

#endif  // SERVICES_NETWORK_NETWORK_CHANGE_MANAGER_H_
