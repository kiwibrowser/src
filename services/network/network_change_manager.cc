// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "services/network/network_change_manager.h"

#include <algorithm>
#include <utility>

#include "base/logging.h"
#include "net/base/network_change_notifier.h"

namespace network {

NetworkChangeManager::NetworkChangeManager(
    std::unique_ptr<net::NetworkChangeNotifier> network_change_notifier)
    : network_change_notifier_(std::move(network_change_notifier)) {
  net::NetworkChangeNotifier::AddNetworkChangeObserver(this);
  connection_type_ =
      mojom::ConnectionType(net::NetworkChangeNotifier::GetConnectionType());
}

NetworkChangeManager::~NetworkChangeManager() {
  net::NetworkChangeNotifier::RemoveNetworkChangeObserver(this);
}

void NetworkChangeManager::AddRequest(
    mojom::NetworkChangeManagerRequest request) {
  bindings_.AddBinding(this, std::move(request));
}

void NetworkChangeManager::RequestNotifications(
    mojom::NetworkChangeManagerClientPtr client_ptr) {
  client_ptr.set_connection_error_handler(
      base::Bind(&NetworkChangeManager::NotificationPipeBroken,
                 // base::Unretained is safe as destruction of the
                 // NetworkChangeManager will also destroy the
                 // |clients_| list (which this object will be
                 // inserted into, below), which will destroy the
                 // client_ptr, rendering this callback moot.
                 base::Unretained(this), base::Unretained(client_ptr.get())));
  client_ptr->OnInitialConnectionType(connection_type_);
  clients_.push_back(std::move(client_ptr));
}

size_t NetworkChangeManager::GetNumClientsForTesting() const {
  return clients_.size();
}

void NetworkChangeManager::NotificationPipeBroken(
    mojom::NetworkChangeManagerClient* client) {
  clients_.erase(
      std::find_if(clients_.begin(), clients_.end(),
                   [client](mojom::NetworkChangeManagerClientPtr& ptr) {
                     return ptr.get() == client;
                   }));
}

void NetworkChangeManager::OnNetworkChanged(
    net::NetworkChangeNotifier::ConnectionType type) {
  connection_type_ = mojom::ConnectionType(type);
  for (const auto& client : clients_) {
    client->OnNetworkChanged(connection_type_);
  }
}

}  // namespace network
