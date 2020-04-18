// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/modules/xr/xr.h"

#include "services/metrics/public/cpp/ukm_builders.h"
#include "services/service_manager/public/cpp/interface_provider.h"
#include "third_party/blink/public/platform/interface_provider.h"
#include "third_party/blink/renderer/bindings/core/v8/script_promise_resolver.h"
#include "third_party/blink/renderer/core/dom/document.h"
#include "third_party/blink/renderer/core/dom/dom_exception.h"
#include "third_party/blink/renderer/core/frame/local_frame.h"
#include "third_party/blink/renderer/modules/event_modules.h"
#include "third_party/blink/renderer/modules/event_target_modules.h"
#include "third_party/blink/renderer/modules/xr/xr_device.h"
#include "third_party/blink/renderer/platform/feature_policy/feature_policy.h"
namespace blink {

namespace {

const char kNavigatorDetachedError[] =
    "The navigator.xr object is no longer associated with a document.";

const char kFeaturePolicyBlocked[] =
    "Access to the feature \"xr\" is disallowed by feature policy.";

const char kCrossOriginSubframeBlocked[] =
    "Blocked call to navigator.xr.requestDevice inside a cross-origin "
    "iframe because the frame has never been activated by the user.";

const char kNoDevicesMessage[] = "No devices found.";

}  // namespace

XR::XR(LocalFrame& frame, int64_t ukm_source_id)
    : ContextLifecycleObserver(frame.GetDocument()),
      FocusChangedObserver(frame.GetPage()),
      devices_synced_(false),
      ukm_source_id_(ukm_source_id),
      binding_(this) {
  frame.GetInterfaceProvider().GetInterface(mojo::MakeRequest(&service_));
  service_.set_connection_error_handler(
      WTF::Bind(&XR::Dispose, WrapWeakPersistent(this)));

  device::mojom::blink::VRServiceClientPtr client;
  binding_.Bind(mojo::MakeRequest(&client));

  // Setting the client kicks off a request for the details of any connected
  // XRDevices.
  service_->SetClient(std::move(client),
                      WTF::Bind(&XR::OnDevicesSynced, WrapPersistent(this)));
}

void XR::FocusedFrameChanged() {
  // Tell devices that focus changed.
  for (const auto& device : devices_)
    device->OnFrameFocusChanged();
}

bool XR::IsFrameFocused() {
  return FocusChangedObserver::IsFrameFocused(GetFrame());
}

ExecutionContext* XR::GetExecutionContext() const {
  return ContextLifecycleObserver::GetExecutionContext();
}

const AtomicString& XR::InterfaceName() const {
  return EventTargetNames::XR;
}

ScriptPromise XR::requestDevice(ScriptState* script_state) {
  LocalFrame* frame = GetFrame();
  if (!frame) {
    // Reject if the frame is inaccessible.
    return ScriptPromise::RejectWithDOMException(
        script_state,
        DOMException::Create(kInvalidStateError, kNavigatorDetachedError));
  }

  if (!did_log_requestDevice_ && frame->GetDocument()) {
    ukm::builders::XR_WebXR(ukm_source_id_)
        .SetDidRequestAvailableDevices(1)
        .Record(frame->GetDocument()->UkmRecorder());
    did_log_requestDevice_ = true;
  }

  if (IsSupportedInFeaturePolicy(mojom::FeaturePolicyFeature::kWebVr)) {
    if (!frame->IsFeatureEnabled(mojom::FeaturePolicyFeature::kWebVr)) {
      // Only allow the call to be made if the appropraite feature policy is in
      // place.
      return ScriptPromise::RejectWithDOMException(
          script_state,
          DOMException::Create(kSecurityError, kFeaturePolicyBlocked));
    }
  } else if (!frame->HasBeenActivated() && frame->IsCrossOriginSubframe()) {
    // Block calls from cross-origin iframes that have never had a user gesture.
    return ScriptPromise::RejectWithDOMException(
        script_state,
        DOMException::Create(kSecurityError, kCrossOriginSubframeBlocked));
  }

  // If we're still waiting for a previous call to resolve return that promise
  // again.
  if (pending_devices_resolver_) {
    return pending_devices_resolver_->Promise();
  }

  ScriptPromiseResolver* resolver = ScriptPromiseResolver::Create(script_state);
  ScriptPromise promise = resolver->Promise();

  // If we've previously synced the XRDevices or no longer have a valid service
  // connection just use the current list. In the case of the service being
  // disconnected this will be an empty array.
  if (!service_ || devices_synced_) {
    // TODO (offenwanger): When we have a prioritized order of devices, or some
    // other method of getting the prefered device, insert that here. For now,
    // just get the first device out of the list, if there is one.
    if (devices_.size() == 0) {
      resolver->Reject(DOMException::Create(kNotFoundError, kNoDevicesMessage));
    } else {
      resolver->Resolve(devices_[0]);
    }

    return promise;
  }

  // Otherwise wait for the full list of devices to be populated and resolve
  // when onDevicesSynced is called.
  pending_devices_resolver_ = resolver;

  return promise;
}

// Each time a new XRDevice is connected we'll recieve a XRDisplayPtr for it
// here. Upon calling SetClient in the constructor we should receive one call
// for each XRDevice that was already connected at the time.
void XR::OnDisplayConnected(
    device::mojom::blink::VRMagicWindowProviderPtr magic_window_provider,
    device::mojom::blink::VRDisplayHostPtr display,
    device::mojom::blink::VRDisplayClientRequest client_request,
    device::mojom::blink::VRDisplayInfoPtr display_info) {
  XRDevice* xr_device =
      new XRDevice(this, std::move(magic_window_provider), std::move(display),
                   std::move(client_request), std::move(display_info));

  devices_.push_back(xr_device);

  DispatchEvent(blink::Event::Create(EventTypeNames::devicechange));
}

// Called when the XRService has called OnDevicesConnected for all active
// XRDevices.
void XR::OnDevicesSynced() {
  devices_synced_ = true;
  ResolveRequestDevice();
}

// Called when details for every connected XRDevice has been received.
void XR::ResolveRequestDevice() {
  if (pending_devices_resolver_) {
    if (devices_.size() == 0) {
      pending_devices_resolver_->Reject(
          DOMException::Create(kNotFoundError, kNoDevicesMessage));
    } else {
      if (!did_log_returned_device_ || !did_log_supports_exclusive_) {
        Document* doc = GetFrame() ? GetFrame()->GetDocument() : nullptr;
        if (doc) {
          ukm::builders::XR_WebXR ukm_builder(ukm_source_id_);
          ukm_builder.SetReturnedDevice(1);
          did_log_returned_device_ = true;

          // We only expose a single device to WebXR, so report that device's
          // capabilities.
          if (devices_[0]->SupportsExclusive()) {
            ukm_builder.SetReturnedPresentationCapableDevice(1);
            did_log_supports_exclusive_ = true;
          }

          ukm_builder.Record(doc->UkmRecorder());
        }
      }
      pending_devices_resolver_->Resolve(devices_[0]);
    }

    pending_devices_resolver_ = nullptr;
  }
}

void XR::ContextDestroyed(ExecutionContext*) {
  Dispose();
}

void XR::Dispose() {
  // If the document context was destroyed, shut down the client connection
  // and never call the mojo service again.
  service_.reset();
  binding_.Close();

  // Shutdown all devices' message pipe
  for (const auto& device : devices_)
    device->Dispose();

  devices_.clear();

  // Ensure that any outstanding requestDevice promises are resolved. They will
  // receive a null result.
  ResolveRequestDevice();
}

void XR::Trace(blink::Visitor* visitor) {
  visitor->Trace(devices_);
  visitor->Trace(pending_devices_resolver_);
  ContextLifecycleObserver::Trace(visitor);
  EventTargetWithInlineData::Trace(visitor);
}

}  // namespace blink
