// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/renderer/media/stream/media_stream_audio_track.h"

#include <utility>
#include <vector>

#include "base/callback_helpers.h"
#include "base/logging.h"
#include "content/public/renderer/media_stream_audio_sink.h"
#include "media/base/audio_bus.h"
#include "third_party/blink/public/platform/web_media_stream_source.h"

namespace content {

MediaStreamAudioTrack::MediaStreamAudioTrack(bool is_local_track)
    : MediaStreamTrack(is_local_track), is_enabled_(1), weak_factory_(this) {
  DVLOG(1) << "MediaStreamAudioTrack@" << this << "::MediaStreamAudioTrack("
           << (is_local_track ? "local" : "remote") << " track)";
}

MediaStreamAudioTrack::~MediaStreamAudioTrack() {
  DCHECK(thread_checker_.CalledOnValidThread());
  DVLOG(1) << "MediaStreamAudioTrack@" << this << " is being destroyed.";
  Stop();
}

// static
MediaStreamAudioTrack* MediaStreamAudioTrack::From(
    const blink::WebMediaStreamTrack& track) {
  if (track.IsNull() ||
      track.Source().GetType() != blink::WebMediaStreamSource::kTypeAudio) {
    return nullptr;
  }
  return static_cast<MediaStreamAudioTrack*>(track.GetTrackData());
}

void MediaStreamAudioTrack::AddSink(MediaStreamAudioSink* sink) {
  DCHECK(thread_checker_.CalledOnValidThread());

  DVLOG(1) << "Adding MediaStreamAudioSink@" << sink
           << " to MediaStreamAudioTrack@" << this << '.';

  // If the track has already stopped, just notify the sink of this fact without
  // adding it.
  if (stop_callback_.is_null()) {
    sink->OnReadyStateChanged(blink::WebMediaStreamSource::kReadyStateEnded);
    return;
  }

  deliverer_.AddConsumer(sink);
  sink->OnEnabledChanged(!!base::subtle::NoBarrier_Load(&is_enabled_));
}

void MediaStreamAudioTrack::RemoveSink(MediaStreamAudioSink* sink) {
  DCHECK(thread_checker_.CalledOnValidThread());
  deliverer_.RemoveConsumer(sink);
  DVLOG(1) << "Removed MediaStreamAudioSink@" << sink
           << " from MediaStreamAudioTrack@" << this << '.';
}

media::AudioParameters MediaStreamAudioTrack::GetOutputFormat() const {
  return deliverer_.GetAudioParameters();
}

void MediaStreamAudioTrack::SetEnabled(bool enabled) {
  DCHECK(thread_checker_.CalledOnValidThread());
  DVLOG(1) << "MediaStreamAudioTrack@" << this << "::SetEnabled("
           << (enabled ? 'Y' : 'N') << ')';

  const bool previously_enabled =
      !!base::subtle::NoBarrier_AtomicExchange(&is_enabled_, enabled ? 1 : 0);
  if (enabled == previously_enabled)
    return;

  std::vector<MediaStreamAudioSink*> sinks_to_notify;
  deliverer_.GetConsumerList(&sinks_to_notify);
  for (MediaStreamAudioSink* sink : sinks_to_notify)
    sink->OnEnabledChanged(enabled);
}

void MediaStreamAudioTrack::SetContentHint(
    blink::WebMediaStreamTrack::ContentHintType content_hint) {
  DCHECK(thread_checker_.CalledOnValidThread());

  std::vector<MediaStreamAudioSink*> sinks_to_notify;
  deliverer_.GetConsumerList(&sinks_to_notify);
  for (MediaStreamAudioSink* sink : sinks_to_notify)
    sink->OnContentHintChanged(content_hint);
}

void* MediaStreamAudioTrack::GetClassIdentifier() const {
  return nullptr;
}

void MediaStreamAudioTrack::Start(const base::Closure& stop_callback) {
  DCHECK(thread_checker_.CalledOnValidThread());
  DCHECK(!stop_callback.is_null());
  DCHECK(stop_callback_.is_null());
  DVLOG(1) << "Starting MediaStreamAudioTrack@" << this << '.';
  stop_callback_ = stop_callback;
}

void MediaStreamAudioTrack::StopAndNotify(base::OnceClosure callback) {
  DCHECK(thread_checker_.CalledOnValidThread());
  DVLOG(1) << "Stopping MediaStreamAudioTrack@" << this << '.';

  if (!stop_callback_.is_null())
    base::ResetAndReturn(&stop_callback_).Run();

  std::vector<MediaStreamAudioSink*> sinks_to_end;
  deliverer_.GetConsumerList(&sinks_to_end);
  for (MediaStreamAudioSink* sink : sinks_to_end) {
    deliverer_.RemoveConsumer(sink);
    sink->OnReadyStateChanged(blink::WebMediaStreamSource::kReadyStateEnded);
  }

  if (callback)
    std::move(callback).Run();
  weak_factory_.InvalidateWeakPtrs();
}

void MediaStreamAudioTrack::OnSetFormat(const media::AudioParameters& params) {
  deliverer_.OnSetFormat(params);
}

void MediaStreamAudioTrack::OnData(const media::AudioBus& audio_bus,
                                   base::TimeTicks reference_time) {
  // Note: Using NoBarrier_Load because the timing of when the audio thread sees
  // a changed |is_enabled_| value can be relaxed.
  const bool deliver_data = !!base::subtle::NoBarrier_Load(&is_enabled_);

  if (deliver_data) {
    deliverer_.OnData(audio_bus, reference_time);
  } else {
    // The W3C spec requires silent audio to flow while a track is disabled.
    if (!silent_bus_ || silent_bus_->channels() != audio_bus.channels() ||
        silent_bus_->frames() != audio_bus.frames()) {
      silent_bus_ = media::AudioBus::Create(audio_bus.channels(),
                                            audio_bus.frames());
      silent_bus_->Zero();
    }
    deliverer_.OnData(*silent_bus_, reference_time);
  }
}

}  // namespace content
