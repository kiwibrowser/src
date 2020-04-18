// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROMECAST_MEDIA_CMA_BACKEND_DIRECT_MIXER_SOURCE_H_
#define CHROMECAST_MEDIA_CMA_BACKEND_DIRECT_MIXER_SOURCE_H_

#include <string>
#include <vector>

#include "base/macros.h"
#include "chromecast/media/cma/backend/mixer_input.h"
#include "chromecast/public/media/media_pipeline_backend.h"
#include "chromecast/public/volume_control.h"

namespace media {
class AudioBus;
}  // namespace media

namespace chromecast {
namespace media {
class DirectAudioSource;
struct MediaPipelineDeviceParams;
class StreamMixer;

// Empty interface so we can use a pointer to DirectMixerSource as the token.
class DirectAudioSourceToken {};

// A simple adapter for DirectAudioSource to map the public API to the
// MixerInput::Source API.
class DirectMixerSource : public MixerInput::Source,
                          public DirectAudioSourceToken {
 public:
  using RenderingDelay = MediaPipelineBackend::AudioDecoder::RenderingDelay;

  DirectMixerSource(DirectAudioSource* direct_source,
                    const MediaPipelineDeviceParams& params,
                    int playout_channel);

  // Sets the volume multiplier for this stream. If |multiplier| < 0, sets the
  // volume multiplier to 0.
  void SetVolumeMultiplier(float multiplier);

  // Removes this source from the mixer asynchronously. After this method is
  // called, no more calls will be made to delegate methods. The source will
  // be removed from the mixer once it has faded out appropriately.
  void Remove();

 private:
  ~DirectMixerSource() override;

  // MixerInput::Source implementation:
  int num_channels() override;
  int input_samples_per_second() override;
  bool primary() override;
  const std::string& device_id() override;
  AudioContentType content_type() override;
  int desired_read_size() override;
  int playout_channel() override;

  void InitializeAudioPlayback(int read_size,
                               RenderingDelay initial_rendering_delay) override;
  int FillAudioPlaybackFrames(int num_frames,
                              RenderingDelay rendering_delay,
                              ::media::AudioBus* buffer) override;
  void OnAudioPlaybackError(MixerError error) override;
  void FinalizeAudioPlayback() override;

  DirectAudioSource* const source_;
  const int num_channels_;
  const int input_samples_per_second_;
  const bool primary_;
  const std::string device_id_;
  const AudioContentType content_type_;
  const int playout_channel_;
  StreamMixer* const mixer_;

  std::vector<float*> channel_vector_;

  DISALLOW_COPY_AND_ASSIGN(DirectMixerSource);
};

}  // namespace media
}  // namespace chromecast

#endif  // CHROMECAST_MEDIA_CMA_BACKEND_DIRECT_MIXER_SOURCE_H_
