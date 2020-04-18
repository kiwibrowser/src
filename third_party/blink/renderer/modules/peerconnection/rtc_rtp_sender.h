// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef THIRD_PARTY_BLINK_RENDERER_MODULES_PEERCONNECTION_RTC_RTP_SENDER_H_
#define THIRD_PARTY_BLINK_RENDERER_MODULES_PEERCONNECTION_RTC_RTP_SENDER_H_

#include "third_party/blink/public/platform/web_rtc_rtp_sender.h"
#include "third_party/blink/renderer/bindings/core/v8/script_promise.h"
#include "third_party/blink/renderer/modules/mediastream/media_stream.h"
#include "third_party/blink/renderer/modules/peerconnection/rtc_rtp_parameters.h"
#include "third_party/blink/renderer/platform/bindings/script_wrappable.h"
#include "third_party/blink/renderer/platform/heap/garbage_collected.h"
#include "third_party/blink/renderer/platform/heap/member.h"
#include "third_party/blink/renderer/platform/heap/visitor.h"
#include "third_party/blink/renderer/platform/wtf/text/wtf_string.h"

namespace blink {

class MediaStreamTrack;
class RTCDTMFSender;
class RTCPeerConnection;
class RTCRtpParameters;

// https://w3c.github.io/webrtc-pc/#rtcrtpsender-interface
class RTCRtpSender final : public ScriptWrappable {
  DEFINE_WRAPPERTYPEINFO();

 public:
  // TODO(hbos): Get rid of sender's reference to RTCPeerConnection?
  // https://github.com/w3c/webrtc-pc/issues/1712
  RTCRtpSender(RTCPeerConnection*,
               std::unique_ptr<WebRTCRtpSender>,
               MediaStreamTrack*,
               MediaStreamVector streams);

  MediaStreamTrack* track();
  ScriptPromise replaceTrack(ScriptState*, MediaStreamTrack*);
  RTCDTMFSender* dtmf();
  void getParameters(RTCRtpParameters&);
  ScriptPromise setParameters(ScriptState*, const RTCRtpParameters&);
  ScriptPromise getStats(ScriptState*);

  WebRTCRtpSender* web_sender();
  // Sets the track. This must be called when the |WebRTCRtpSender| has its
  // track updated, and the |track| must match the |WebRTCRtpSender::Track|.
  void SetTrack(MediaStreamTrack*);
  void ClearLastReturnedParameters();
  MediaStreamVector streams() const;

  void Trace(blink::Visitor*) override;

 private:
  Member<RTCPeerConnection> pc_;
  std::unique_ptr<WebRTCRtpSender> sender_;
  Member<MediaStreamTrack> track_;
  // The spec says that "kind" should be looked up in transceiver, but
  // keeping it in sender at least until transceiver is implemented.
  String kind_;
  Member<RTCDTMFSender> dtmf_;
  MediaStreamVector streams_;
  base::Optional<RTCRtpParameters> last_returned_parameters_;
};

}  // namespace blink

#endif  // THIRD_PARTY_BLINK_RENDERER_MODULES_PEERCONNECTION_RTC_RTP_SENDER_H_
