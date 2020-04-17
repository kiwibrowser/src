// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_PERMISSIONS_PERMISSION_CONTROLLER_IMPL_CC_
#define CONTENT_BROWSER_PERMISSIONS_PERMISSION_CONTROLLER_IMPL_CC_

#include "content/browser/permissions/permission_controller_impl.h"
#include "content/common/content_export.h"
#include "content/public/browser/browser_context.h"
#include "content/public/browser/permission_controller_delegate.h"
#include "third_party/blink/public/platform/modules/permissions/permission_status.mojom.h"

class GURL;

namespace content {

PermissionControllerImpl::PermissionControllerImpl(
    BrowserContext* browser_context)
    : browser_context_(browser_context) {}

// static
PermissionControllerImpl* PermissionControllerImpl::FromBrowserContext(
    BrowserContext* browser_context) {
  return static_cast<PermissionControllerImpl*>(
      BrowserContext::GetPermissionController(browser_context));
}

PermissionControllerImpl::~PermissionControllerImpl() = default;

int PermissionControllerImpl::RequestPermission(
    PermissionType permission,
    RenderFrameHost* render_frame_host,
    const GURL& requesting_origin,
    bool user_gesture,
    const base::Callback<void(blink::mojom::PermissionStatus)>& callback) {
  PermissionControllerDelegate* delegate =
      browser_context_->GetPermissionControllerDelegate();
  if (!delegate) {
    callback.Run(blink::mojom::PermissionStatus::DENIED);
    return kNoPendingOperation;
  }
  return delegate->RequestPermission(permission, render_frame_host,
                                     requesting_origin, user_gesture, callback);
}

int PermissionControllerImpl::RequestPermissions(
    const std::vector<PermissionType>& permissions,
    RenderFrameHost* render_frame_host,
    const GURL& requesting_origin,
    bool user_gesture,
    const base::Callback<
        void(const std::vector<blink::mojom::PermissionStatus>&)>& callback) {
  PermissionControllerDelegate* delegate =
      browser_context_->GetPermissionControllerDelegate();
  if (!delegate) {
    std::vector<blink::mojom::PermissionStatus> result(
        permissions.size(), blink::mojom::PermissionStatus::DENIED);
    callback.Run(result);
    return kNoPendingOperation;
  }
  return delegate->RequestPermissions(permissions, render_frame_host,
                                      requesting_origin, user_gesture,
                                      callback);
}

blink::mojom::PermissionStatus PermissionControllerImpl::GetPermissionStatus(
    PermissionType permission,
    const GURL& requesting_origin,
    const GURL& embedding_origin) {
  PermissionControllerDelegate* delegate =
      browser_context_->GetPermissionControllerDelegate();
  if (!delegate)
    return blink::mojom::PermissionStatus::DENIED;
  return delegate->GetPermissionStatus(permission, requesting_origin,
                                       embedding_origin);
}

blink::mojom::PermissionStatus
PermissionControllerImpl::GetPermissionStatusForFrame(
    PermissionType permission,
    RenderFrameHost* render_frame_host,
    const GURL& requesting_origin) {
  PermissionControllerDelegate* delegate =
      browser_context_->GetPermissionControllerDelegate();
  if (!delegate)
    return blink::mojom::PermissionStatus::DENIED;
  return delegate->GetPermissionStatusForFrame(permission, render_frame_host,
                                               requesting_origin);
}

void PermissionControllerImpl::ResetPermission(PermissionType permission,
                                               const GURL& requesting_origin,
                                               const GURL& embedding_origin) {
  PermissionControllerDelegate* delegate =
      browser_context_->GetPermissionControllerDelegate();
  if (!delegate)
    return;
  delegate->ResetPermission(permission, requesting_origin, embedding_origin);
}

int PermissionControllerImpl::SubscribePermissionStatusChange(
    PermissionType permission,
    RenderFrameHost* render_frame_host,
    const GURL& requesting_origin,
    const base::Callback<void(blink::mojom::PermissionStatus)>& callback) {
  PermissionControllerDelegate* delegate =
      browser_context_->GetPermissionControllerDelegate();
  if (!delegate)
    return kNoPendingOperation;
  return delegate->SubscribePermissionStatusChange(
      permission, render_frame_host, requesting_origin, callback);
}

void PermissionControllerImpl::UnsubscribePermissionStatusChange(
    int subscription_id) {
  PermissionControllerDelegate* delegate =
      browser_context_->GetPermissionControllerDelegate();
  if (!delegate)
    return;
  delegate->UnsubscribePermissionStatusChange(subscription_id);
}

}  // namespace content

#endif  // CONTENT_BROWSER_PERMISSIONS_PERMISSION_CONTROLLER_IMPL_CC_
