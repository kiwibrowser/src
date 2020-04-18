// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/modules/peerconnection/rtc_session_description_request_promise_impl.h"

#include "third_party/blink/public/platform/web_rtc_session_description.h"
#include "third_party/blink/renderer/bindings/core/v8/script_promise_resolver.h"
#include "third_party/blink/renderer/core/dom/dom_exception.h"
#include "third_party/blink/renderer/core/dom/exception_code.h"
#include "third_party/blink/renderer/modules/peerconnection/rtc_error_util.h"
#include "third_party/blink/renderer/modules/peerconnection/rtc_peer_connection.h"
#include "third_party/blink/renderer/modules/peerconnection/rtc_session_description.h"

namespace blink {

RTCSessionDescriptionRequestPromiseImpl*
RTCSessionDescriptionRequestPromiseImpl::Create(
    RTCPeerConnection* requester,
    ScriptPromiseResolver* resolver) {
  return new RTCSessionDescriptionRequestPromiseImpl(requester, resolver);
}

RTCSessionDescriptionRequestPromiseImpl::
    RTCSessionDescriptionRequestPromiseImpl(RTCPeerConnection* requester,
                                            ScriptPromiseResolver* resolver)
    : requester_(requester), resolver_(resolver) {
  DCHECK(requester_);
  DCHECK(resolver_);
}

RTCSessionDescriptionRequestPromiseImpl::
    ~RTCSessionDescriptionRequestPromiseImpl() = default;

void RTCSessionDescriptionRequestPromiseImpl::RequestSucceeded(
    const WebRTCSessionDescription& web_session_description) {
  if (requester_ && requester_->ShouldFireDefaultCallbacks()) {
    auto* description = RTCSessionDescription::Create(web_session_description);
    requester_->NoteSdpCreated(*description);
    resolver_->Resolve(description);
  } else {
    // This is needed to have the resolver release its internal resources
    // while leaving the associated promise pending as specified.
    resolver_->Detach();
  }

  Clear();
}

void RTCSessionDescriptionRequestPromiseImpl::RequestFailed(
    const webrtc::RTCError& error) {
  if (requester_ && requester_->ShouldFireDefaultCallbacks()) {
    resolver_->Reject(CreateDOMExceptionFromRTCError(error));
  } else {
    // This is needed to have the resolver release its internal resources
    // while leaving the associated promise pending as specified.
    resolver_->Detach();
  }

  Clear();
}

void RTCSessionDescriptionRequestPromiseImpl::Clear() {
  requester_.Clear();
}

void RTCSessionDescriptionRequestPromiseImpl::Trace(blink::Visitor* visitor) {
  visitor->Trace(resolver_);
  visitor->Trace(requester_);
  RTCSessionDescriptionRequest::Trace(visitor);
}

}  // namespace blink
