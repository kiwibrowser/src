// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/renderer/media_capture_from_element/html_audio_element_capturer_source.h"

#include "base/threading/thread_task_runner_handle.h"
#include "media/base/audio_parameters.h"
#include "media/base/audio_renderer_sink.h"
#include "media/blink/webaudiosourceprovider_impl.h"
#include "media/blink/webmediaplayer_impl.h"
#include "third_party/blink/public/platform/web_media_player.h"

namespace content {

//static
HtmlAudioElementCapturerSource*
HtmlAudioElementCapturerSource::CreateFromWebMediaPlayerImpl(
    blink::WebMediaPlayer* player) {
  DCHECK(player);
  return new HtmlAudioElementCapturerSource(
      static_cast<media::WebAudioSourceProviderImpl*>(
          player->GetAudioSourceProvider()));
}

HtmlAudioElementCapturerSource::HtmlAudioElementCapturerSource(
    media::WebAudioSourceProviderImpl* audio_source)
    : MediaStreamAudioSource(true /* is_local_source */),
      audio_source_(audio_source),
      is_started_(false),
      last_sample_rate_(0),
      last_num_channels_(0),
      last_bus_frames_(0),
      weak_factory_(this) {
  DCHECK(audio_source_);
}

HtmlAudioElementCapturerSource::~HtmlAudioElementCapturerSource() {
  DCHECK(thread_checker_.CalledOnValidThread());
  EnsureSourceIsStopped();
}

bool HtmlAudioElementCapturerSource::EnsureSourceIsStarted() {
  DCHECK(thread_checker_.CalledOnValidThread());
  if (audio_source_ && !is_started_) {
    base::ThreadTaskRunnerHandle::Get()->PostTask(
        FROM_HERE,
        base::BindOnce(&HtmlAudioElementCapturerSource::SetAudioCallback,
                       weak_factory_.GetWeakPtr()));
    is_started_ = true;
  }
  return is_started_;
}

void HtmlAudioElementCapturerSource::SetAudioCallback() {
  DCHECK(thread_checker_.CalledOnValidThread());
  if (audio_source_ && is_started_) {
    // base:Unretained() is safe here since EnsureSourceIsStopped() guarantees
    // no more calls to OnAudioBus().
    audio_source_->SetCopyAudioCallback(base::Bind(
        &HtmlAudioElementCapturerSource::OnAudioBus, base::Unretained(this)));
  }
}

void HtmlAudioElementCapturerSource::EnsureSourceIsStopped() {
  DCHECK(thread_checker_.CalledOnValidThread());
  if (!is_started_)
    return;

  if (audio_source_) {
    audio_source_->ClearCopyAudioCallback();
    audio_source_ = nullptr;
  }
  is_started_ = false;
}

void HtmlAudioElementCapturerSource::OnAudioBus(
    std::unique_ptr<media::AudioBus> audio_bus,
    uint32_t frames_delayed,
    int sample_rate) {
  const base::TimeTicks capture_time =
      base::TimeTicks::Now() -
      base::TimeDelta::FromMicroseconds(base::Time::kMicrosecondsPerSecond *
                                        frames_delayed / sample_rate);

  if (sample_rate != last_sample_rate_ ||
      audio_bus->channels() != last_num_channels_ ||
      audio_bus->frames() != last_bus_frames_) {
    MediaStreamAudioSource::SetFormat(
        media::AudioParameters(media::AudioParameters::AUDIO_PCM_LOW_LATENCY,
                               media::GuessChannelLayout(audio_bus->channels()),
                               sample_rate, audio_bus->frames()));
    last_sample_rate_ = sample_rate;
    last_num_channels_ = audio_bus->channels();
    last_bus_frames_ = audio_bus->frames();
  }

  MediaStreamAudioSource::DeliverDataToTracks(*audio_bus, capture_time);
}

}  // namespace content
