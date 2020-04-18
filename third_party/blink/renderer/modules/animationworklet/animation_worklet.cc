// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/modules/animationworklet/animation_worklet.h"

#include "third_party/blink/renderer/bindings/core/v8/v8_binding_for_core.h"
#include "third_party/blink/renderer/core/dom/animation_worklet_proxy_client.h"
#include "third_party/blink/renderer/core/dom/document.h"
#include "third_party/blink/renderer/core/page/chrome_client.h"
#include "third_party/blink/renderer/core/workers/worker_clients.h"
#include "third_party/blink/renderer/modules/animationworklet/animation_worklet_messaging_proxy.h"
#include "third_party/blink/renderer/modules/animationworklet/animation_worklet_proxy_client_impl.h"
#include "third_party/blink/renderer/modules/animationworklet/animation_worklet_thread.h"

namespace blink {

AnimationWorklet::AnimationWorklet(Document* document) : Worklet(document) {}

AnimationWorklet::~AnimationWorklet() = default;

bool AnimationWorklet::NeedsToCreateGlobalScope() {
  // For now, create only one global scope per document.
  // TODO(nhiroki): Revisit this later.
  return GetNumberOfGlobalScopes() == 0;
}

WorkletGlobalScopeProxy* AnimationWorklet::CreateGlobalScope() {
  DCHECK(NeedsToCreateGlobalScope());
  AnimationWorkletThread::EnsureSharedBackingThread();

  Document* document = ToDocument(GetExecutionContext());
  AnimationWorkletProxyClient* proxy_client =
      AnimationWorkletProxyClientImpl::FromDocument(document);

  WorkerClients* worker_clients = WorkerClients::Create();
  ProvideAnimationWorkletProxyClientTo(worker_clients, proxy_client);

  AnimationWorkletMessagingProxy* proxy =
      new AnimationWorkletMessagingProxy(GetExecutionContext());
  proxy->Initialize(worker_clients, ModuleResponsesMap());
  return proxy;
}

void AnimationWorklet::Trace(blink::Visitor* visitor) {
  Worklet::Trace(visitor);
}

}  // namespace blink
