// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef THIRD_PARTY_BLINK_RENDERER_CORE_DOM_ANIMATION_WORKLET_PROXY_CLIENT_H_
#define THIRD_PARTY_BLINK_RENDERER_CORE_DOM_ANIMATION_WORKLET_PROXY_CLIENT_H_

#include "base/macros.h"
#include "third_party/blink/renderer/core/core_export.h"
#include "third_party/blink/renderer/core/workers/worker_clients.h"

namespace blink {

class WorkletGlobalScope;

class CORE_EXPORT AnimationWorkletProxyClient
    : public Supplement<WorkerClients> {
 public:
  static const char kSupplementName[];

  AnimationWorkletProxyClient() = default;

  static AnimationWorkletProxyClient* From(WorkerClients*);

  virtual void SetGlobalScope(WorkletGlobalScope*) = 0;
  virtual void Dispose() = 0;
  DISALLOW_COPY_AND_ASSIGN(AnimationWorkletProxyClient);
};

CORE_EXPORT void ProvideAnimationWorkletProxyClientTo(
    WorkerClients*,
    AnimationWorkletProxyClient*);

}  // namespace blink

#endif  // THIRD_PARTY_BLINK_RENDERER_CORE_DOM_ANIMATION_WORKLET_PROXY_CLIENT_H_
