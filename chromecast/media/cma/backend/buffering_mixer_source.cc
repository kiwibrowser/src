// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chromecast/media/cma/backend/buffering_mixer_source.h"

#include <stdint.h>
#include <string.h>

#include <algorithm>
#include <utility>

#include "base/bind.h"
#include "base/location.h"
#include "base/logging.h"
#include "base/single_thread_task_runner.h"
#include "base/threading/thread_task_runner_handle.h"
#include "chromecast/media/cma/backend/stream_mixer.h"
#include "chromecast/media/cma/base/decoder_buffer_base.h"
#include "media/audio/audio_device_description.h"
#include "media/base/audio_bus.h"
#include "media/base/audio_timestamp_helper.h"

#define POST_TASK_TO_CALLER_THREAD(task, ...)                               \
  shim_task_runner_->PostTask(                                              \
      FROM_HERE, base::BindOnce(&PostTaskShim, caller_task_runner_,         \
                                base::BindOnce(&BufferingMixerSource::task, \
                                               weak_this_, ##__VA_ARGS__)));

namespace chromecast {
namespace media {

namespace {

const int kNumOutputChannels = 2;
const int64_t kInputQueueMs = 90;
const int kFadeTimeMs = 5;

// Special queue size and start threshold for "communications" streams to avoid
// issues with voice calling.
const int64_t kCommsInputQueueMs = 200;
const int64_t kCommsStartThresholdMs = 150;

void PostTaskShim(scoped_refptr<base::SingleThreadTaskRunner> task_runner,
                  base::OnceClosure task) {
  task_runner->PostTask(FROM_HERE, std::move(task));
}

std::string AudioContentTypeToString(media::AudioContentType type) {
  switch (type) {
    case media::AudioContentType::kAlarm:
      return "alarm";
    case media::AudioContentType::kCommunication:
      return "communication";
    default:
      return "media";
  }
}

int MsToSamples(int64_t ms, int sample_rate) {
  return ::media::AudioTimestampHelper::TimeToFrames(
      base::TimeDelta::FromMilliseconds(ms), sample_rate);
}

int64_t SamplesToMicroseconds(int64_t samples, int sample_rate) {
  return ::media::AudioTimestampHelper::FramesToTime(samples, sample_rate)
      .InMicroseconds();
}

int MaxQueuedFrames(const std::string& device_id, int sample_rate) {
  if (device_id == ::media::AudioDeviceDescription::kCommunicationsDeviceId) {
    return MsToSamples(kCommsInputQueueMs, sample_rate);
  }
  return MsToSamples(kInputQueueMs, sample_rate);
}

int StartThreshold(const std::string& device_id, int sample_rate) {
  if (device_id == ::media::AudioDeviceDescription::kCommunicationsDeviceId) {
    return MsToSamples(kCommsStartThresholdMs, sample_rate);
  }
  return 0;
}

}  // namespace

BufferingMixerSource::LockedMembers::Members::Members(
    BufferingMixerSource* source,
    int input_samples_per_second,
    int num_channels)
    : state_(State::kUninitialized),
      paused_(false),
      mixer_error_(false),
      queued_frames_(0),
      extra_delay_frames_(0),
      current_buffer_offset_(0),
      fader_(source,
             num_channels,
             MsToSamples(kFadeTimeMs, input_samples_per_second)),
      zero_fader_frames_(false),
      started_(false) {}

BufferingMixerSource::LockedMembers::Members::~Members() = default;

BufferingMixerSource::LockedMembers::AcquiredLock::AcquiredLock(
    LockedMembers* members)
    : locked_(members) {
  DCHECK(locked_);
  locked_->member_lock_.Acquire();
}

BufferingMixerSource::LockedMembers::AcquiredLock::~AcquiredLock() {
  locked_->member_lock_.Release();
}

BufferingMixerSource::LockedMembers::AssertedLock::AssertedLock(
    LockedMembers* members)
    : locked_(members) {
  DCHECK(locked_);
  locked_->member_lock_.AssertAcquired();
}

BufferingMixerSource::LockedMembers::LockedMembers(BufferingMixerSource* source,
                                                   int input_samples_per_second,
                                                   int num_channels)
    : members_(source, input_samples_per_second, num_channels) {}

BufferingMixerSource::LockedMembers::~LockedMembers() = default;

BufferingMixerSource::LockedMembers::AcquiredLock
BufferingMixerSource::LockedMembers::Lock() {
  return AcquiredLock(this);
}

BufferingMixerSource::LockedMembers::AssertedLock
BufferingMixerSource::LockedMembers::AssertAcquired() {
  return AssertedLock(this);
}

BufferingMixerSource::BufferingMixerSource(Delegate* delegate,
                                           int input_samples_per_second,
                                           bool primary,
                                           const std::string& device_id,
                                           AudioContentType content_type,
                                           int playout_channel,
                                           int64_t playback_start_timestamp)
    : delegate_(delegate),
      num_channels_(kNumOutputChannels),
      input_samples_per_second_(input_samples_per_second),
      primary_(primary),
      device_id_(device_id),
      content_type_(content_type),
      playout_channel_(playout_channel),
      mixer_(StreamMixer::Get()),
      caller_task_runner_(base::ThreadTaskRunnerHandle::Get()),
      shim_task_runner_(mixer_->shim_task_runner()),
      max_queued_frames_(MaxQueuedFrames(device_id, input_samples_per_second)),
      start_threshold_frames_(
          StartThreshold(device_id, input_samples_per_second)),
      playback_start_timestamp_(playback_start_timestamp),
      locked_members_(this, input_samples_per_second, num_channels_),
      weak_factory_(this) {
  LOG(INFO) << "Create " << device_id_ << " (" << this
            << "), content type = " << AudioContentTypeToString(content_type_);
  DCHECK(delegate_);
  DCHECK(mixer_);
  DCHECK_LE(start_threshold_frames_, max_queued_frames_);
  weak_this_ = weak_factory_.GetWeakPtr();

  mixer_->AddInput(this);
}

BufferingMixerSource::~BufferingMixerSource() {
  LOG(INFO) << "Destroy " << device_id_ << " (" << this << ")";
}

int BufferingMixerSource::num_channels() {
  return num_channels_;
}
int BufferingMixerSource::input_samples_per_second() {
  return input_samples_per_second_;
}
bool BufferingMixerSource::primary() {
  return primary_;
}
const std::string& BufferingMixerSource::device_id() {
  return device_id_;
}
AudioContentType BufferingMixerSource::content_type() {
  return content_type_;
}
int BufferingMixerSource::desired_read_size() {
  return input_samples_per_second_ / 100;
}
int BufferingMixerSource::playout_channel() {
  return playout_channel_;
}

void BufferingMixerSource::WritePcm(scoped_refptr<DecoderBufferBase> data) {
  DCHECK(caller_task_runner_->BelongsToCurrentThread());

  auto locked = locked_members_.Lock();
  if (locked->state_ == State::kUninitialized ||
      locked->queued_frames_ + locked->fader_.buffered_frames() >=
          max_queued_frames_) {
    DCHECK(!locked->pending_data_);
    locked->pending_data_ = std::move(data);
    return;
  }
  RenderingDelay delay = QueueData(std::move(data));
  PostPcmCompletion(delay);
}

BufferingMixerSource::RenderingDelay BufferingMixerSource::QueueData(
    scoped_refptr<DecoderBufferBase> data) {
  auto locked = locked_members_.AssertAcquired();
  if (data->end_of_stream()) {
    LOG(INFO) << "End of stream for " << device_id_ << " (" << this << ")";
    locked->state_ = State::kGotEos;
  } else {
    const int frames = data->data_size() / (num_channels_ * sizeof(float));
    locked->queued_frames_ += frames;
    locked->queue_.push_back(std::move(data));
  }

  RenderingDelay delay;
  if (locked->started_) {
    delay = locked->mixer_rendering_delay_;
    delay.delay_microseconds += SamplesToMicroseconds(
        locked->queued_frames_ + locked->extra_delay_frames_,
        input_samples_per_second_);
  }
  return delay;
}

void BufferingMixerSource::SetPaused(bool paused) {
  LOG(INFO) << (paused ? "Pausing " : "Unpausing ") << device_id_ << " ("
            << this << ")";
  auto locked = locked_members_.Lock();
  locked->paused_ = paused;
}

void BufferingMixerSource::SetVolumeMultiplier(float multiplier) {
  mixer_->SetVolumeMultiplier(this, multiplier);
}

void BufferingMixerSource::InitializeAudioPlayback(
    int read_size,
    RenderingDelay initial_rendering_delay) {
  // Start accepting buffers into the queue.
  bool queued_data = false;
  RenderingDelay pending_buffer_delay;
  {
    auto locked = locked_members_.Lock();
    locked->mixer_rendering_delay_ = initial_rendering_delay;
    if (locked->state_ == State::kUninitialized) {
      locked->state_ = State::kNormalPlayback;
    } else {
      DCHECK(locked->state_ == State::kRemoved);
    }

    if (locked->pending_data_ &&
        locked->queued_frames_ + locked->fader_.buffered_frames() <
            max_queued_frames_) {
      pending_buffer_delay = QueueData(std::move(locked->pending_data_));
      queued_data = true;
    }
  }

  if (queued_data) {
    POST_TASK_TO_CALLER_THREAD(PostPcmCompletion, pending_buffer_delay);
  }
}

int BufferingMixerSource::FillAudioPlaybackFrames(
    int num_frames,
    RenderingDelay rendering_delay,
    ::media::AudioBus* buffer) {
  DCHECK(buffer);
  DCHECK_EQ(num_channels_, buffer->channels());
  DCHECK_GE(buffer->frames(), num_frames);

  int64_t playback_absolute_timestamp = rendering_delay.delay_microseconds +
                                        rendering_delay.timestamp_microseconds;

  // Don't write to the mixer yet if it's not time to start playback yet.
  //
  // TODO(almasrymina): mixer behaviour has playback_absolute_timestamp go up
  // in chunks of 10ms, so we're going to start playback at
  // playback_start_timestamp_ accurate to +10ms. Improve this to be sample
  // accurate by writing a partial silence buffer when it's time to start
  // playback.
  if (playback_absolute_timestamp < playback_start_timestamp_) {
    return 0;
  }

  int filled = 0;
  bool queued_more_data = false;
  bool signal_eos = false;
  bool remove_self = false;
  RenderingDelay pending_buffer_delay;
  {
    auto locked = locked_members_.Lock();

    // In normal playback, don't pass data to the fader if we can't satisfy the
    // full request. This will allow us to buffer up more data so we can fully
    // fade in.
    if (locked->state_ == State::kNormalPlayback &&
        (locked->queued_frames_ <
             locked->fader_.FramesNeededFromSource(num_frames) ||
         (!locked->started_ &&
          locked->queued_frames_ < start_threshold_frames_))) {
      if (locked->started_) {
        LOG(INFO) << "Stream underrun for " << device_id_ << " (" << this
                  << ")";
      }
      locked->zero_fader_frames_ = true;
      locked->started_ = false;
    } else {
      locked->zero_fader_frames_ = false;
      locked->started_ = true;
    }

    filled = locked->fader_.FillFrames(num_frames, buffer);

    locked->mixer_rendering_delay_ = rendering_delay;
    locked->extra_delay_frames_ = num_frames + locked->fader_.buffered_frames();

    // See if we can accept more data into the queue.
    if (locked->pending_data_ &&
        locked->queued_frames_ + locked->fader_.buffered_frames() <
            max_queued_frames_) {
      pending_buffer_delay = QueueData(std::move(locked->pending_data_));
      queued_more_data = true;
    }

    // Check if we have played out EOS.
    if (locked->state_ == State::kGotEos && locked->queued_frames_ == 0 &&
        locked->fader_.buffered_frames() == 0) {
      signal_eos = true;
      locked->state_ = State::kSignaledEos;
    }

    // If the caller has removed this source, delete once we have faded out.
    if (locked->state_ == State::kRemoved &&
        locked->fader_.buffered_frames() == 0) {
      remove_self = true;
    }
  }

  if (queued_more_data) {
    POST_TASK_TO_CALLER_THREAD(PostPcmCompletion, pending_buffer_delay);
  }
  if (signal_eos) {
    POST_TASK_TO_CALLER_THREAD(PostEos);
  }

  if (remove_self) {
    mixer_->RemoveInput(this);
  }
  return filled;
}

int BufferingMixerSource::FillFaderFrames(::media::AudioBus* dest,
                                          int frame_offset,
                                          int num_frames) {
  DCHECK(dest);
  DCHECK_EQ(num_channels_, dest->channels());
  auto locked = locked_members_.AssertAcquired();

  if (locked->zero_fader_frames_ || locked->paused_ ||
      locked->state_ == State::kRemoved) {
    return 0;
  }

  int num_filled = 0;
  while (num_frames) {
    if (locked->queue_.empty()) {
      return num_filled;
    }

    DecoderBufferBase* buffer = locked->queue_.front().get();
    const int buffer_frames =
        buffer->data_size() / (num_channels_ * sizeof(float));
    const int frames_to_copy =
        std::min(num_frames, buffer_frames - locked->current_buffer_offset_);
    const float* buffer_samples =
        reinterpret_cast<const float*>(buffer->data());
    for (int c = 0; c < num_channels_; ++c) {
      const float* buffer_channel = buffer_samples + (buffer_frames * c);
      memcpy(dest->channel(c) + frame_offset,
             buffer_channel + locked->current_buffer_offset_,
             frames_to_copy * sizeof(float));
    }

    num_frames -= frames_to_copy;
    locked->queued_frames_ -= frames_to_copy;
    frame_offset += frames_to_copy;
    num_filled += frames_to_copy;

    locked->current_buffer_offset_ += frames_to_copy;
    if (locked->current_buffer_offset_ == buffer_frames) {
      locked->queue_.pop_front();
      locked->current_buffer_offset_ = 0;
    }
  }

  return num_filled;
}

void BufferingMixerSource::PostPcmCompletion(RenderingDelay delay) {
  DCHECK(caller_task_runner_->BelongsToCurrentThread());
  delegate_->OnWritePcmCompletion(delay);
}

void BufferingMixerSource::PostEos() {
  DCHECK(caller_task_runner_->BelongsToCurrentThread());
  delegate_->OnEos();
}

void BufferingMixerSource::OnAudioPlaybackError(MixerError error) {
  if (error == MixerError::kInputIgnored) {
    LOG(INFO) << "Mixer input " << device_id_ << " (" << this << ")"
              << " now being ignored due to output sample rate change";
  }

  POST_TASK_TO_CALLER_THREAD(PostError, error);

  auto locked = locked_members_.Lock();
  locked->mixer_error_ = true;
  if (locked->state_ == State::kRemoved) {
    mixer_->RemoveInput(this);
  }
}

void BufferingMixerSource::PostError(MixerError error) {
  DCHECK(caller_task_runner_->BelongsToCurrentThread());
  delegate_->OnMixerError(error);
}

void BufferingMixerSource::Remove() {
  DCHECK(caller_task_runner_->BelongsToCurrentThread());
  weak_factory_.InvalidateWeakPtrs();

  LOG(INFO) << "Remove " << device_id_ << " (" << this << ")";
  bool remove_self = false;
  {
    auto locked = locked_members_.Lock();
    locked->pending_data_ = nullptr;
    locked->state_ = State::kRemoved;
    remove_self = locked->mixer_error_;
  }

  if (remove_self) {
    mixer_->RemoveInput(this);
  }
}

void BufferingMixerSource::FinalizeAudioPlayback() {
  delete this;
}

}  // namespace media
}  // namespace chromecast
