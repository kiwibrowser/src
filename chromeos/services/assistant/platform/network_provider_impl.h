// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROMEOS_SERVICES_ASSISTANT_PLATFORM_NETWORK_PROVIDER_IMPL_H_
#define CHROMEOS_SERVICES_ASSISTANT_PLATFORM_NETWORK_PROVIDER_IMPL_H_

#include "base/macros.h"
#include "libassistant/shared/public/platform_net.h"
#include "net/base/network_change_notifier.h"

namespace chromeos {
namespace assistant {

class NetworkProviderImpl
    : public assistant_client::NetworkProvider,
      public net::NetworkChangeNotifier::NetworkChangeObserver {
 public:
  NetworkProviderImpl();
  ~NetworkProviderImpl() override;

  // net::NetworkChangeNotifier overrides:
  void OnNetworkChanged(
      net::NetworkChangeNotifier::ConnectionType type) override;

  // assistant_client::NetworkProvider::NetworkChangeObserver overrides:
  ConnectionStatus GetConnectionStatus() override;
  assistant_client::MdnsResponder* GetMdnsResponder() override;

 private:
  net::NetworkChangeNotifier::ConnectionType connection_type_;

  DISALLOW_COPY_AND_ASSIGN(NetworkProviderImpl);
};

}  // namespace assistant
}  // namespace chromeos

#endif  // CHROMEOS_SERVICES_ASSISTANT_PLATFORM_NETWORK_PROVIDER_IMPL_H_
