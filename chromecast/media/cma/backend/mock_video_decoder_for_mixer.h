// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROMECAST_MEDIA_CMA_BACKEND_MOCK_VIDEO_DECODER_FOR_MIXER_H_
#define CHROMECAST_MEDIA_CMA_BACKEND_MOCK_VIDEO_DECODER_FOR_MIXER_H_

#include <stdint.h>
#include <memory>

#include "base/timer/timer.h"
#include "chromecast/media/cma/backend/video_decoder_for_mixer.h"
#include "chromecast/public/media/media_pipeline_backend.h"
#include "chromecast/public/media/media_pipeline_device_params.h"

namespace chromecast {
namespace media {

class VideoDecoderForTest : public VideoDecoderForMixer {
 public:
  virtual int64_t GetExpectedDroppedFrames() = 0;
  virtual int64_t GetExpectedRepeatedFrames() = 0;
  virtual int64_t GetNumberOfFramesTolerated() = 0;
  virtual int64_t GetNumberOfHardCorrectionsTolerated() = 0;
  virtual int64_t GetNumberOfSoftCorrectionsTolerated() = 0;
  virtual int64_t GetAvSyncDriftTolerated() = 0;
};

template <int64_t kClockRateNumerator,
          int64_t kClockRateDenominator,
          int64_t kContentFps>
class MockVideoDecoderForMixer : public VideoDecoderForTest {
 public:
  static std::unique_ptr<VideoDecoderForTest> Create();
  MockVideoDecoderForMixer();
  ~MockVideoDecoderForMixer() override;

  // VideoDecoderForMixer implementation:
  void Initialize() override;
  bool Start(int64_t start_pts, bool need_avsync) override;
  void Stop() override;
  bool Pause() override;
  bool Resume() override;
  bool GetCurrentPts(int64_t* timestamp, int64_t* pts) const override;
  bool SetPlaybackRate(float rate) override;
  bool SetPts(int64_t timestamp, int64_t pts) override;
  int64_t GetDroppedFrames() override;
  int64_t GetRepeatedFrames() override;
  int64_t GetOutputRefreshRate() override;
  int64_t GetCurrentContentRefreshRate() override;

  // VideoDecoder implementation:
  void SetDelegate(MediaPipelineBackend::Decoder::Delegate* delegate) override;
  BufferStatus PushBuffer(CastDecoderBuffer* buffer) override;
  void GetStatistics(Statistics* statistics) override;
  bool SetConfig(const VideoConfig& config) override;

  // VideoDecoderForTest implementation:
  int64_t GetExpectedDroppedFrames() override;
  int64_t GetExpectedRepeatedFrames() override;
  int64_t GetNumberOfFramesTolerated() override;
  int64_t GetNumberOfHardCorrectionsTolerated() override;
  int64_t GetNumberOfSoftCorrectionsTolerated() override;
  int64_t GetAvSyncDriftTolerated() override;

 private:
  void UpkeepVsync();
  int64_t GetVsyncPeriodUs();

  base::RepeatingTimer vsync_timer_;

  double linear_clock_rate_ =
      (kClockRateNumerator * 1.0) / (kClockRateDenominator * 1.0);
  int64_t content_fps_ = kContentFps;

  int64_t display_refresh_rate_ = 60;

  double linear_pts_rate_ = 1.0;
  int64_t current_video_pts_ = INT64_MIN;
  int64_t last_displayed_frame_pts_ = 0.0;
  int64_t start_pts_ = 0;
  int64_t dropped_frames_ = 0;
  int64_t repeated_frames_ = 0;
};

typedef MockVideoDecoderForMixer<1, 1, 60> NormalVideoDecoder;
typedef MockVideoDecoderForMixer<1, 1, 30> NormalVideoDecoder30;
typedef MockVideoDecoderForMixer<1, 1, 24> NormalVideoDecoder24;
typedef MockVideoDecoderForMixer<2, 1, 60> LinearDoubleSpeedVideoDecoder;
typedef MockVideoDecoderForMixer<1, 2, 60> LinearHalfSpeedVideoDecoder;
typedef MockVideoDecoderForMixer<14, 10, 60> Linear140PercentSpeedVideoDecoder;
typedef MockVideoDecoderForMixer<13, 10, 60> Linear130PercentSpeedVideoDecoder;
typedef MockVideoDecoderForMixer<12, 10, 60> Linear120PercentSpeedVideoDecoder;
typedef MockVideoDecoderForMixer<11, 10, 60> Linear110PercentSpeedVideoDecoder;
typedef MockVideoDecoderForMixer<9, 10, 60> Linear90PercentSpeedVideoDecoder;
typedef MockVideoDecoderForMixer<8, 10, 60> Linear80PercentSpeedVideoDecoder;
typedef MockVideoDecoderForMixer<7, 10, 60> Linear70PercentSpeedVideoDecoder;
typedef MockVideoDecoderForMixer<6, 10, 60> Linear60PercentSpeedVideoDecoder;

}  // namespace media
}  // namespace chromecast

#endif  // CHROMECAST_MEDIA_CMA_BACKEND_MOCK_VIDEO_DECODER_FOR_MIXER_H_
