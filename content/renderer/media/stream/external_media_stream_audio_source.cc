// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/renderer/media/stream/external_media_stream_audio_source.h"

namespace content {

ExternalMediaStreamAudioSource::ExternalMediaStreamAudioSource(
    scoped_refptr<media::AudioCapturerSource> source,
    int sample_rate,
    media::ChannelLayout channel_layout,
    int frames_per_buffer,
    bool is_remote)
    : MediaStreamAudioSource(!is_remote),
      source_(std::move(source)),
      was_started_(false) {
  DVLOG(1)
      << "ExternalMediaStreamAudioSource::ExternalMediaStreamAudioSource()";
  DCHECK(source_.get());
  MediaStreamAudioSource::SetFormat(media::AudioParameters(
      media::AudioParameters::AUDIO_PCM_LOW_LATENCY, channel_layout,
      sample_rate,
      frames_per_buffer));
}

ExternalMediaStreamAudioSource::~ExternalMediaStreamAudioSource() {
  DVLOG(1)
      << "ExternalMediaStreamAudioSource::~ExternalMediaStreamAudioSource()";
  EnsureSourceIsStopped();
}

bool ExternalMediaStreamAudioSource::EnsureSourceIsStarted() {
  DCHECK(thread_checker_.CalledOnValidThread());
  if (was_started_)
    return true;
  VLOG(1) << "Starting externally-provided "
          << (is_local_source() ? "local" : "remote")
          << " source with audio parameters={"
          << GetAudioParameters().AsHumanReadableString() << "}.";
  source_->Initialize(GetAudioParameters(), this);
  source_->Start();
  was_started_ = true;
  return true;
}

void ExternalMediaStreamAudioSource::EnsureSourceIsStopped() {
  DCHECK(thread_checker_.CalledOnValidThread());
  if (!source_)
    return;
  if (was_started_)
    source_->Stop();
  source_ = nullptr;
  VLOG(1) << "Stopped externally-provided "
          << (is_local_source() ? "local" : "remote")
          << " source with audio parameters={"
          << GetAudioParameters().AsHumanReadableString() << "}.";
}

void ExternalMediaStreamAudioSource::Capture(const media::AudioBus* audio_bus,
                                             int audio_delay_milliseconds,
                                             double volume,
                                             bool key_pressed) {
  DCHECK(audio_bus);
  // TODO(miu): Plumbing is needed to determine the actual capture timestamp
  // of the audio, instead of just snapshotting TimeTicks::Now(), for proper
  // audio/video sync. http://crbug.com/335335
  MediaStreamAudioSource::DeliverDataToTracks(
      *audio_bus,
      base::TimeTicks::Now() -
          base::TimeDelta::FromMilliseconds(audio_delay_milliseconds));
}

void ExternalMediaStreamAudioSource::OnCaptureError(const std::string& why) {
  StopSourceOnError(why);
}

void ExternalMediaStreamAudioSource::OnCaptureMuted(bool is_muted) {
  SetMutedState(is_muted);
}

}  // namespace content
