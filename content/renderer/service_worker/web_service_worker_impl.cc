// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/renderer/service_worker/web_service_worker_impl.h"

#include <utility>

#include "base/macros.h"
#include "content/common/service_worker/service_worker_messages.h"
#include "content/renderer/service_worker/service_worker_context_client.h"
#include "content/renderer/service_worker/service_worker_provider_context.h"
#include "content/renderer/service_worker/web_service_worker_provider_impl.h"
#include "third_party/blink/public/platform/modules/serviceworker/web_service_worker_proxy.h"
#include "third_party/blink/public/platform/web_runtime_features.h"
#include "third_party/blink/public/platform/web_string.h"

using blink::WebString;

namespace content {

namespace {

class ServiceWorkerHandleImpl : public blink::WebServiceWorker::Handle {
 public:
  explicit ServiceWorkerHandleImpl(scoped_refptr<WebServiceWorkerImpl> worker)
      : worker_(std::move(worker)) {}
  ~ServiceWorkerHandleImpl() override {}

  blink::WebServiceWorker* ServiceWorker() override { return worker_.get(); }

 private:
  scoped_refptr<WebServiceWorkerImpl> worker_;

  DISALLOW_COPY_AND_ASSIGN(ServiceWorkerHandleImpl);
};

void OnTerminated(
    std::unique_ptr<WebServiceWorkerImpl::TerminateForTestingCallback>
        callback) {
  callback->OnSuccess();
}

}  // namespace

// static
scoped_refptr<WebServiceWorkerImpl>
WebServiceWorkerImpl::CreateForServiceWorkerGlobalScope(
    blink::mojom::ServiceWorkerObjectInfoPtr info) {
  scoped_refptr<WebServiceWorkerImpl> impl =
      new WebServiceWorkerImpl(std::move(info), nullptr /* provider_context */);
  return impl;
}

// static
scoped_refptr<WebServiceWorkerImpl>
WebServiceWorkerImpl::CreateForServiceWorkerClient(
    blink::mojom::ServiceWorkerObjectInfoPtr info,
    base::WeakPtr<ServiceWorkerProviderContext> provider_context) {
  DCHECK(provider_context);
  scoped_refptr<WebServiceWorkerImpl> impl =
      new WebServiceWorkerImpl(std::move(info), std::move(provider_context));
  return impl;
}

void WebServiceWorkerImpl::StateChanged(
    blink::mojom::ServiceWorkerState new_state) {
  state_ = new_state;

  // TODO(nhiroki): This is a quick fix for http://crbug.com/507110
  DCHECK(proxy_);
  if (proxy_)
    proxy_->DispatchStateChangeEvent();
}

void WebServiceWorkerImpl::SetProxy(blink::WebServiceWorkerProxy* proxy) {
  proxy_ = proxy;
}

blink::WebServiceWorkerProxy* WebServiceWorkerImpl::Proxy() {
  return proxy_;
}

blink::WebURL WebServiceWorkerImpl::Url() const {
  return info_->url;
}

blink::mojom::ServiceWorkerState WebServiceWorkerImpl::GetState() const {
  return state_;
}

void WebServiceWorkerImpl::PostMessageToServiceWorker(
    blink::TransferableMessage message) {
  host_->PostMessageToServiceWorker(std::move(message));
}

void WebServiceWorkerImpl::TerminateForTesting(
    std::unique_ptr<TerminateForTestingCallback> callback) {
  host_->TerminateForTesting(
      base::BindOnce(&OnTerminated, std::move(callback)));
}

// static
std::unique_ptr<blink::WebServiceWorker::Handle>
WebServiceWorkerImpl::CreateHandle(scoped_refptr<WebServiceWorkerImpl> worker) {
  if (!worker)
    return nullptr;
  return std::make_unique<ServiceWorkerHandleImpl>(std::move(worker));
}

WebServiceWorkerImpl::WebServiceWorkerImpl(
    blink::mojom::ServiceWorkerObjectInfoPtr info,
    base::WeakPtr<ServiceWorkerProviderContext> provider_context)
    : binding_(this),
      info_(std::move(info)),
      state_(info_->state),
      proxy_(nullptr),
      is_for_client_(provider_context),
      context_for_client_(std::move(provider_context)) {
  DCHECK_NE(blink::mojom::kInvalidServiceWorkerVersionId, info_->version_id);
  host_.Bind(std::move(info_->host_ptr_info));
  binding_.Bind(std::move(info_->request));

  if (is_for_client_) {
    context_for_client_->AddServiceWorkerObject(info_->version_id, this);
  } else {
    ServiceWorkerContextClient::ThreadSpecificInstance()
        ->AddServiceWorkerObject(info_->version_id, this);
  }
}

WebServiceWorkerImpl::~WebServiceWorkerImpl() {
  if (is_for_client_) {
    if (context_for_client_) {
      context_for_client_->RemoveServiceWorkerObject(info_->version_id);
    }
  } else {
    if (ServiceWorkerContextClient::ThreadSpecificInstance()) {
      ServiceWorkerContextClient::ThreadSpecificInstance()
          ->RemoveServiceWorkerObject(info_->version_id);
    }
  }
}

}  // namespace content
