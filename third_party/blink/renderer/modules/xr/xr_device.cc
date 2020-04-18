// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/modules/xr/xr_device.h"

#include "services/metrics/public/cpp/ukm_builders.h"
#include "third_party/blink/renderer/bindings/core/v8/script_promise_resolver.h"
#include "third_party/blink/renderer/core/dom/dom_exception.h"
#include "third_party/blink/renderer/core/frame/frame.h"
#include "third_party/blink/renderer/core/frame/local_frame.h"
#include "third_party/blink/renderer/modules/event_target_modules.h"
#include "third_party/blink/renderer/modules/xr/xr.h"
#include "third_party/blink/renderer/modules/xr/xr_frame_provider.h"
#include "third_party/blink/renderer/modules/xr/xr_session.h"

namespace blink {

namespace {

const char kActiveExclusiveSession[] =
    "XRDevice already has an active, exclusive session";

const char kExclusiveNotSupported[] =
    "XRDevice does not support the creation of exclusive sessions.";

const char kNoOutputContext[] =
    "Non-exclusive sessions must be created with an outputContext.";

const char kRequestRequiresUserActivation[] =
    "The requested session requires user activation.";

}  // namespace

XRDevice::XRDevice(
    XR* xr,
    device::mojom::blink::VRMagicWindowProviderPtr magic_window_provider,
    device::mojom::blink::VRDisplayHostPtr display,
    device::mojom::blink::VRDisplayClientRequest client_request,
    device::mojom::blink::VRDisplayInfoPtr display_info)
    : xr_(xr),
      magic_window_provider_(std::move(magic_window_provider)),
      display_(std::move(display)),
      display_client_binding_(this, std::move(client_request)) {
  SetXRDisplayInfo(std::move(display_info));
}

ExecutionContext* XRDevice::GetExecutionContext() const {
  return xr_->GetExecutionContext();
}

const AtomicString& XRDevice::InterfaceName() const {
  return EventTargetNames::XRDevice;
}

const char* XRDevice::checkSessionSupport(
    const XRSessionCreationOptions& options) const {
  if (options.exclusive()) {
    // Validation for exclusive sessions.
    if (!supports_exclusive_) {
      return kExclusiveNotSupported;
    }
  } else {
    // Validation for non-exclusive sessions.
    if (!options.hasOutputContext()) {
      return kNoOutputContext;
    }
  }

  return nullptr;
}

ScriptPromise XRDevice::supportsSession(
    ScriptState* script_state,
    const XRSessionCreationOptions& options) const {
  // Check to see if the device is capable of supporting the requested session
  // options. Note that reporting support here does not guarantee that creating
  // a session with those options will succeed, as other external and
  // time-sensitve factors (focus state, existance of another exclusive session,
  // etc.) may prevent the creation of a session as well.
  const char* reject_reason = checkSessionSupport(options);
  if (reject_reason) {
    return ScriptPromise::RejectWithDOMException(
        script_state, DOMException::Create(kNotSupportedError, reject_reason));
  }

  // If the above checks pass, resolve without a value. Future API iterations
  // may specify a value to be returned here.
  ScriptPromiseResolver* resolver = ScriptPromiseResolver::Create(script_state);
  ScriptPromise promise = resolver->Promise();
  resolver->Resolve();
  return promise;
}

int64_t XRDevice::GetSourceId() const {
  return xr_->GetSourceId();
}

ScriptPromise XRDevice::requestSession(
    ScriptState* script_state,
    const XRSessionCreationOptions& options) {
  Document* doc = ToDocumentOrNull(ExecutionContext::From(script_state));

  if (options.exclusive() && !did_log_request_exclusive_session_ && doc) {
    ukm::builders::XR_WebXR(GetSourceId())
        .SetDidRequestPresentation(1)
        .Record(doc->UkmRecorder());
    did_log_request_exclusive_session_ = true;
  }

  // Check first to see if the device is capable of supporting the requested
  // options.
  const char* reject_reason = checkSessionSupport(options);
  if (reject_reason) {
    return ScriptPromise::RejectWithDOMException(
        script_state, DOMException::Create(kNotSupportedError, reject_reason));
  }

  // Check if the current page state prevents the requested session from being
  // created.
  if (options.exclusive()) {
    if (frameProvider()->exclusive_session()) {
      return ScriptPromise::RejectWithDOMException(
          script_state,
          DOMException::Create(kInvalidStateError, kActiveExclusiveSession));
    }

    if (!Frame::HasTransientUserActivation(doc ? doc->GetFrame() : nullptr)) {
      return ScriptPromise::RejectWithDOMException(
          script_state,
          DOMException::Create(kSecurityError, kRequestRequiresUserActivation));
    }
  }

  // All AR sessions require a user gesture.
  // TODO(https://crbug.com/828321): Use session options instead.
  if (RuntimeEnabledFeatures::WebXRHitTestEnabled()) {
    if (!Frame::HasTransientUserActivation(doc ? doc->GetFrame() : nullptr)) {
      return ScriptPromise::RejectWithDOMException(
          script_state,
          DOMException::Create(kSecurityError, kRequestRequiresUserActivation));
    }
  }

  ScriptPromiseResolver* resolver = ScriptPromiseResolver::Create(script_state);
  ScriptPromise promise = resolver->Promise();

  XRPresentationContext* output_context = nullptr;
  if (options.hasOutputContext()) {
    output_context = options.outputContext();
  }

  XRSession* session = new XRSession(this, options.exclusive(), output_context);
  sessions_.insert(session);

  if (options.exclusive()) {
    frameProvider()->BeginExclusiveSession(session, resolver);
  } else {
    resolver->Resolve(session);
  }

  return promise;
}

void XRDevice::OnFrameFocusChanged() {
  OnFocusChanged();
}

void XRDevice::OnFocusChanged() {
  // Tell all sessions that focus changed.
  for (const auto& session : sessions_) {
    session->OnFocusChanged();
  }

  if (frame_provider_)
    frame_provider_->OnFocusChanged();
}

bool XRDevice::IsFrameFocused() {
  return xr_->IsFrameFocused();
}

// TODO: Forward these calls on to the sessions once they've been implemented.
void XRDevice::OnChanged(device::mojom::blink::VRDisplayInfoPtr display_info) {
  SetXRDisplayInfo(std::move(display_info));
}
void XRDevice::OnExitPresent() {}
void XRDevice::OnBlur() {
  // The device is reporting to us that it is blurred.  This could happen for a
  // variety of reasons, such as browser UI, a different application using the
  // headset, or another page entering an exclusive session.
  has_device_focus_ = false;
  OnFocusChanged();
}
void XRDevice::OnFocus() {
  has_device_focus_ = true;
  OnFocusChanged();
}
void XRDevice::OnActivate(device::mojom::blink::VRDisplayEventReason,
                          OnActivateCallback on_handled) {}
void XRDevice::OnDeactivate(device::mojom::blink::VRDisplayEventReason) {}

XRFrameProvider* XRDevice::frameProvider() {
  if (!frame_provider_) {
    frame_provider_ = new XRFrameProvider(this);
  }

  return frame_provider_;
}

void XRDevice::Dispose() {
  display_client_binding_.Close();
  if (frame_provider_)
    frame_provider_->Dispose();
}

void XRDevice::SetXRDisplayInfo(
    device::mojom::blink::VRDisplayInfoPtr display_info) {
  display_info_id_++;
  display_info_ = std::move(display_info);
  is_external_ = display_info_->capabilities->hasExternalDisplay;
  supports_exclusive_ = (display_info_->capabilities->canPresent);
}

void XRDevice::Trace(blink::Visitor* visitor) {
  visitor->Trace(xr_);
  visitor->Trace(frame_provider_);
  visitor->Trace(sessions_);
  EventTargetWithInlineData::Trace(visitor);
}

}  // namespace blink
