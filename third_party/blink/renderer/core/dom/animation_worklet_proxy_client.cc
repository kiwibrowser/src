// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/core/dom/animation_worklet_proxy_client.h"

namespace blink {

AnimationWorkletProxyClient* AnimationWorkletProxyClient::From(
    WorkerClients* clients) {
  return Supplement<WorkerClients>::From<AnimationWorkletProxyClient>(clients);
}

const char AnimationWorkletProxyClient::kSupplementName[] =
    "AnimationWorkletProxyClient";

void ProvideAnimationWorkletProxyClientTo(WorkerClients* clients,
                                          AnimationWorkletProxyClient* client) {
  clients->ProvideSupplement(client);
}

}  // namespace blink
