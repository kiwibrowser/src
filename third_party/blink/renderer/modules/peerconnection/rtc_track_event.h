// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef THIRD_PARTY_BLINK_RENDERER_MODULES_PEERCONNECTION_RTC_TRACK_EVENT_H_
#define THIRD_PARTY_BLINK_RENDERER_MODULES_PEERCONNECTION_RTC_TRACK_EVENT_H_

#include "third_party/blink/renderer/modules/event_modules.h"
#include "third_party/blink/renderer/platform/heap/member.h"
#include "third_party/blink/renderer/platform/heap/visitor.h"
#include "third_party/blink/renderer/platform/wtf/text/atomic_string.h"

namespace blink {

class MediaStream;
class MediaStreamTrack;
class RTCRtpReceiver;
class RTCTrackEventInit;

// https://w3c.github.io/webrtc-pc/#rtctrackevent
class RTCTrackEvent final : public Event {
  DEFINE_WRAPPERTYPEINFO();

 public:
  static RTCTrackEvent* Create(const AtomicString& type,
                               const RTCTrackEventInit& eventInitDict);
  RTCTrackEvent(RTCRtpReceiver*,
                MediaStreamTrack*,
                const HeapVector<Member<MediaStream>>&);

  RTCRtpReceiver* receiver() const;
  MediaStreamTrack* track() const;
  HeapVector<Member<MediaStream>> streams() const;

  void Trace(blink::Visitor*) override;

 private:
  RTCTrackEvent(const AtomicString& type,
                const RTCTrackEventInit& eventInitDict);

  Member<RTCRtpReceiver> receiver_;
  Member<MediaStreamTrack> track_;
  HeapVector<Member<MediaStream>> streams_;
};

}  // namespace blink

#endif  // THIRD_PARTY_BLINK_RENDERER_MODULES_PEERCONNECTION_RTC_TRACK_EVENT_H_
