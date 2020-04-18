// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/modules/vr/vr_controller.h"

#include "services/metrics/public/cpp/ukm_builders.h"
#include "services/service_manager/public/cpp/interface_provider.h"
#include "third_party/blink/renderer/bindings/core/v8/script_promise_resolver.h"
#include "third_party/blink/renderer/core/dom/document.h"
#include "third_party/blink/renderer/core/dom/dom_exception.h"
#include "third_party/blink/renderer/core/frame/local_frame.h"
#include "third_party/blink/renderer/modules/vr/navigator_vr.h"
#include "third_party/blink/renderer/modules/vr/vr_get_devices_callback.h"
#include "third_party/blink/renderer/platform/wtf/assertions.h"

namespace blink {

VRController::VRController(NavigatorVR* navigator_vr)
    : ContextLifecycleObserver(navigator_vr->GetDocument()),
      navigator_vr_(navigator_vr),
      display_synced_(false),
      binding_(this) {
  navigator_vr->GetDocument()->GetFrame()->GetInterfaceProvider().GetInterface(
      mojo::MakeRequest(&service_));
  service_.set_connection_error_handler(
      WTF::Bind(&VRController::Dispose, WrapWeakPersistent(this)));

  device::mojom::blink::VRServiceClientPtr client;
  binding_.Bind(mojo::MakeRequest(&client));
  service_->SetClient(
      std::move(client),
      WTF::Bind(&VRController::OnDisplaysSynced, WrapPersistent(this)));
}

VRController::~VRController() = default;

void VRController::GetDisplays(ScriptPromiseResolver* resolver) {
  // If we've previously synced the VRDisplays or no longer have a valid service
  // connection just return the current list. In the case of the service being
  // disconnected this will be an empty array.
  if (!service_ || display_synced_) {
    LogGetDisplayResult();
    resolver->Resolve(displays_);
    return;
  }

  // Otherwise we're still waiting for the full list of displays to be populated
  // so queue up the promise for resolution when onDisplaysSynced is called.
  pending_get_devices_callbacks_.push_back(
      std::make_unique<VRGetDevicesCallback>(resolver));
}

void VRController::SetListeningForActivate(bool listening) {
  if (service_)
    service_->SetListeningForActivate(listening);
}

// Each time a new VRDisplay is connected we'll receive a VRDisplayPtr for it
// here. Upon calling SetClient in the constructor we should receive one call
// for each VRDisplay that was already connected at the time.
void VRController::OnDisplayConnected(
    device::mojom::blink::VRMagicWindowProviderPtr magic_window_provider,
    device::mojom::blink::VRDisplayHostPtr display,
    device::mojom::blink::VRDisplayClientRequest request,
    device::mojom::blink::VRDisplayInfoPtr display_info) {
  VRDisplay* vr_display =
      new VRDisplay(navigator_vr_, std::move(magic_window_provider),
                    std::move(display), std::move(request));
  vr_display->Update(display_info);
  vr_display->OnConnected();
  vr_display->FocusChanged();

  has_presentation_capable_display_ = display_info->capabilities->canPresent;
  has_display_ = true;

  displays_.push_back(vr_display);
}

void VRController::FocusChanged() {
  for (const auto& display : displays_)
    display->FocusChanged();
}

// Called when the VRService has called OnDisplayConnected for all active
// VRDisplays.
void VRController::OnDisplaysSynced() {
  display_synced_ = true;
  OnGetDisplays();
}

void VRController::LogGetDisplayResult() {
  Document* doc = navigator_vr_->GetDocument();
  if (has_display_ && doc && doc->IsInMainFrame()) {
    ukm::builders::XR_WebXR ukm_builder(doc->UkmSourceID());
    ukm_builder.SetReturnedDevice(1);
    if (has_presentation_capable_display_) {
      ukm_builder.SetReturnedPresentationCapableDevice(1);
    }
    ukm_builder.Record(doc->UkmRecorder());
  }
}

void VRController::OnGetDisplays() {
  while (!pending_get_devices_callbacks_.IsEmpty()) {
    LogGetDisplayResult();
    std::unique_ptr<VRGetDevicesCallback> callback =
        pending_get_devices_callbacks_.TakeFirst();
    callback->OnSuccess(displays_);
  }
}

void VRController::ContextDestroyed(ExecutionContext*) {
  Dispose();
}

void VRController::Dispose() {
  // If the document context was destroyed, shut down the client connection
  // and never call the mojo service again.
  service_.reset();
  binding_.Close();

  // Shutdown all displays' message pipe
  for (const auto& display : displays_)
    display->Dispose();

  displays_.clear();

  // Ensure that any outstanding getDisplays promises are resolved.
  OnGetDisplays();
}

void VRController::Trace(blink::Visitor* visitor) {
  visitor->Trace(navigator_vr_);
  visitor->Trace(displays_);

  ContextLifecycleObserver::Trace(visitor);
}

}  // namespace blink
