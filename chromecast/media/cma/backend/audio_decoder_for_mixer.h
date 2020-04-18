// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROMECAST_MEDIA_CMA_BACKEND_AUDIO_DECODER_FOR_MIXER_H_
#define CHROMECAST_MEDIA_CMA_BACKEND_AUDIO_DECODER_FOR_MIXER_H_

#include <memory>

#include "base/bind.h"
#include "base/containers/circular_deque.h"
#include "base/location.h"
#include "chromecast/media/cma/backend/buffering_mixer_source.h"
#include "chromecast/media/cma/decoder/cast_audio_decoder.h"
#include "chromecast/public/media/decoder_config.h"
#include "chromecast/public/media/media_pipeline_backend.h"
#include "chromecast/public/media/media_pipeline_device_params.h"
#include "media/base/audio_buffer.h"

namespace base {
class SingleThreadTaskRunner;
}  // namespace base

namespace media {
class AudioBus;
class AudioRendererAlgorithm;
}  // namespace media

namespace chromecast {
namespace media {
class DecoderBufferBase;
class MediaPipelineBackendForMixer;

// AudioDecoder implementation that streams decoded stream to the StreamMixer.
class AudioDecoderForMixer : public MediaPipelineBackend::AudioDecoder,
                             public BufferingMixerSource::Delegate {
 public:
  using BufferStatus = MediaPipelineBackend::BufferStatus;

  explicit AudioDecoderForMixer(MediaPipelineBackendForMixer* backend);
  ~AudioDecoderForMixer() override;

  virtual void Initialize();
  virtual bool Start(int64_t timestamp);
  virtual void Stop();
  virtual bool Pause();
  virtual bool Resume();
  virtual float SetPlaybackRate(float rate);
  virtual int64_t GetCurrentPts() const;

  // MediaPipelineBackend::AudioDecoder implementation:
  void SetDelegate(MediaPipelineBackend::Decoder::Delegate* delegate) override;
  BufferStatus PushBuffer(CastDecoderBuffer* buffer) override;
  void GetStatistics(Statistics* statistics) override;
  bool SetConfig(const AudioConfig& config) override;
  bool SetVolume(float multiplier) override;
  RenderingDelay GetRenderingDelay() override;

 private:
  friend class MockAudioDecoderForMixer;
  friend class AvSyncTest;

  struct RateShifterInfo {
    explicit RateShifterInfo(float playback_rate);

    double rate;
    double input_frames;
    int64_t output_frames;
  };

  // BufferingMixerSource::Delegate implementation:
  void OnWritePcmCompletion(RenderingDelay delay) override;
  void OnMixerError(MixerError error) override;
  void OnEos() override;

  void CleanUpPcm();
  void CreateDecoder();
  void CreateRateShifter(int samples_per_second);
  void OnDecoderInitialized(bool success);
  void OnBufferDecoded(uint64_t input_bytes,
                       CastAudioDecoder::Status status,
                       const scoped_refptr<DecoderBufferBase>& decoded);
  void CheckBufferComplete();
  void PushRateShifted();
  void PushMorePcm();
  void RunEos();
  bool BypassDecoder() const;
  bool ShouldStartClock() const;
  void UpdateStatistics(Statistics delta);

  MediaPipelineBackendForMixer* const backend_;
  const scoped_refptr<base::SingleThreadTaskRunner> task_runner_;
  MediaPipelineBackend::Decoder::Delegate* delegate_;

  Statistics stats_;

  bool pending_buffer_complete_;
  bool got_eos_;
  bool pushed_eos_;
  bool mixer_error_;

  AudioConfig config_;
  std::unique_ptr<CastAudioDecoder> decoder_;

  std::unique_ptr<::media::AudioRendererAlgorithm> rate_shifter_;
  base::circular_deque<RateShifterInfo> rate_shifter_info_;
  std::unique_ptr<::media::AudioBus> rate_shifter_output_;

  int64_t first_push_pts_;
  int64_t last_push_pts_;
  int64_t last_push_timestamp_;
  int64_t last_push_pts_length_;
  int64_t paused_pts_;

  std::unique_ptr<BufferingMixerSource, BufferingMixerSource::Deleter>
      mixer_input_;
  RenderingDelay last_mixer_delay_;
  int64_t pending_output_frames_;
  float volume_multiplier_;

  scoped_refptr<::media::AudioBufferMemoryPool> pool_;

  int64_t playback_start_timestamp_ = 0;

  base::WeakPtrFactory<AudioDecoderForMixer> weak_factory_;

  DISALLOW_COPY_AND_ASSIGN(AudioDecoderForMixer);
};

}  // namespace media
}  // namespace chromecast

#endif  // CHROMECAST_MEDIA_CMA_BACKEND_AUDIO_DECODER_FOR_MIXER_H_
