// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef THIRD_PARTY_BLINK_RENDERER_MODULES_PEERCONNECTION_RTC_SESSION_DESCRIPTION_REQUEST_PROMISE_IMPL_H_
#define THIRD_PARTY_BLINK_RENDERER_MODULES_PEERCONNECTION_RTC_SESSION_DESCRIPTION_REQUEST_PROMISE_IMPL_H_

#include "third_party/blink/renderer/platform/peerconnection/rtc_session_description_request.h"
#include "third_party/blink/renderer/platform/wtf/text/wtf_string.h"

namespace blink {

class RTCPeerConnection;
class ScriptPromiseResolver;
class WebRTCSessionDescription;

class RTCSessionDescriptionRequestPromiseImpl final
    : public RTCSessionDescriptionRequest {
 public:
  static RTCSessionDescriptionRequestPromiseImpl* Create(
      RTCPeerConnection*,
      ScriptPromiseResolver*);
  ~RTCSessionDescriptionRequestPromiseImpl() override;

  // RTCSessionDescriptionRequest
  void RequestSucceeded(const WebRTCSessionDescription&) override;
  void RequestFailed(const webrtc::RTCError& error) override;

  void Trace(blink::Visitor*) override;

 private:
  RTCSessionDescriptionRequestPromiseImpl(RTCPeerConnection*,
                                          ScriptPromiseResolver*);

  void Clear();

  Member<RTCPeerConnection> requester_;
  Member<ScriptPromiseResolver> resolver_;
};

}  // namespace blink

#endif  // THIRD_PARTY_BLINK_RENDERER_MODULES_PEERCONNECTION_RTC_SESSION_DESCRIPTION_REQUEST_PROMISE_IMPL_H_
