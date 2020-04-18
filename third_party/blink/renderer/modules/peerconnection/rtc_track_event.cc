// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/modules/peerconnection/rtc_track_event.h"

#include "third_party/blink/renderer/modules/mediastream/media_stream.h"
#include "third_party/blink/renderer/modules/mediastream/media_stream_track.h"
#include "third_party/blink/renderer/modules/peerconnection/rtc_rtp_receiver.h"
#include "third_party/blink/renderer/modules/peerconnection/rtc_track_event_init.h"

namespace blink {

RTCTrackEvent* RTCTrackEvent::Create(const AtomicString& type,
                                     const RTCTrackEventInit& eventInitDict) {
  return new RTCTrackEvent(type, eventInitDict);
}

RTCTrackEvent::RTCTrackEvent(const AtomicString& type,
                             const RTCTrackEventInit& eventInitDict)
    : Event(type, eventInitDict),
      receiver_(eventInitDict.receiver()),
      track_(eventInitDict.track()),
      streams_(eventInitDict.streams()) {
  DCHECK(receiver_);
  DCHECK(track_);
}

RTCTrackEvent::RTCTrackEvent(RTCRtpReceiver* receiver,
                             MediaStreamTrack* track,
                             const HeapVector<Member<MediaStream>>& streams)
    : Event(EventTypeNames::track, Bubbles::kNo, Cancelable::kNo),
      receiver_(receiver),
      track_(track),
      streams_(streams) {
  DCHECK(receiver_);
  DCHECK(track_);
}

RTCRtpReceiver* RTCTrackEvent::receiver() const {
  return receiver_;
}

MediaStreamTrack* RTCTrackEvent::track() const {
  return track_;
}

HeapVector<Member<MediaStream>> RTCTrackEvent::streams() const {
  return streams_;
}

void RTCTrackEvent::Trace(blink::Visitor* visitor) {
  visitor->Trace(receiver_);
  visitor->Trace(track_);
  visitor->Trace(streams_);
  Event::Trace(visitor);
}

}  // namespace blink
