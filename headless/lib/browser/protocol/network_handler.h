// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef HEADLESS_LIB_BROWSER_PROTOCOL_NETWORK_HANDLER_H_
#define HEADLESS_LIB_BROWSER_PROTOCOL_NETWORK_HANDLER_H_

#include "headless/lib/browser/headless_network_conditions.h"
#include "headless/lib/browser/protocol/domain_handler.h"
#include "headless/lib/browser/protocol/dp_network.h"

namespace headless {

class HeadlessBrowserContext;

namespace protocol {

class NetworkHandler : public DomainHandler, public Network::Backend {
 public:
  explicit NetworkHandler(base::WeakPtr<HeadlessBrowserImpl> browser);
  ~NetworkHandler() override;

  void Wire(UberDispatcher* dispatcher) override;

  // Network::Backend implementation
  Response EmulateNetworkConditions(
      bool offline,
      double latency,
      double download_throughput,
      double upload_throughput,
      Maybe<Network::ConnectionType> connection_type) override;
  Response Disable() override;

 private:
  void SetNetworkConditions(
      std::vector<HeadlessBrowserContext*> browser_contexts,
      HeadlessNetworkConditions conditions);
  DISALLOW_COPY_AND_ASSIGN(NetworkHandler);
};

}  // namespace protocol
}  // namespace headless

#endif  // HEADLESS_LIB_BROWSER_PROTOCOL_NETWORK_HANDLER_H_
