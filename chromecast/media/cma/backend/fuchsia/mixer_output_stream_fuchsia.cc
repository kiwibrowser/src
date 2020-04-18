// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chromecast/media/cma/backend/fuchsia/mixer_output_stream_fuchsia.h"

#include <media/audio.h>
#include <zircon/syscalls.h>

#include "base/command_line.h"
#include "base/time/time.h"
#include "chromecast/base/chromecast_switches.h"
#include "media/base/audio_sample_types.h"
#include "media/base/audio_timestamp_helper.h"
#include "media/base/media_switches.h"

namespace chromecast {
namespace media {

// |buffer_size| passed to media_client library when initializing audio output
// stream. Current implementation ignores this parameter, so the value doesn't
// make much difference. StreamMixer by default writes chunks of 768 frames.
constexpr int kDefaultPeriodSize = 768;

// Same value as in MixerOutputStreamAlsa. Currently this value is used to
// simulate blocking Write() similar to ALSA's behavior, see comments in
// MixerOutputStreamFuchsia::Write().
constexpr int kMaxOutputBufferSizeFrames = 4096;

// static
std::unique_ptr<MixerOutputStream> MixerOutputStream::Create() {
  return std::make_unique<MixerOutputStreamFuchsia>();
}

MixerOutputStreamFuchsia::MixerOutputStreamFuchsia() {}

MixerOutputStreamFuchsia::~MixerOutputStreamFuchsia() {
  if (manager_)
    fuchsia_audio_manager_free(manager_);
}

bool MixerOutputStreamFuchsia::Start(int requested_sample_rate, int channels) {
  DCHECK(!stream_);

  if (!manager_)
    manager_ = fuchsia_audio_manager_create();

  DCHECK(manager_);

  fuchsia_audio_parameters fuchsia_params;
  fuchsia_params.sample_rate = requested_sample_rate;
  fuchsia_params.num_channels = channels;
  fuchsia_params.buffer_size = kDefaultPeriodSize;

  int result = fuchsia_audio_manager_create_output_stream(
      manager_, nullptr, &fuchsia_params, &stream_);
  if (result < 0) {
    LOG(ERROR) << "Failed to open audio output, error code: " << result;
    DCHECK(!stream_);
    return false;
  }

  if (!UpdatePresentationDelay()) {
    fuchsia_audio_output_stream_free(stream_);
    stream_ = nullptr;
    return false;
  }

  sample_rate_ = requested_sample_rate;
  channels_ = channels;

  started_time_ = base::TimeTicks();

  return true;
}

int MixerOutputStreamFuchsia::GetSampleRate() {
  return sample_rate_;
}

MediaPipelineBackend::AudioDecoder::RenderingDelay
MixerOutputStreamFuchsia::GetRenderingDelay() {
  base::TimeTicks now = base::TimeTicks::Now();
  base::TimeDelta delay =
      base::TimeDelta::FromMicroseconds(presentation_delay_ns_ / 1000);
  if (!started_time_.is_null()) {
    base::TimeTicks stream_time = GetCurrentStreamTime();
    if (stream_time > now)
      delay += stream_time - now;
  }

  return MediaPipelineBackend::AudioDecoder::RenderingDelay(
      /*delay_microseconds=*/delay.InMicroseconds(),
      /*timestamp_microseconds=*/(now - base::TimeTicks()).InMicroseconds());
}

int MixerOutputStreamFuchsia::OptimalWriteFramesCount() {
  return kDefaultPeriodSize;
}

bool MixerOutputStreamFuchsia::Write(const float* data,
                                     int data_size,
                                     bool* out_playback_interrupted) {
  if (!stream_)
    return false;

  DCHECK(data_size % channels_ == 0);

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
        stream_, const_cast<float*>(data), data_size, presentation_time);
    if (result == ZX_ERR_IO_MISSED_DEADLINE) {
      LOG(ERROR) << "MixerOutputStreamFuchsia::PumpSamples() missed deadline, "
                    "resetting PTS.";
      if (!UpdatePresentationDelay()) {
        return false;
      }
      started_time_ = base::TimeTicks();
      *out_playback_interrupted = true;
    } else if (result != ZX_OK) {
      LOG(ERROR) << "fuchsia_audio_output_stream_write() returned " << result;
      return false;
    }

  } while (started_time_.is_null());

  int frames = data_size / channels_;
  stream_position_samples_ += frames;

  // Block the thread to limit amount of buffered data. Currently
  // MixerOutputStreamAlsa uses blocking Write() and StreamMixer relies on that
  // behavior. Sleep() below replicates the same behavior on Fuchsia.
  // TODO(sergeyu): Refactor StreamMixer to work with non-blocking Write().
  base::TimeDelta max_buffer_duration =
      ::media::AudioTimestampHelper::FramesToTime(kMaxOutputBufferSizeFrames,
                                                  sample_rate_);
  base::TimeDelta current_buffer_duration =
      GetCurrentStreamTime() - base::TimeTicks::Now();
  if (current_buffer_duration > max_buffer_duration)
    base::PlatformThread::Sleep(current_buffer_duration - max_buffer_duration);

  return true;
}

void MixerOutputStreamFuchsia::Stop() {
  started_time_ = base::TimeTicks();

  if (stream_) {
    fuchsia_audio_output_stream_free(stream_);
    stream_ = nullptr;
  }
}

bool MixerOutputStreamFuchsia::UpdatePresentationDelay() {
  int result = fuchsia_audio_output_stream_get_min_delay(
      stream_, &presentation_delay_ns_);
  if (result != ZX_OK) {
    LOG(ERROR) << "fuchsia_audio_output_stream_get_min_delay() failed: "
               << result;
    return false;
  }

  return true;
}

base::TimeTicks MixerOutputStreamFuchsia::GetCurrentStreamTime() {
  DCHECK(!started_time_.is_null());
  return started_time_ + ::media::AudioTimestampHelper::FramesToTime(
                             stream_position_samples_, sample_rate_);
}

}  // namespace media
}  // namespace chromecast
