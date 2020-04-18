// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef THIRD_PARTY_BLINK_RENDERER_MODULES_PEERCONNECTION_RTC_RTP_RECEIVER_H_
#define THIRD_PARTY_BLINK_RENDERER_MODULES_PEERCONNECTION_RTC_RTP_RECEIVER_H_

#include <map>

#include "third_party/blink/public/platform/web_rtc_rtp_receiver.h"
#include "third_party/blink/renderer/modules/mediastream/media_stream.h"
#include "third_party/blink/renderer/modules/mediastream/media_stream_track.h"
#include "third_party/blink/renderer/modules/peerconnection/rtc_rtp_contributing_source.h"
#include "third_party/blink/renderer/platform/bindings/script_wrappable.h"
#include "third_party/blink/renderer/platform/heap/garbage_collected.h"
#include "third_party/blink/renderer/platform/heap/member.h"
#include "third_party/blink/renderer/platform/heap/visitor.h"

namespace blink {

// https://w3c.github.io/webrtc-pc/#rtcrtpreceiver-interface
class RTCRtpReceiver final : public ScriptWrappable {
  DEFINE_WRAPPERTYPEINFO();

 public:
  // Takes ownership of the receiver.
  RTCRtpReceiver(std::unique_ptr<WebRTCRtpReceiver>,
                 MediaStreamTrack*,
                 MediaStreamVector);

  MediaStreamTrack* track() const;
  const HeapVector<Member<RTCRtpContributingSource>>& getContributingSources();
  ScriptPromise getStats(ScriptState*);

  const WebRTCRtpReceiver& web_receiver() const;
  MediaStreamVector streams() const;
  void UpdateSourcesIfNeeded();

  void Trace(blink::Visitor*) override;

 private:
#if DCHECK_IS_ON()
  bool StateMatchesWebReceiver() const;
#endif  // DCHECK_IS_ON()
  void SetContributingSourcesNeedsUpdating();

  std::unique_ptr<WebRTCRtpReceiver> receiver_;
  Member<MediaStreamTrack> track_;
  MediaStreamVector streams_;

  // The current contributing sources (|getContributingSources|).
  HeapVector<Member<RTCRtpContributingSource>> contributing_sources_;
  bool contributing_sources_needs_updating_ = true;
};

}  // namespace blink

#endif  // THIRD_PARTY_BLINK_RENDERER_MODULES_PEERCONNECTION_RTC_RTP_RECEIVER_H_
