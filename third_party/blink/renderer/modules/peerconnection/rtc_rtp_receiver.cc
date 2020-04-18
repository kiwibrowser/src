// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/modules/peerconnection/rtc_rtp_receiver.h"

#include "third_party/blink/public/platform/web_media_stream.h"
#include "third_party/blink/public/platform/web_media_stream_track.h"
#include "third_party/blink/public/platform/web_rtc_rtp_contributing_source.h"
#include "third_party/blink/renderer/modules/peerconnection/web_rtc_stats_report_callback_resolver.h"
#include "third_party/blink/renderer/platform/bindings/microtask.h"
#include "third_party/blink/renderer/platform/wtf/text/wtf_string.h"

namespace blink {

RTCRtpReceiver::RTCRtpReceiver(std::unique_ptr<WebRTCRtpReceiver> receiver,
                               MediaStreamTrack* track,
                               MediaStreamVector streams)
    : receiver_(std::move(receiver)),
      track_(track),
      streams_(std::move(streams)) {
  DCHECK(receiver_);
  DCHECK(track_);
  // Some bots require #if around the DCHECK to avoid compile error about
  // |StateMatchesWebReceiver| (which is behind #if) not being defined.
#if DCHECK_IS_ON()
  DCHECK(StateMatchesWebReceiver());
#endif  // DCHECK_IS_ON()
}

MediaStreamTrack* RTCRtpReceiver::track() const {
  return track_;
}

const HeapVector<Member<RTCRtpContributingSource>>&
RTCRtpReceiver::getContributingSources() {
  UpdateSourcesIfNeeded();
  return contributing_sources_;
}

ScriptPromise RTCRtpReceiver::getStats(ScriptState* script_state) {
  ScriptPromiseResolver* resolver = ScriptPromiseResolver::Create(script_state);
  ScriptPromise promise = resolver->Promise();
  receiver_->GetStats(WebRTCStatsReportCallbackResolver::Create(resolver));
  return promise;
}

const WebRTCRtpReceiver& RTCRtpReceiver::web_receiver() const {
  return *receiver_;
}

MediaStreamVector RTCRtpReceiver::streams() const {
  return streams_;
}

#if DCHECK_IS_ON()

bool RTCRtpReceiver::StateMatchesWebReceiver() const {
  if (track_->Component() !=
      static_cast<MediaStreamComponent*>(receiver_->Track())) {
    return false;
  }
  WebVector<WebMediaStream> web_streams = receiver_->Streams();
  if (streams_.size() != web_streams.size())
    return false;
  for (size_t i = 0; i < streams_.size(); ++i) {
    if (streams_[i]->Descriptor() !=
        static_cast<MediaStreamDescriptor*>(web_streams[i])) {
      return false;
    }
  }
  return true;
}

#endif  // DCHECK_IS_ON()

void RTCRtpReceiver::UpdateSourcesIfNeeded() {
  if (!contributing_sources_needs_updating_)
    return;
  contributing_sources_.clear();
  for (const std::unique_ptr<WebRTCRtpContributingSource>&
           web_contributing_source : receiver_->GetSources()) {
    if (web_contributing_source->SourceType() ==
        WebRTCRtpContributingSourceType::SSRC) {
      // TODO(hbos): When |getSynchronizationSources| is added to get SSRC
      // sources don't ignore SSRCs here.
      continue;
    }
    DCHECK_EQ(web_contributing_source->SourceType(),
              WebRTCRtpContributingSourceType::CSRC);
    RTCRtpContributingSource* contributing_source =
        new RTCRtpContributingSource(this, *web_contributing_source);
    contributing_sources_.push_back(contributing_source);
  }
  // Clear the flag and schedule a microtask to reset it to true. This makes
  // the cache valid until the next microtask checkpoint. As such, sources
  // represent a snapshot and can be compared reliably in .js code, no risk of
  // being updated due to an RTP packet arriving. E.g.
  // "source.timestamp == source.timestamp" will always be true.
  contributing_sources_needs_updating_ = false;
  Microtask::EnqueueMicrotask(
      WTF::Bind(&RTCRtpReceiver::SetContributingSourcesNeedsUpdating,
                WrapWeakPersistent(this)));
}

void RTCRtpReceiver::SetContributingSourcesNeedsUpdating() {
  contributing_sources_needs_updating_ = true;
}

void RTCRtpReceiver::Trace(blink::Visitor* visitor) {
  visitor->Trace(track_);
  visitor->Trace(streams_);
  visitor->Trace(contributing_sources_);
  ScriptWrappable::Trace(visitor);
}

}  // namespace blink
