// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/vr/service/vr_display_host.h"

#include "base/bind.h"
#include "base/command_line.h"
#include "chrome/browser/vr/metrics/session_metrics_helper.h"
#include "chrome/browser/vr/mode.h"
#include "chrome/browser/vr/service/browser_xr_device.h"
#include "components/ukm/content/source_url_recorder.h"
#include "content/public/browser/render_frame_host.h"
#include "content/public/browser/render_widget_host_view.h"
#include "content/public/browser/web_contents.h"
#include "content/public/common/content_switches.h"
#include "content/public/common/origin_util.h"
#include "device/vr/vr_display_impl.h"

namespace vr {

namespace {

// TODO(mthiesse): When we unship WebVR 1.1, set this to false.
static constexpr bool kAllowHTTPWebVRWithFlag = true;

bool IsSecureContext(content::RenderFrameHost* host) {
  if (!host)
    return false;
  while (host != nullptr) {
    if (!content::IsOriginSecure(host->GetLastCommittedURL()))
      return false;
    host = host->GetParent();
  }
  return true;
}

}  // namespace

VRDisplayHost::VRDisplayHost(BrowserXrDevice* device,
                             content::RenderFrameHost* render_frame_host,
                             device::mojom::VRServiceClient* service_client,
                             device::mojom::VRDisplayInfoPtr display_info)
    : browser_device_(device),
      in_focused_frame_(
          render_frame_host ? render_frame_host->GetView()->HasFocus() : false),
      render_frame_host_(render_frame_host),
      binding_(this) {
  device::mojom::VRDisplayHostPtr display;
  binding_.Bind(mojo::MakeRequest(&display));
  display_ = std::make_unique<device::VRDisplayImpl>(
      device->GetDevice(), std::move(service_client), std::move(display_info),
      std::move(display), mojo::MakeRequest(&client_), in_focused_frame_);
  browser_device_->OnDisplayHostAdded(this);
}

VRDisplayHost::~VRDisplayHost() {
  browser_device_->OnDisplayHostRemoved(this);
}

void VRDisplayHost::RequestPresent(
    device::mojom::VRSubmitFrameClientPtr client,
    device::mojom::VRPresentationProviderRequest request,
    device::mojom::VRRequestPresentOptionsPtr options,
    bool triggered_by_displayactive,
    RequestPresentCallback callback) {
  bool requires_secure_context =
      !kAllowHTTPWebVRWithFlag ||
      !base::CommandLine::ForCurrentProcess()->HasSwitch(
          switches::kEnableWebVR);
  if (requires_secure_context && !IsSecureContext(render_frame_host_)) {
    std::move(callback).Run(false, nullptr);
    return;
  }

  if (!triggered_by_displayactive) {
    ReportRequestPresent();
  }

  // Check with browser-side device for whether something is already presenting.
  bool another_page_presenting =
      (browser_device_->GetPresentingDisplayHost() != this &&
       browser_device_->GetPresentingDisplayHost() != nullptr);
  if (another_page_presenting || !in_focused_frame_) {
    std::move(callback).Run(false, nullptr);
    return;
  }

  browser_device_->RequestPresent(this, std::move(client), std::move(request),
                                  std::move(options), std::move(callback));
}

void VRDisplayHost::ReportRequestPresent() {
  content::WebContents* web_contents =
      content::WebContents::FromRenderFrameHost(render_frame_host_);
  SessionMetricsHelper* metrics_helper =
      SessionMetricsHelper::FromWebContents(web_contents);
  if (!metrics_helper) {
    // This will only happen if we are not already in VR, set start params
    // accordingly.
    metrics_helper = SessionMetricsHelper::CreateForWebContents(
        web_contents, Mode::kNoVr, false);
  }
  metrics_helper->ReportRequestPresent();
}

void VRDisplayHost::ExitPresent() {
  browser_device_->ExitPresent(this);
}

void VRDisplayHost::SetListeningForActivate(bool listening) {
  listening_for_activate_ = listening;
  browser_device_->UpdateListeningForActivate(this);
}

void VRDisplayHost::SetInFocusedFrame(bool in_focused_frame) {
  in_focused_frame_ = in_focused_frame;
  browser_device_->UpdateListeningForActivate(this);
  display_->SetInFocusedFrame(in_focused_frame);
}

void VRDisplayHost::OnChanged(device::mojom::VRDisplayInfoPtr vr_device_info) {
  client_->OnChanged(std::move(vr_device_info));
}

void VRDisplayHost::OnExitPresent() {
  client_->OnExitPresent();
}

void VRDisplayHost::OnBlur() {
  client_->OnBlur();
}

void VRDisplayHost::OnFocus() {
  client_->OnFocus();
}

void VRDisplayHost::OnActivate(device::mojom::VRDisplayEventReason reason,
                               base::OnceCallback<void(bool)> on_handled) {
  client_->OnActivate(reason, std::move(on_handled));
}

void VRDisplayHost::OnDeactivate(device::mojom::VRDisplayEventReason reason) {
  client_->OnDeactivate(reason);
}

}  // namespace vr
