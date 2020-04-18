// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROMECAST_MEDIA_CMA_BACKEND_MOCK_AUDIO_DECODER_FOR_MIXER_H_
#define CHROMECAST_MEDIA_CMA_BACKEND_MOCK_AUDIO_DECODER_FOR_MIXER_H_

#include <stdint.h>
#include <memory>

#include "base/macros.h"
#include "base/timer/timer.h"
#include "chromecast/media/cma/backend/audio_decoder_for_mixer.h"

namespace chromecast {
namespace media {

class MediaPipelineBackendForMixer;

class MockAudioDecoderForMixer : public AudioDecoderForMixer {
 public:
  static std::unique_ptr<MockAudioDecoderForMixer> Create(
      MediaPipelineBackendForMixer* backend);

  explicit MockAudioDecoderForMixer(MediaPipelineBackendForMixer* backend);
  ~MockAudioDecoderForMixer() override;

  // AudioDecoderForMixer implementation:
  void Initialize() override;
  bool Start(int64_t timestamp) override;
  void Stop() override;
  bool Pause() override;
  bool Resume() override;
  float SetPlaybackRate(float rate) override;
  int64_t GetCurrentPts() const override;

 private:
  void PushBufferPeriodic();
  void PlayAudio();

  base::RepeatingTimer data_push_timer_;
  base::RepeatingTimer audio_play_timer_;

  // TODO(almasrymina): to enhance the tests further, we may want to add tests
  // that tweak those params, such as add non-contsant mixer delay.
  int64_t next_push_buffer_pts_ = INT64_MIN;
  int64_t audio_buffer_duration_us_ = 100000;
  int64_t audio_push_buffer_internal_us_ = 20000;
  int64_t mixer_latency_us_ = 100000;
  int64_t current_audio_pts_ = INT64_MIN;
  int64_t audio_play_interval_us_ = 1000;

  DISALLOW_COPY_AND_ASSIGN(MockAudioDecoderForMixer);
};

}  // namespace media
}  // namespace chromecast

#endif  // CHROMECAST_MEDIA_CMA_BACKEND_MOCK_AUDIO_DECODER_FOR_MIXER_H_
