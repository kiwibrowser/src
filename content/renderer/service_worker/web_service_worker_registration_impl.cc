// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/renderer/service_worker/web_service_worker_registration_impl.h"

#include <utility>

#include "base/macros.h"
#include "content/child/child_process.h"
#include "content/common/service_worker/service_worker_types.h"
#include "content/renderer/service_worker/service_worker_context_client.h"
#include "content/renderer/service_worker/service_worker_provider_context.h"
#include "content/renderer/service_worker/web_service_worker_impl.h"
#include "content/renderer/service_worker/web_service_worker_provider_impl.h"
#include "third_party/blink/public/platform/modules/serviceworker/web_navigation_preload_state.h"
#include "third_party/blink/public/platform/modules/serviceworker/web_service_worker_error.h"
#include "third_party/blink/public/platform/modules/serviceworker/web_service_worker_registration_proxy.h"

namespace content {

namespace {

class ServiceWorkerRegistrationHandleImpl
    : public blink::WebServiceWorkerRegistration::Handle {
 public:
  explicit ServiceWorkerRegistrationHandleImpl(
      scoped_refptr<WebServiceWorkerRegistrationImpl> registration)
      : registration_(std::move(registration)) {}
  ~ServiceWorkerRegistrationHandleImpl() override {}

  blink::WebServiceWorkerRegistration* Registration() override {
    return registration_.get();
  }

 private:
  scoped_refptr<WebServiceWorkerRegistrationImpl> registration_;

  DISALLOW_COPY_AND_ASSIGN(ServiceWorkerRegistrationHandleImpl);
};

void DidGetNavigationPreloadState(
    std::unique_ptr<
        WebServiceWorkerRegistrationImpl::WebGetNavigationPreloadStateCallbacks>
        callbacks,
    blink::mojom::ServiceWorkerErrorType error,
    const base::Optional<std::string>& error_msg,
    blink::mojom::NavigationPreloadStatePtr state) {
  if (error != blink::mojom::ServiceWorkerErrorType::kNone) {
    DCHECK(error_msg);
    callbacks->OnError(blink::WebServiceWorkerError(
        error, blink::WebString::FromUTF8(*error_msg)));
    return;
  }
  callbacks->OnSuccess(blink::WebNavigationPreloadState(
      state->enabled, blink::WebString::FromUTF8(state->header)));
}

}  // namespace

WebServiceWorkerRegistrationImpl::QueuedTask::QueuedTask(
    QueuedTaskType type,
    const scoped_refptr<WebServiceWorkerImpl>& worker)
    : type(type), worker(worker) {}

WebServiceWorkerRegistrationImpl::QueuedTask::QueuedTask(
    const QueuedTask& other) = default;

WebServiceWorkerRegistrationImpl::QueuedTask::~QueuedTask() {}

// static
scoped_refptr<WebServiceWorkerRegistrationImpl>
WebServiceWorkerRegistrationImpl::CreateForServiceWorkerGlobalScope(
    blink::mojom::ServiceWorkerRegistrationObjectInfoPtr info) {
  scoped_refptr<WebServiceWorkerRegistrationImpl> impl =
      new WebServiceWorkerRegistrationImpl(std::move(info),
                                           nullptr /* provider_context */);
  return impl;
}

// static
scoped_refptr<WebServiceWorkerRegistrationImpl>
WebServiceWorkerRegistrationImpl::CreateForServiceWorkerClient(
    blink::mojom::ServiceWorkerRegistrationObjectInfoPtr info,
    base::WeakPtr<ServiceWorkerProviderContext> provider_context) {
  DCHECK(provider_context);
  scoped_refptr<WebServiceWorkerRegistrationImpl> impl =
      new WebServiceWorkerRegistrationImpl(std::move(info),
                                           std::move(provider_context));
  return impl;
}

void WebServiceWorkerRegistrationImpl::AttachForServiceWorkerClient(
    blink::mojom::ServiceWorkerRegistrationObjectInfoPtr info) {
  DCHECK_EQ(info_->registration_id, info->registration_id);
  Attach(std::move(info));
}

void WebServiceWorkerRegistrationImpl::SetProxy(
    blink::WebServiceWorkerRegistrationProxy* proxy) {
  proxy_ = proxy;
  RunQueuedTasks();
}

void WebServiceWorkerRegistrationImpl::RunQueuedTasks() {
  DCHECK(proxy_);
  for (const QueuedTask& task : queued_tasks_) {
    if (task.type == INSTALLING)
      proxy_->SetInstalling(WebServiceWorkerImpl::CreateHandle(task.worker));
    else if (task.type == WAITING)
      proxy_->SetWaiting(WebServiceWorkerImpl::CreateHandle(task.worker));
    else if (task.type == ACTIVE)
      proxy_->SetActive(WebServiceWorkerImpl::CreateHandle(task.worker));
    else if (task.type == UPDATE_FOUND)
      proxy_->DispatchUpdateFoundEvent();
  }
  queued_tasks_.clear();
}

void WebServiceWorkerRegistrationImpl::Attach(
    blink::mojom::ServiceWorkerRegistrationObjectInfoPtr info) {
  DCHECK_NE(blink::mojom::kInvalidServiceWorkerRegistrationId,
            info->registration_id);
  DCHECK(info->host_ptr_info.is_valid());
  DCHECK(info->request.is_pending());
  if (!host_)
    host_.Bind(std::move(info->host_ptr_info));
  else
    info->host_ptr_info = nullptr;
  binding_.Close();
  binding_.Bind(std::move(info->request));

  info_ = std::move(info);
  RefreshVersionAttributes();
}

blink::WebServiceWorkerRegistrationProxy*
WebServiceWorkerRegistrationImpl::Proxy() {
  return proxy_;
}

blink::WebURL WebServiceWorkerRegistrationImpl::Scope() const {
  return info_->options->scope;
}

blink::mojom::ServiceWorkerUpdateViaCache
WebServiceWorkerRegistrationImpl::UpdateViaCache() const {
  return info_->options->update_via_cache;
}

void WebServiceWorkerRegistrationImpl::Update(
    std::unique_ptr<WebServiceWorkerUpdateCallbacks> callbacks) {
  host_->Update(base::BindOnce(
      [](std::unique_ptr<WebServiceWorkerUpdateCallbacks> callbacks,
         blink::mojom::ServiceWorkerErrorType error,
         const base::Optional<std::string>& error_msg) {
        if (error != blink::mojom::ServiceWorkerErrorType::kNone) {
          DCHECK(error_msg);
          callbacks->OnError(blink::WebServiceWorkerError(
              error, blink::WebString::FromUTF8(*error_msg)));
          return;
        }

        DCHECK(!error_msg);
        callbacks->OnSuccess();
      },
      std::move(callbacks)));
}

void WebServiceWorkerRegistrationImpl::Unregister(
    std::unique_ptr<WebServiceWorkerUnregistrationCallbacks> callbacks) {
  host_->Unregister(base::BindOnce(
      [](std::unique_ptr<WebServiceWorkerUnregistrationCallbacks> callbacks,
         blink::mojom::ServiceWorkerErrorType error,
         const base::Optional<std::string>& error_msg) {
        if (error != blink::mojom::ServiceWorkerErrorType::kNone &&
            error != blink::mojom::ServiceWorkerErrorType::kNotFound) {
          DCHECK(error_msg);
          callbacks->OnError(blink::WebServiceWorkerError(
              error, blink::WebString::FromUTF8(*error_msg)));
          return;
        }

        callbacks->OnSuccess(error ==
                             blink::mojom::ServiceWorkerErrorType::kNone);
      },
      std::move(callbacks)));
}

void WebServiceWorkerRegistrationImpl::EnableNavigationPreload(
    bool enable,
    std::unique_ptr<WebEnableNavigationPreloadCallbacks> callbacks) {
  host_->EnableNavigationPreload(
      enable,
      base::BindOnce(
          [](std::unique_ptr<WebEnableNavigationPreloadCallbacks> callbacks,
             blink::mojom::ServiceWorkerErrorType error,
             const base::Optional<std::string>& error_msg) {
            if (error != blink::mojom::ServiceWorkerErrorType::kNone) {
              DCHECK(error_msg);
              callbacks->OnError(blink::WebServiceWorkerError(
                  error, blink::WebString::FromUTF8(*error_msg)));
              return;
            }
            callbacks->OnSuccess();
          },
          std::move(callbacks)));
}

void WebServiceWorkerRegistrationImpl::GetNavigationPreloadState(
    std::unique_ptr<WebGetNavigationPreloadStateCallbacks> callbacks) {
  host_->GetNavigationPreloadState(
      base::BindOnce(&DidGetNavigationPreloadState, std::move(callbacks)));
}

void WebServiceWorkerRegistrationImpl::SetNavigationPreloadHeader(
    const blink::WebString& value,
    std::unique_ptr<WebSetNavigationPreloadHeaderCallbacks> callbacks) {
  host_->SetNavigationPreloadHeader(
      value.Utf8(),
      base::BindOnce(
          [](std::unique_ptr<WebSetNavigationPreloadHeaderCallbacks> callbacks,
             blink::mojom::ServiceWorkerErrorType error,
             const base::Optional<std::string>& error_msg) {
            if (error != blink::mojom::ServiceWorkerErrorType::kNone) {
              DCHECK(error_msg);
              callbacks->OnError(blink::WebServiceWorkerError(
                  error, blink::WebString::FromUTF8(*error_msg)));
              return;
            }
            callbacks->OnSuccess();
          },
          std::move(callbacks)));
}

int64_t WebServiceWorkerRegistrationImpl::RegistrationId() const {
  return info_->registration_id;
}

// static
std::unique_ptr<blink::WebServiceWorkerRegistration::Handle>
WebServiceWorkerRegistrationImpl::CreateHandle(
    scoped_refptr<WebServiceWorkerRegistrationImpl> registration) {
  if (!registration)
    return nullptr;
  return std::make_unique<ServiceWorkerRegistrationHandleImpl>(
      std::move(registration));
}

WebServiceWorkerRegistrationImpl::WebServiceWorkerRegistrationImpl(
    blink::mojom::ServiceWorkerRegistrationObjectInfoPtr info,
    base::WeakPtr<ServiceWorkerProviderContext> provider_context)
    : proxy_(nullptr),
      binding_(this),
      is_for_client_(provider_context),
      provider_context_for_client_(std::move(provider_context)) {
  Attach(std::move(info));
  if (is_for_client_) {
    provider_context_for_client_->AddServiceWorkerRegistrationObject(
        info_->registration_id, this);
  }
}

WebServiceWorkerRegistrationImpl::~WebServiceWorkerRegistrationImpl() {
  if (provider_context_for_client_) {
    provider_context_for_client_->RemoveServiceWorkerRegistrationObject(
        info_->registration_id);
  }
}

void WebServiceWorkerRegistrationImpl::SetInstalling(
    blink::mojom::ServiceWorkerObjectInfoPtr info) {
  scoped_refptr<WebServiceWorkerImpl> service_worker =
      GetOrCreateServiceWorkerObject(std::move(info));
  if (proxy_)
    proxy_->SetInstalling(WebServiceWorkerImpl::CreateHandle(service_worker));
  else
    queued_tasks_.push_back(QueuedTask(INSTALLING, service_worker));
}

void WebServiceWorkerRegistrationImpl::SetWaiting(
    blink::mojom::ServiceWorkerObjectInfoPtr info) {
  scoped_refptr<WebServiceWorkerImpl> service_worker =
      GetOrCreateServiceWorkerObject(std::move(info));
  if (proxy_)
    proxy_->SetWaiting(WebServiceWorkerImpl::CreateHandle(service_worker));
  else
    queued_tasks_.push_back(QueuedTask(WAITING, service_worker));
}

void WebServiceWorkerRegistrationImpl::SetActive(
    blink::mojom::ServiceWorkerObjectInfoPtr info) {
  scoped_refptr<WebServiceWorkerImpl> service_worker =
      GetOrCreateServiceWorkerObject(std::move(info));
  if (proxy_)
    proxy_->SetActive(WebServiceWorkerImpl::CreateHandle(service_worker));
  else
    queued_tasks_.push_back(QueuedTask(ACTIVE, service_worker));
}

void WebServiceWorkerRegistrationImpl::RefreshVersionAttributes() {
  SetInstalling(std::move(info_->installing));
  SetWaiting(std::move(info_->waiting));
  SetActive(std::move(info_->active));
}

scoped_refptr<WebServiceWorkerImpl>
WebServiceWorkerRegistrationImpl::GetOrCreateServiceWorkerObject(
    blink::mojom::ServiceWorkerObjectInfoPtr info) {
  scoped_refptr<WebServiceWorkerImpl> service_worker;
  if (is_for_client_) {
    if (provider_context_for_client_) {
      service_worker =
          provider_context_for_client_->GetOrCreateServiceWorkerObject(
              std::move(info));
    }
  } else if (ServiceWorkerContextClient::ThreadSpecificInstance()) {
    service_worker = ServiceWorkerContextClient::ThreadSpecificInstance()
                         ->GetOrCreateServiceWorkerObject(std::move(info));
  }
  return service_worker;
}

void WebServiceWorkerRegistrationImpl::SetVersionAttributes(
    int changed_mask,
    blink::mojom::ServiceWorkerObjectInfoPtr installing,
    blink::mojom::ServiceWorkerObjectInfoPtr waiting,
    blink::mojom::ServiceWorkerObjectInfoPtr active) {
  ChangedVersionAttributesMask mask(changed_mask);
  DCHECK(mask.installing_changed() || !installing);
  if (mask.installing_changed()) {
    SetInstalling(std::move(installing));
  }
  DCHECK(mask.waiting_changed() || !waiting);
  if (mask.waiting_changed()) {
    SetWaiting(std::move(waiting));
  }
  DCHECK(mask.active_changed() || !active);
  if (mask.active_changed()) {
    SetActive(std::move(active));
  }
}

void WebServiceWorkerRegistrationImpl::SetUpdateViaCache(
    blink::mojom::ServiceWorkerUpdateViaCache update_via_cache) {
  info_->options->update_via_cache = update_via_cache;
}

void WebServiceWorkerRegistrationImpl::UpdateFound() {
  if (proxy_)
    proxy_->DispatchUpdateFoundEvent();
  else
    queued_tasks_.push_back(QueuedTask(UPDATE_FOUND, nullptr));
}

}  // namespace content
