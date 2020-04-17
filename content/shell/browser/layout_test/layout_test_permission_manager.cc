// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/shell/browser/layout_test/layout_test_permission_manager.h"

#include <list>
#include <memory>
#include <utility>

#include "base/bind.h"
#include "base/callback.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/permission_controller.h"
#include "content/public/browser/permission_type.h"
#include "content/public/browser/web_contents.h"
#include "content/shell/browser/layout_test/layout_test_content_browser_client.h"

namespace content {

struct LayoutTestPermissionManager::Subscription {
  PermissionDescription permission;
  base::Callback<void(blink::mojom::PermissionStatus)> callback;
  blink::mojom::PermissionStatus current_value;
};

LayoutTestPermissionManager::PermissionDescription::PermissionDescription(
    PermissionType type,
    const GURL& origin,
    const GURL& embedding_origin)
    : type(type),
      origin(origin),
      embedding_origin(embedding_origin) {
}

bool LayoutTestPermissionManager::PermissionDescription::operator==(
    const PermissionDescription& other) const {
  return type == other.type &&
         origin == other.origin &&
         embedding_origin == other.embedding_origin;
}

bool LayoutTestPermissionManager::PermissionDescription::operator!=(
    const PermissionDescription& other) const {
  return !this->operator==(other);
}

size_t LayoutTestPermissionManager::PermissionDescription::Hash::operator()(
    const PermissionDescription& description) const {
  size_t hash = BASE_HASH_NAMESPACE::hash<int>()(static_cast<int>(
      description.type));
  hash += BASE_HASH_NAMESPACE::hash<std::string>()(
      description.embedding_origin.spec());
  hash += BASE_HASH_NAMESPACE::hash<std::string>()(
      description.origin.spec());
  return hash;
}

LayoutTestPermissionManager::LayoutTestPermissionManager()
    : PermissionControllerDelegate() {}

LayoutTestPermissionManager::~LayoutTestPermissionManager() {
}

int LayoutTestPermissionManager::RequestPermission(
    PermissionType permission,
    RenderFrameHost* render_frame_host,
    const GURL& requesting_origin,
    bool user_gesture,
    const base::Callback<void(blink::mojom::PermissionStatus)>& callback) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);

  callback.Run(GetPermissionStatus(
      permission, requesting_origin,
      WebContents::FromRenderFrameHost(render_frame_host)
          ->GetLastCommittedURL().GetOrigin()));
  return PermissionController::kNoPendingOperation;
}

int LayoutTestPermissionManager::RequestPermissions(
    const std::vector<PermissionType>& permissions,
    content::RenderFrameHost* render_frame_host,
    const GURL& requesting_origin,
    bool user_gesture,
    const base::Callback<
        void(const std::vector<blink::mojom::PermissionStatus>&)>& callback) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);

  std::vector<blink::mojom::PermissionStatus> result;
  result.reserve(permissions.size());
  const GURL& embedding_origin =
      WebContents::FromRenderFrameHost(render_frame_host)
          ->GetLastCommittedURL().GetOrigin();
  for (const auto& permission : permissions) {
    result.push_back(GetPermissionStatus(
        permission, requesting_origin, embedding_origin));
  }

  callback.Run(result);
  return PermissionController::kNoPendingOperation;
}

void LayoutTestPermissionManager::ResetPermission(
    PermissionType permission,
    const GURL& requesting_origin,
    const GURL& embedding_origin) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);

  base::AutoLock lock(permissions_lock_);

  auto it = permissions_.find(
      PermissionDescription(permission, requesting_origin, embedding_origin));
  if (it == permissions_.end())
    return;
  permissions_.erase(it);
}

