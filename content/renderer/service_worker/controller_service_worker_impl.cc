// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/renderer/service_worker/controller_service_worker_impl.h"

#include "content/common/service_worker/service_worker_utils.h"
#include "content/renderer/service_worker/service_worker_context_client.h"

namespace content {

ControllerServiceWorkerImpl::ControllerServiceWorkerImpl(
    mojom::ControllerServiceWorkerRequest request,
    base::WeakPtr<ServiceWorkerContextClient> context_client)
    : context_client_(std::move(context_client)) {
  CHECK(ServiceWorkerUtils::IsServicificationEnabled());
  bindings_.AddBinding(this, std::move(request));
}

ControllerServiceWorkerImpl::~ControllerServiceWorkerImpl() = default;

void ControllerServiceWorkerImpl::Clone(
    mojom::ControllerServiceWorkerRequest request) {
  bindings_.AddBinding(this, std::move(request));
}

void ControllerServiceWorkerImpl::DispatchFetchEvent(
    mojom::DispatchFetchEventParamsPtr params,
    mojom::ServiceWorkerFetchResponseCallbackPtr response_callback,
    DispatchFetchEventCallback callback) {
  DCHECK(context_client_);
  context_client_->DispatchOrQueueFetchEvent(
      std::move(params), std::move(response_callback), std::move(callback));
}

}  // namespace content
