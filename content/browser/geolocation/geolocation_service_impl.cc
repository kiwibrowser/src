// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/geolocation/geolocation_service_impl.h"

#include "content/browser/permissions/permission_controller_impl.h"
#include "content/public/browser/permission_type.h"
#include "content/public/browser/render_frame_host.h"
#include "content/public/common/content_features.h"
#include "third_party/blink/public/mojom/feature_policy/feature_policy.mojom.h"

namespace content {

GeolocationServiceImplContext::GeolocationServiceImplContext(
    PermissionControllerImpl* permission_controller)
    : permission_controller_(permission_controller),
      request_id_(PermissionController::kNoPendingOperation),
      weak_factory_(this) {}

GeolocationServiceImplContext::~GeolocationServiceImplContext() {
}

void GeolocationServiceImplContext::RequestPermission(
    RenderFrameHost* render_frame_host,
    bool user_gesture,
    const base::Callback<void(blink::mojom::PermissionStatus)>& callback) {
  if (request_id_ != PermissionController::kNoPendingOperation) {
    mojo::ReportBadMessage(
        "GeolocationService client may only create one Geolocation at a "
        "time.");
    return;
  }

  request_id_ = permission_controller_->RequestPermission(
      PermissionType::GEOLOCATION, render_frame_host,
      render_frame_host->GetLastCommittedOrigin().GetURL(), user_gesture,
      // NOTE: The permission request is canceled in the destructor, so it is
      // safe to pass |this| as Unretained.
      base::Bind(&GeolocationServiceImplContext::HandlePermissionStatus,
                 weak_factory_.GetWeakPtr(), std::move(callback)));
}

void GeolocationServiceImplContext::HandlePermissionStatus(
    const base::Callback<void(blink::mojom::PermissionStatus)>& callback,
    blink::mojom::PermissionStatus permission_status) {
  request_id_ = PermissionController::kNoPendingOperation;
  callback.Run(permission_status);
}

GeolocationServiceImpl::GeolocationServiceImpl(
    device::mojom::GeolocationContext* geolocation_context,
    PermissionControllerImpl* permission_controller,
    RenderFrameHost* render_frame_host)
    : geolocation_context_(geolocation_context),
      permission_controller_(permission_controller),
      render_frame_host_(render_frame_host) {
  DCHECK(geolocation_context);
  DCHECK(permission_controller);
  DCHECK(render_frame_host);
}

GeolocationServiceImpl::~GeolocationServiceImpl() {}

void GeolocationServiceImpl::Bind(
    blink::mojom::GeolocationServiceRequest request) {
  binding_set_.AddBinding(
      this, std::move(request),
      std::make_unique<GeolocationServiceImplContext>(permission_controller_));
}

void GeolocationServiceImpl::CreateGeolocation(
    mojo::InterfaceRequest<device::mojom::Geolocation> request,
    bool user_gesture) {
  if (base::FeatureList::IsEnabled(features::kUseFeaturePolicyForPermissions) &&
      !render_frame_host_->IsFeatureEnabled(
          blink::mojom::FeaturePolicyFeature::kGeolocation)) {
    return;
  }

  binding_set_.dispatch_context()->RequestPermission(
      render_frame_host_, user_gesture,
      // There is an assumption here that the GeolocationServiceImplContext will
      // outlive the GeolocationServiceImpl.
      base::Bind(&GeolocationServiceImpl::CreateGeolocationWithPermissionStatus,
                 base::Unretained(this), base::Passed(&request)));
}

void GeolocationServiceImpl::CreateGeolocationWithPermissionStatus(
    device::mojom::GeolocationRequest request,
    blink::mojom::PermissionStatus permission_status) {
  if (permission_status != blink::mojom::PermissionStatus::GRANTED)
    return;

  geolocation_context_->BindGeolocation(std::move(request));
}

}  // namespace content
