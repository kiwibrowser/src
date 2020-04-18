// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_NETWORK_HINTS_RENDERER_PRESCIENT_NETWORKING_DISPATCHER_H_
#define COMPONENTS_NETWORK_HINTS_RENDERER_PRESCIENT_NETWORKING_DISPATCHER_H_

#include "base/macros.h"
#include "components/network_hints/renderer/renderer_dns_prefetch.h"
#include "components/network_hints/renderer/renderer_preconnect.h"
#include "third_party/blink/public/platform/web_prescient_networking.h"

namespace network_hints {

// The main entry point from blink for sending DNS prefetch requests to the
// network stack.
class PrescientNetworkingDispatcher : public blink::WebPrescientNetworking {
 public:
  PrescientNetworkingDispatcher();
  ~PrescientNetworkingDispatcher() override;

  void PrefetchDNS(const blink::WebString& hostname) override;
  void Preconnect(const blink::WebURL& url,
                  const bool allow_credentials) override;

 private:
  network_hints::RendererDnsPrefetch dns_prefetch_;
  network_hints::RendererPreconnect preconnect_;

  DISALLOW_COPY_AND_ASSIGN(PrescientNetworkingDispatcher);
};

}   // namespace network_hints

#endif  // COMPONENTS_NETWORK_HINTS_RENDERER_PRESCIENT_NETWORKING_DISPATCHER_H_
