// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/renderer/shared_worker/shared_worker_client_impl.h"

#include "content/child/child_thread_impl.h"
#include "content/common/view_messages.h"
#include "mojo/public/cpp/bindings/strong_binding.h"
#include "third_party/blink/public/platform/web_feature.mojom.h"

namespace content {

SharedWorkerClientImpl::SharedWorkerClientImpl(
    std::unique_ptr<blink::WebSharedWorkerConnectListener> listener)
    : listener_(std::move(listener)) {}

SharedWorkerClientImpl::~SharedWorkerClientImpl() = default;

void SharedWorkerClientImpl::OnCreated(
    blink::mojom::SharedWorkerCreationContextType creation_context_type) {
  listener_->WorkerCreated(creation_context_type);
}

void SharedWorkerClientImpl::OnConnected(
    const std::vector<blink::mojom::WebFeature>& features_used) {
  listener_->Connected();
  for (auto feature : features_used)
    listener_->CountFeature(feature);
}

void SharedWorkerClientImpl::OnScriptLoadFailed() {
  listener_->ScriptLoadFailed();
}

void SharedWorkerClientImpl::OnFeatureUsed(blink::mojom::WebFeature feature) {
  listener_->CountFeature(feature);
}

}  // namespace content
