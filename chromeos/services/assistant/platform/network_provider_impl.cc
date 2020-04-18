// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chromeos/services/assistant/platform/network_provider_impl.h"

using assistant_client::NetworkProvider;
using ConnectionStatus = assistant_client::NetworkProvider::ConnectionStatus;

namespace chromeos {
namespace assistant {

NetworkProviderImpl::NetworkProviderImpl()
    : connection_type_(net::NetworkChangeNotifier::GetConnectionType()) {
  net::NetworkChangeNotifier::AddNetworkChangeObserver(this);
}

NetworkProviderImpl::~NetworkProviderImpl() = default;

void NetworkProviderImpl::OnNetworkChanged(
    net::NetworkChangeNotifier::ConnectionType type) {
  connection_type_ = type;
}

ConnectionStatus NetworkProviderImpl::GetConnectionStatus() {
  // TODO(updowndota): Check actual internect connectivity in addition to the
  // physical connectivity.
  switch (connection_type_) {
    case net::NetworkChangeNotifier::CONNECTION_UNKNOWN:
      return ConnectionStatus::UNKNOWN;
    case net::NetworkChangeNotifier::CONNECTION_ETHERNET:
    case net::NetworkChangeNotifier::CONNECTION_WIFI:
    case net::NetworkChangeNotifier::CONNECTION_2G:
    case net::NetworkChangeNotifier::CONNECTION_3G:
    case net::NetworkChangeNotifier::CONNECTION_4G:
    case net::NetworkChangeNotifier::CONNECTION_BLUETOOTH:
      return ConnectionStatus::CONNECTED;
    case net::NetworkChangeNotifier::CONNECTION_NONE:
      return ConnectionStatus::DISCONNECTED_FROM_INTERNET;
  }
}

// Mdns responder is not supported in ChromeOS.
assistant_client::MdnsResponder* NetworkProviderImpl::GetMdnsResponder() {
  return nullptr;
}

}  // namespace assistant
}  // namespace chromeos
