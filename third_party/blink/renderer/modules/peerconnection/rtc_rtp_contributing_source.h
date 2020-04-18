// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef THIRD_PARTY_BLINK_RENDERER_MODULES_PEERCONNECTION_RTC_RTP_CONTRIBUTING_SOURCE_H_
#define THIRD_PARTY_BLINK_RENDERER_MODULES_PEERCONNECTION_RTC_RTP_CONTRIBUTING_SOURCE_H_

#include "third_party/blink/renderer/platform/bindings/script_wrappable.h"
#include "third_party/blink/renderer/platform/heap/garbage_collected.h"
#include "third_party/blink/renderer/platform/heap/member.h"
#include "third_party/blink/renderer/platform/heap/visitor.h"

namespace blink {

class RTCRtpReceiver;
class WebRTCRtpContributingSource;

// https://w3c.github.io/webrtc-pc/#dom-rtcrtpcontributingsource
class RTCRtpContributingSource final : public ScriptWrappable {
  DEFINE_WRAPPERTYPEINFO();

 public:
  RTCRtpContributingSource(RTCRtpReceiver*, const WebRTCRtpContributingSource&);

  double timestamp() const;
  uint32_t source() const;

  void Trace(blink::Visitor*) override;

 private:
  Member<RTCRtpReceiver> receiver_;
  double timestamp_ms_;
  const uint32_t source_;
};

}  // namespace blink

#endif  // THIRD_PARTY_BLINK_RENDERER_MODULES_PEERCONNECTION_RTC_RTP_CONTRIBUTING_SOURCE_H_
