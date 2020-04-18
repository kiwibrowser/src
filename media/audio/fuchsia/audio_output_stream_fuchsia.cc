// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "media/audio/fuchsia/audio_output_stream_fuchsia.h"

#include <media/audio.h>
#include <zircon/syscalls.h>

#include "media/audio/fuchsia/audio_manager_fuchsia.h"
#include "media/base/audio_sample_types.h"
#include "media/base/audio_timestamp_helper.h"

namespace media {

AudioOutputStreamFuchsia::AudioOutputStreamFuchsia(
    AudioManagerFuchsia* manager,
    const std::string& device_id,
    const AudioParameters& parameters)
    : manager_(manager),
      device_id_(device_id),
      parameters_(parameters),
      audio_bus_(AudioBus::Create(parameters)),
      buffer_(parameters_.frames_per_buffer() * parameters_.channels()) {}

AudioOutputStreamFuchsia::~AudioOutputStreamFuchsia() {
  // Close() must be called first.
  DCHECK(!stream_);
}

bool AudioOutputStreamFuchsia::Open() {
  DCHECK(!stream_);

  fuchsia_audio_parameters fuchsia_params;
  fuchsia_params.sample_rate = parameters_.sample_rate();
  fuchsia_params.num_channels = parameters_.channels();
  fuchsia_params.buffer_size = parameters_.frames_per_buffer();

  int result = fuchsia_audio_manager_create_output_stream(
      manager_->GetFuchsiaAudioManager(), const_cast<char*>(device_id_.c_str()),
      &fuchsia_params, &stream_);
  if (result < 0) {
    DLOG(ERROR) << "Failed to open audio output " << device_id_
                << " error code: " << result;
    DCHECK(!stream_);
    return false;
  }

  return true;
}

void AudioOutputStreamFuchsia::Start(AudioSourceCallback* callback) {
  DCHECK(!callback_);
  DCHECK(started_time_.is_null());
  callback_ = callback;

  PumpSamples();
}

void AudioOutputStreamFuchsia::Stop() {
  callback_ = nullptr;
  started_time_ = base::TimeTicks();
  timer_.Stop();
}

void AudioOutputStreamFuchsia::SetVolume(double volume) {
  DCHECK(0.0 <= volume && volume <= 1.0) << volume;
  volume_ = volume;
}

void AudioOutputStreamFuchsia::GetVolume(double* volume) {
  *volume = volume_;
}

void AudioOutputStreamFuchsia::Close() {
  Stop();

  if (stream_) {
    fuchsia_audio_output_stream_free(stream_);
    stream_ = nullptr;
  }

  // Signal to the manager that we're closed and can be removed. This should be
  // the last call in the function as it deletes "this".
  manager_->ReleaseOutputStream(this);
}

base::TimeTicks AudioOutputStreamFuchsia::GetCurrentStreamTime() {
  DCHECK(!started_time_.is_null());
  return started_time_ +
         AudioTimestampHelper::FramesToTime(stream_position_samples_,
                                            parameters_.sample_rate());
}

bool AudioOutputStreamFuchsia::UpdatePresentationDelay() {
  int result = fuchsia_audio_output_stream_get_min_delay(
      stream_, &presentation_delay_ns_);
  if (result != ZX_OK) {
    DLOG(ERROR) << "fuchsia_audio_output_stream_get_min_delay() failed: "
                << result;
    callback_->OnError();
    return false;
  }

  return true;
}

void AudioOutputStreamFuchsia::PumpSamples() {
  DCHECK(stream_);

  base::TimeTicks now = base::TimeTicks::Now();

  // Reset stream position if:
  //  1. The stream wasn't previously running.
  //  2. We missed timer deadline, e.g. after the system was suspended.
  if (started_time_.is_null() || now > GetCurrentStreamTime()) {
    if (!UpdatePresentationDelay())
      return;

    started_time_ = base::TimeTicks();
  }

  base::TimeDelta delay =
      base::TimeDelta::FromMicroseconds(presentation_delay_ns_ / 1000);
  if (!started_time_.is_null())
    delay += GetCurrentStreamTime() - now;

  int frames_filled = callback_->OnMoreData(delay, now, 0, audio_bus_.get());
  DCHECK_EQ(frames_filled, audio_bus_->frames());

  audio_bus_->Scale(volume_);
  audio_bus_->ToInterleaved<media::Float32SampleTypeTraits>(
      audio_bus_->frames(), buffer_.data());

  do {
    zx_time_t presentation_time = FUCHSIA_AUDIO_NO_TIMESTAMP;
    if (started_time_.is_null()) {
      // Presentation time (PTS) needs to be specified only for the first frame
      // after stream is started or restarted. Mixer will calculate PTS for all
      // following frames. 1us is added to account for the time passed between
      // zx_clock_get() and fuchsia_audio_output_stream_write().
      zx_time_t zx_now = zx_clock_get(ZX_CLOCK_MONOTONIC);
      presentation_time = zx_now + presentation_delay_ns_ + ZX_USEC(1);
      started_time_ = base::TimeTicks::FromZxTime(zx_now);
      stream_position_samples_ = 0;
    }
    int result = fuchsia_audio_output_stream_write(
        stream_, buffer_.data(), buffer_.size(), presentation_time);
    if (result == ZX_ERR_IO_MISSED_DEADLINE) {
      DLOG(ERROR) << "AudioOutputStreamFuchsia::PumpSamples() missed deadline, "
                     "resetting PTS.";
      if (!UpdatePresentationDelay())
        return;
      started_time_ = base::TimeTicks();
    } else if (result != ZX_OK) {
      DLOG(ERROR) << "fuchsia_audio_output_stream_write() returned " << result;
      callback_->OnError();
    }
  } while (started_time_.is_null());

  stream_position_samples_ += frames_filled;

  timer_.Start(FROM_HERE,
               GetCurrentStreamTime() - base::TimeTicks::Now() -
                   parameters_.GetBufferDuration() / 2,
               base::Bind(&AudioOutputStreamFuchsia::PumpSamples,
                          base::Unretained(this)));
}

}  // namespace media
