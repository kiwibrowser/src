// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "headless/lib/browser/protocol/network_handler.h"

#include "headless/lib/browser/headless_browser_context_impl.h"
#include "headless/lib/browser/headless_browser_impl.h"

namespace headless {
namespace protocol {

NetworkHandler::NetworkHandler(base::WeakPtr<HeadlessBrowserImpl> browser)
    : DomainHandler(Network::Metainfo::domainName, browser) {}

NetworkHandler::~NetworkHandler() = default;

void NetworkHandler::Wire(UberDispatcher* dispatcher) {
  Network::Dispatcher::wire(dispatcher, this);
}

Response NetworkHandler::EmulateNetworkConditions(
    bool offline,
    double latency,
    double download_throughput,
    double upload_throughput,
    Maybe<Network::ConnectionType>) {
  // Associate NetworkConditions to context
  std::vector<HeadlessBrowserContext*> browser_contexts =
      browser()->GetAllBrowserContexts();
  HeadlessNetworkConditions conditions(HeadlessNetworkConditions(
      offline, std::max(latency, 0.0), std::max(download_throughput, 0.0),
      std::max(upload_throughput, 0.0)));
  SetNetworkConditions(browser_contexts, conditions);
  return Response::FallThrough();
}

Response NetworkHandler::Disable() {
  // Can be a part of the shutdown cycle.
  if (!browser())
    return Response::OK();
  std::vector<HeadlessBrowserContext*> browser_contexts =
      browser()->GetAllBrowserContexts();
  SetNetworkConditions(browser_contexts, HeadlessNetworkConditions());
  return Response::FallThrough();
}

void NetworkHandler::SetNetworkConditions(
    std::vector<HeadlessBrowserContext*> browser_contexts,
    HeadlessNetworkConditions conditions) {
  for (std::vector<HeadlessBrowserContext*>::iterator it =
           browser_contexts.begin();
       it != browser_contexts.end(); ++it) {
    HeadlessBrowserContextImpl* context =
        static_cast<HeadlessBrowserContextImpl*>(*it);
    context->SetNetworkConditions(conditions);
  }
}

}  // namespace protocol
}  // namespace headless
