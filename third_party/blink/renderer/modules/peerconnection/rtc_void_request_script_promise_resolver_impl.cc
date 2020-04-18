// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/modules/peerconnection/rtc_void_request_script_promise_resolver_impl.h"
#include "third_party/blink/renderer/bindings/core/v8/script_promise_resolver.h"
#include "third_party/blink/renderer/core/dom/dom_exception.h"
#include "third_party/blink/renderer/core/dom/exception_code.h"
#include "third_party/blink/renderer/modules/peerconnection/rtc_error_util.h"
#include "third_party/blink/renderer/modules/peerconnection/rtc_peer_connection.h"

namespace blink {

RTCVoidRequestScriptPromiseResolverImpl*
RTCVoidRequestScriptPromiseResolverImpl::Create(
    ScriptPromiseResolver* resolver) {
  return new RTCVoidRequestScriptPromiseResolverImpl(resolver);
}

RTCVoidRequestScriptPromiseResolverImpl::
    RTCVoidRequestScriptPromiseResolverImpl(ScriptPromiseResolver* resolver)
    : resolver_(resolver) {
  DCHECK(resolver_);
}

RTCVoidRequestScriptPromiseResolverImpl::
    ~RTCVoidRequestScriptPromiseResolverImpl() = default;

void RTCVoidRequestScriptPromiseResolverImpl::RequestSucceeded() {
  resolver_->Resolve();
}

void RTCVoidRequestScriptPromiseResolverImpl::RequestFailed(
    const webrtc::RTCError& error) {
  resolver_->Reject(CreateDOMExceptionFromRTCError(error));
}

void RTCVoidRequestScriptPromiseResolverImpl::Trace(blink::Visitor* visitor) {
  visitor->Trace(resolver_);
  RTCVoidRequest::Trace(visitor);
}

}  // namespace blink
