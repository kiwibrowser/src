// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/permissions/permission_service_context.h"

#include <utility>

#include "content/browser/permissions/permission_service_impl.h"
#include "content/public/browser/browser_context.h"
#include "content/public/browser/navigation_handle.h"
#include "content/public/browser/permission_manager.h"
#include "content/public/browser/render_frame_host.h"
#include "content/public/browser/render_process_host.h"
#include "content/public/browser/web_contents.h"

using blink::mojom::PermissionObserverPtr;

namespace content {

class PermissionServiceContext::PermissionSubscription {
 public:
  PermissionSubscription(PermissionServiceContext* context,
                         PermissionObserverPtr observer)
      : context_(context), observer_(std::move(observer)) {
    observer_.set_connection_error_handler(base::BindOnce(
        &PermissionSubscription::OnConnectionError, base::Unretained(this)));
  }

  ~PermissionSubscription() {
    DCHECK_NE(id_, 0);
    BrowserContext* browser_context = context_->GetBrowserContext();
    if (browser_context && browser_context->GetPermissionManager()) {
      browser_context->GetPermissionManager()
          ->UnsubscribePermissionStatusChange(id_);
    }
  }

  void OnConnectionError() {
    DCHECK_NE(id_, 0);
    context_->ObserverHadConnectionError(id_);
  }

  void OnPermissionStatusChanged(blink::mojom::PermissionStatus status) {
    observer_->OnPermissionStatusChange(status);
  }

  void set_id(int id) { id_ = id; }

 private:
  PermissionServiceContext* context_;
  PermissionObserverPtr observer_;
  int id_ = 0;
};

PermissionServiceContext::PermissionServiceContext(
    RenderFrameHost* render_frame_host)
    : WebContentsObserver(WebContents::FromRenderFrameHost(render_frame_host)),
      render_frame_host_(render_frame_host),
      render_process_host_(nullptr) {
}

PermissionServiceContext::PermissionServiceContext(
    RenderProcessHost* render_process_host)
    : WebContentsObserver(nullptr),
      render_frame_host_(nullptr),
      render_process_host_(render_process_host) {
}

PermissionServiceContext::~PermissionServiceContext() {
}

void PermissionServiceContext::CreateService(
    blink::mojom::PermissionServiceRequest request) {
  DCHECK(render_frame_host_);
  services_.AddBinding(std::make_unique<PermissionServiceImpl>(
                           this, render_frame_host_->GetLastCommittedOrigin()),
                       std::move(request));
}

void PermissionServiceContext::CreateServiceForWorker(
    blink::mojom::PermissionServiceRequest request,
    const url::Origin& origin) {
  services_.AddBinding(std::make_unique<PermissionServiceImpl>(this, origin),
                       std::move(request));
}

void PermissionServiceContext::CreateSubscription(
    PermissionType permission_type,
    const url::Origin& origin,
    PermissionObserverPtr observer) {
  BrowserContext* browser_context = GetBrowserContext();
  if (!browser_context || !browser_context->GetPermissionManager())
    return;

  auto subscription =
      std::make_unique<PermissionSubscription>(this, std::move(observer));
  GURL requesting_origin(origin.Serialize());
  GURL embedding_origin = GetEmbeddingOrigin();
  int subscription_id =
      browser_context->GetPermissionManager()->SubscribePermissionStatusChange(
          permission_type, requesting_origin,
          // If the embedding_origin is empty, we'll use the |origin| instead.
          embedding_origin.is_empty() ? requesting_origin : embedding_origin,
          base::Bind(&PermissionSubscription::OnPermissionStatusChanged,
                     base::Unretained(subscription.get())));
  subscription->set_id(subscription_id);
  subscriptions_[subscription_id] = std::move(subscription);
}

void PermissionServiceContext::ObserverHadConnectionError(int subscription_id) {
  auto it = subscriptions_.find(subscription_id);
  DCHECK(it != subscriptions_.end());
  subscriptions_.erase(it);
}

void PermissionServiceContext::RenderFrameHostChanged(
    RenderFrameHost* old_host,
    RenderFrameHost* new_host) {
  CloseBindings(old_host);
}

void PermissionServiceContext::FrameDeleted(
    RenderFrameHost* render_frame_host) {
  CloseBindings(render_frame_host);
}

void PermissionServiceContext::DidFinishNavigation(
    NavigationHandle* navigation_handle) {
  if (!navigation_handle->HasCommitted() || navigation_handle->IsSameDocument())
    return;

  CloseBindings(navigation_handle->GetRenderFrameHost());
}

void PermissionServiceContext::CloseBindings(
    RenderFrameHost* render_frame_host) {
  DCHECK(render_frame_host_);
  if (render_frame_host != render_frame_host_)
    return;

  services_.CloseAllBindings();
  subscriptions_.clear();
}

BrowserContext* PermissionServiceContext::GetBrowserContext() const {
  // web_contents() may return nullptr during teardown, or when showing
  // an interstitial.
  if (web_contents())
    return web_contents()->GetBrowserContext();

  if (render_process_host_)
    return render_process_host_->GetBrowserContext();

  return nullptr;
}

GURL PermissionServiceContext::GetEmbeddingOrigin() const {
  return web_contents() ? web_contents()->GetLastCommittedURL().GetOrigin()
                        : GURL();
}

RenderFrameHost* PermissionServiceContext::render_frame_host() const {
  return render_frame_host_;
}

} // namespace content
