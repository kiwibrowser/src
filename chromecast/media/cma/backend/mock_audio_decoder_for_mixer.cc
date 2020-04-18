// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chromecast/media/cma/backend/mock_audio_decoder_for_mixer.h"

#include "base/logging.h"
#include "base/test/test_mock_time_task_runner.h"

namespace chromecast {
namespace media {

std::unique_ptr<MockAudioDecoderForMixer> MockAudioDecoderForMixer::Create(
    MediaPipelineBackendForMixer* backend) {
  return std::make_unique<MockAudioDecoderForMixer>(backend);
}

MockAudioDecoderForMixer::MockAudioDecoderForMixer(
    MediaPipelineBackendForMixer* backend)
    : AudioDecoderForMixer(backend) {}

MockAudioDecoderForMixer::~MockAudioDecoderForMixer() {}

void MockAudioDecoderForMixer::PushBufferPeriodic() {
  auto now_ticks =
      static_cast<base::TestMockTimeTaskRunner*>(task_runner_.get())
          ->NowTicks();

  int64_t buffer_timestamp = next_push_buffer_pts_;
  int64_t audio_play_delay =
      next_push_buffer_pts_ - current_audio_pts_ + mixer_latency_us_;
  next_push_buffer_pts_ += audio_buffer_duration_us_;
  DCHECK_GT(audio_play_delay, 0);

  MediaPipelineBackend::AudioDecoder::RenderingDelay delay(
      audio_play_delay, (now_ticks - base::TimeTicks()).InMicroseconds());

  VLOG(4) << "audio_buffer_pushed"
          << " current_audio_pts_=" << current_audio_pts_
          << " buffer_timestamp=" << buffer_timestamp
          << " delay.timestamp_microseconds=" << delay.timestamp_microseconds
          << " delay.delay_microseconds=" << delay.delay_microseconds;
}

void MockAudioDecoderForMixer::PlayAudio() {
  // This checks that we're not underruning, which I'm not interested in mocking
  // at this time. We may want to improve the tests to assert that AV sync is
  // maintained after an audio glitch.
  if (next_push_buffer_pts_ >= (current_audio_pts_ + audio_play_interval_us_)) {
    current_audio_pts_ += audio_play_interval_us_;
    VLOG(4) << "current_audio_pts_=" << current_audio_pts_;
  }
}

void MockAudioDecoderForMixer::Initialize() {}

bool MockAudioDecoderForMixer::Start(int64_t timestamp) {
  next_push_buffer_pts_ = 0;
  current_audio_pts_ = 0;
  data_push_timer_.Start(
      FROM_HERE,
      base::TimeDelta::FromMicroseconds(audio_push_buffer_internal_us_), this,
      &MockAudioDecoderForMixer::PushBufferPeriodic);
  audio_play_timer_.Start(
      FROM_HERE, base::TimeDelta::FromMicroseconds(audio_play_interval_us_),
      this, &MockAudioDecoderForMixer::PlayAudio);
  return true;
}

void MockAudioDecoderForMixer::Stop() {
  next_push_buffer_pts_ = INT64_MIN;
  data_push_timer_.Stop();
  audio_play_timer_.Stop();
}

bool MockAudioDecoderForMixer::Pause() {
  data_push_timer_.Stop();
  audio_play_timer_.Stop();
  return true;
}

bool MockAudioDecoderForMixer::Resume() {
  data_push_timer_.Start(
      FROM_HERE,
      base::TimeDelta::FromMicroseconds(audio_push_buffer_internal_us_), this,
      &MockAudioDecoderForMixer::PushBufferPeriodic);
  audio_play_timer_.Start(
      FROM_HERE, base::TimeDelta::FromMicroseconds(audio_play_interval_us_),
      this, &MockAudioDecoderForMixer::PlayAudio);
  return true;
}

float MockAudioDecoderForMixer::SetPlaybackRate(float rate) {
  return 1.0;
}

int64_t MockAudioDecoderForMixer::GetCurrentPts() const {
  return current_audio_pts_;
}

}  // namespace media
}  // namespace chromecast
