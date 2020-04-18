// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/renderer/media/stream/media_stream_audio_source.h"

#include "base/bind.h"
#include "base/single_thread_task_runner.h"
#include "base/threading/thread_task_runner_handle.h"
#include "content/renderer/media/stream/media_stream_audio_track.h"
#include "third_party/blink/public/platform/web_media_stream_source.h"
#include "third_party/blink/public/platform/web_string.h"

namespace content {

MediaStreamAudioSource::MediaStreamAudioSource(bool is_local_source,
                                               bool hotword_enabled,
                                               bool disable_local_echo)
    : is_local_source_(is_local_source),
      hotword_enabled_(hotword_enabled),
      disable_local_echo_(disable_local_echo),
      is_stopped_(false),
      task_runner_(base::ThreadTaskRunnerHandle::Get()),
      weak_factory_(this) {
  DVLOG(1) << "MediaStreamAudioSource@" << this << "::MediaStreamAudioSource("
           << (is_local_source_ ? "local" : "remote") << " source)";
}

MediaStreamAudioSource::MediaStreamAudioSource(bool is_local_source)
    : MediaStreamAudioSource(is_local_source,
                             false /* hotword_enabled */,
                             false /* disable_local_echo */) {}

MediaStreamAudioSource::~MediaStreamAudioSource() {
  DCHECK(task_runner_->RunsTasksInCurrentSequence());
  DVLOG(1) << "MediaStreamAudioSource@" << this << " is being destroyed.";
}

// static
MediaStreamAudioSource* MediaStreamAudioSource::From(
    const blink::WebMediaStreamSource& source) {
  if (source.IsNull() ||
      source.GetType() != blink::WebMediaStreamSource::kTypeAudio) {
    return nullptr;
  }
  return static_cast<MediaStreamAudioSource*>(source.GetExtraData());
}

bool MediaStreamAudioSource::ConnectToTrack(
    const blink::WebMediaStreamTrack& blink_track) {
  DCHECK(task_runner_->RunsTasksInCurrentSequence());
  DCHECK(!blink_track.IsNull());

  // Sanity-check that there is not already a MediaStreamAudioTrack instance
  // associated with |blink_track|.
  if (MediaStreamAudioTrack::From(blink_track)) {
    LOG(DFATAL)
        << "Attempting to connect another source to a WebMediaStreamTrack.";
    return false;
  }

  // Unless the source has already been permanently stopped, ensure it is
  // started. If the source cannot start, the new MediaStreamAudioTrack will be
  // initialized to the stopped/ended state.
  if (!is_stopped_) {
    if (!EnsureSourceIsStarted())
      StopSource();
  }

  // Create and initialize a new MediaStreamAudioTrack and pass ownership of it
  // to the WebMediaStreamTrack.
  blink::WebMediaStreamTrack mutable_blink_track = blink_track;
  mutable_blink_track.SetTrackData(
      CreateMediaStreamAudioTrack(blink_track.Id().Utf8()).release());

  // Propagate initial "enabled" state.
  MediaStreamAudioTrack* const track = MediaStreamAudioTrack::From(blink_track);
  DCHECK(track);
  track->SetEnabled(blink_track.IsEnabled());

  // If the source is stopped, do not start the track.
  if (is_stopped_)
    return false;

  track->Start(base::Bind(&MediaStreamAudioSource::StopAudioDeliveryTo,
                          weak_factory_.GetWeakPtr(), track));
  DVLOG(1) << "Adding MediaStreamAudioTrack@" << track
           << " as a consumer of MediaStreamAudioSource@" << this << '.';
  deliverer_.AddConsumer(track);
  return true;
}

media::AudioParameters MediaStreamAudioSource::GetAudioParameters() const {
  return deliverer_.GetAudioParameters();
}

bool MediaStreamAudioSource::RenderToAssociatedSinkEnabled() const {
  DCHECK(task_runner_->RunsTasksInCurrentSequence());
  return device().matched_output_device_id.has_value();
}

void* MediaStreamAudioSource::GetClassIdentifier() const {
  return nullptr;
}

std::unique_ptr<MediaStreamAudioTrack>
MediaStreamAudioSource::CreateMediaStreamAudioTrack(const std::string& id) {
  DCHECK(task_runner_->RunsTasksInCurrentSequence());
  return std::unique_ptr<MediaStreamAudioTrack>(
      new MediaStreamAudioTrack(is_local_source()));
}

bool MediaStreamAudioSource::EnsureSourceIsStarted() {
  DCHECK(task_runner_->RunsTasksInCurrentSequence());
  DVLOG(1) << "MediaStreamAudioSource@" << this << "::EnsureSourceIsStarted()";
  return true;
}

void MediaStreamAudioSource::EnsureSourceIsStopped() {
  DCHECK(task_runner_->RunsTasksInCurrentSequence());
  DVLOG(1) << "MediaStreamAudioSource@" << this << "::EnsureSourceIsStopped()";
}

void MediaStreamAudioSource::SetFormat(const media::AudioParameters& params) {
  DVLOG(1) << "MediaStreamAudioSource@" << this << "::SetFormat("
           << params.AsHumanReadableString() << "), was previously set to {"
           << deliverer_.GetAudioParameters().AsHumanReadableString() << "}.";
  deliverer_.OnSetFormat(params);
}

void MediaStreamAudioSource::DeliverDataToTracks(
    const media::AudioBus& audio_bus,
    base::TimeTicks reference_time) {
  deliverer_.OnData(audio_bus, reference_time);
}

void MediaStreamAudioSource::DoStopSource() {
  DCHECK(task_runner_->RunsTasksInCurrentSequence());
  EnsureSourceIsStopped();
  is_stopped_ = true;
}

void MediaStreamAudioSource::StopAudioDeliveryTo(MediaStreamAudioTrack* track) {
  DCHECK(task_runner_->RunsTasksInCurrentSequence());

  const bool did_remove_last_track = deliverer_.RemoveConsumer(track);
  DVLOG(1) << "Removed MediaStreamAudioTrack@" << track
           << " as a consumer of MediaStreamAudioSource@" << this << '.';

  // The W3C spec requires a source automatically stop when the last track is
  // stopped.
  if (!is_stopped_ && did_remove_last_track)
    MediaStreamSource::StopSource();
}

void MediaStreamAudioSource::StopSourceOnError(const std::string& why) {
  VLOG(1) << why;

  // Stop source when error occurs.
  task_runner_->PostTask(
      FROM_HERE, base::BindOnce(&MediaStreamSource::StopSource, GetWeakPtr()));
}

void MediaStreamAudioSource::SetMutedState(bool muted_state) {
  DVLOG(3) << "MediaStreamAudioSource::SetMutedState state=" << muted_state;
  task_runner_->PostTask(FROM_HERE,
                         base::BindOnce(&MediaStreamSource::SetSourceMuted,
                                        GetWeakPtr(), muted_state));
}

base::SingleThreadTaskRunner* MediaStreamAudioSource::GetTaskRunner() const {
  return task_runner_.get();
}

}  // namespace content