blink::mojom::PermissionStatus LayoutTestPermissionManager::GetPermissionStatus(
    PermissionType permission,
    const GURL& requesting_origin,
    const GURL& embedding_origin) {
  DCHECK(BrowserThread::CurrentlyOn(BrowserThread::UI) ||
         BrowserThread::CurrentlyOn(BrowserThread::IO));

  base::AutoLock lock(permissions_lock_);

  auto it = permissions_.find(
      PermissionDescription(permission, requesting_origin, embedding_origin));
  if (it == permissions_.end())
    return blink::mojom::PermissionStatus::DENIED;

  // Immitates the behaviour of the NotificationPermissionContext in that
  // permission cannot be requested from cross-origin iframes, which the current
  // permission status should reflect when it's status is ASK.
  if (permission == PermissionType::NOTIFICATIONS) {
    if (requesting_origin != embedding_origin &&
        it->second == blink::mojom::PermissionStatus::ASK) {
      return blink::mojom::PermissionStatus::DENIED;
    }
  }

  return it->second;
}

blink::mojom::PermissionStatus
LayoutTestPermissionManager::GetPermissionStatusForFrame(
    PermissionType permission,
    content::RenderFrameHost* render_frame_host,
    const GURL& requesting_origin) {
  return GetPermissionStatus(
      permission, requesting_origin,
      content::WebContents::FromRenderFrameHost(render_frame_host)
          ->GetLastCommittedURL()
          .GetOrigin());
}

int LayoutTestPermissionManager::SubscribePermissionStatusChange(
    PermissionType permission,
    RenderFrameHost* render_frame_host,
    const GURL& requesting_origin,
    const base::Callback<void(blink::mojom::PermissionStatus)>& callback) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);

  // If the request is from a worker, it won't have a RFH.
  GURL embedding_origin = requesting_origin;
  if (render_frame_host) {
    WebContents* web_contents =
        WebContents::FromRenderFrameHost(render_frame_host);
    embedding_origin = web_contents->GetLastCommittedURL().GetOrigin();
  }

  auto subscription = std::make_unique<Subscription>();
  subscription->permission =
      PermissionDescription(permission, requesting_origin, embedding_origin);
  subscription->callback = callback;
  subscription->current_value =
      GetPermissionStatus(permission,
                          subscription->permission.origin,
                          subscription->permission.embedding_origin);

  return subscriptions_.Add(std::move(subscription));
}

void LayoutTestPermissionManager::UnsubscribePermissionStatusChange(
    int subscription_id) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);

  // Whether |subscription_id| is known will be checked by the Remove() call.
  subscriptions_.Remove(subscription_id);
}

void LayoutTestPermissionManager::SetPermission(
    PermissionType permission,
    blink::mojom::PermissionStatus status,
    const GURL& url,
    const GURL& embedding_url) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);

  PermissionDescription description(permission, url.GetOrigin(),
                                    embedding_url.GetOrigin());

  base::AutoLock lock(permissions_lock_);

  auto it = permissions_.find(description);
  if (it == permissions_.end()) {
    permissions_.insert(
        std::pair<PermissionDescription, blink::mojom::PermissionStatus>(
            description, status));
  } else {
    it->second = status;
  }

  OnPermissionChanged(description, status);
}

void LayoutTestPermissionManager::ResetPermissions() {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);

  base::AutoLock lock(permissions_lock_);
  permissions_.clear();
}

void LayoutTestPermissionManager::OnPermissionChanged(
    const PermissionDescription& permission,
    blink::mojom::PermissionStatus status) {
  std::list<base::Closure> callbacks;

  for (SubscriptionsMap::iterator iter(&subscriptions_);
       !iter.IsAtEnd(); iter.Advance()) {
    Subscription* subscription = iter.GetCurrentValue();
    if (subscription->permission != permission)
      continue;

    if (subscription->current_value == status)
      continue;

    subscription->current_value = status;

    // Add the callback to |callbacks| which will be run after the loop to
    // prevent re-entrance issues.
    callbacks.push_back(base::Bind(subscription->callback, status));
  }

  for (const auto& callback : callbacks)
    callback.Run();
}

}  // namespace content
