// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chromecast/media/cma/backend/av_sync.h"

#include <cmath>
#include <utility>

#include "base/message_loop/message_loop.h"
#include "base/test/scoped_mock_time_message_loop_task_runner.h"
#include "base/test/test_mock_time_task_runner.h"
#include "base/time/time.h"
#include "chromecast/base/task_runner_impl.h"
#include "chromecast/media/cma/backend/mock_audio_decoder_for_mixer.h"
#include "chromecast/media/cma/backend/mock_media_pipeline_backend_for_mixer.h"
#include "chromecast/media/cma/backend/mock_video_decoder_for_mixer.h"
#include "chromecast/media/cma/backend/video/av_sync_video.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace chromecast {
namespace media {

class AvSyncTest : public testing::Test, public AvSyncVideo::Delegate {
 public:
  AvSyncTest()
      : mock_task_runner_(new base::TestMockTimeTaskRunner()),
        runner_(mock_task_runner_) {}

  void SetupTest(base::OnceCallback<std::unique_ptr<VideoDecoderForTest>()>
                     video_decoder_factory) {
    base::TestMockTimeTaskRunner::ScopedContext scoped_context(
        mock_task_runner_.get());

    MediaPipelineDeviceParams params(&runner_, AudioContentType::kMedia,
                                     "test");

    backend_ = std::make_unique<MockMediaPipelineBackendForMixer>(params);

    backend_->SetVideoDecoder(std::move(video_decoder_factory).Run());
    backend_->SetAudioDecoder(MockAudioDecoderForMixer::Create(backend_.get()));

    backend_->Initialize();
    backend_->Start(0);
  }

  void NotifyAvSyncPlaybackStatistics(
      int64_t unexpected_dropped_frames,
      int64_t unexpected_repeated_frames,
      double average_av_sync_difference_us,
      int64_t current_apts,
      int64_t current_vpts,
      int64_t number_of_soft_corrections,
      int64_t number_of_hard_corrections) override {
    VideoDecoderForTest* video_decoder = static_cast<VideoDecoderForTest*>(
        static_cast<MockMediaPipelineBackendForMixer*>(backend_.get())
            ->video_decoder());

    // TODO(almasrymina): ignore the first few seconds of playback, and don't
    // assert anything about them.
    //
    // The reason for this is that I'm observing with broken video decoders
    // that we'll start playback in sync, but the video decoder is broken,
    // creating a gap, then the AV sync logic kicks in, then it slowly brings
    // the media back in line.
    //
    // Need to find a better solution to this.
    if (current_vpts < 10000000)
      return;

    // Assert the data here is within what's expected by this video decoder.
    EXPECT_LT(std::abs(average_av_sync_difference_us),
              video_decoder->GetAvSyncDriftTolerated());

    int64_t expected_dropped_frames = video_decoder->GetExpectedDroppedFrames();
    int64_t expected_repeated_frames =
        video_decoder->GetExpectedRepeatedFrames();

    EXPECT_LE(std::abs(unexpected_dropped_frames - expected_dropped_frames),
              video_decoder->GetNumberOfFramesTolerated());

    EXPECT_LE(std::abs(unexpected_repeated_frames - expected_repeated_frames),
              video_decoder->GetNumberOfFramesTolerated());

    EXPECT_LE(number_of_hard_corrections,
              video_decoder->GetNumberOfHardCorrectionsTolerated());

    EXPECT_LE(number_of_soft_corrections,
              video_decoder->GetNumberOfSoftCorrectionsTolerated());
    // TODO(almasrymina): b/73746352 add more tests. For example, probably we
    // should assert the total number of dropped/repeated frames for the entire
    // playback is within reason.
    //
    // Assert only 1 soft correction and 1 in sync correction is every
    // executed.
  }

 protected:
  base::MessageLoop message_loop_;
  std::unique_ptr<MockMediaPipelineBackendForMixer> backend_;
  scoped_refptr<base::TestMockTimeTaskRunner> mock_task_runner_;
  TaskRunnerImpl runner_;

 private:
  DISALLOW_COPY_AND_ASSIGN(AvSyncTest);
};

TEST_F(AvSyncTest, Baseline60) {
  SetupTest(base::BindOnce(&NormalVideoDecoder::Create));
  mock_task_runner_->FastForwardBy(base::TimeDelta::FromSeconds(600));
}

TEST_F(AvSyncTest, Baseline30) {
  SetupTest(base::BindOnce(&NormalVideoDecoder30::Create));
  mock_task_runner_->FastForwardBy(base::TimeDelta::FromSeconds(600));
}

TEST_F(AvSyncTest, Baseline24) {
  SetupTest(base::BindOnce(&NormalVideoDecoder24::Create));
  mock_task_runner_->FastForwardBy(base::TimeDelta::FromSeconds(600));
}

TEST_F(AvSyncTest, LinearDoubleSpeedVideoDecoder) {
  SetupTest(base::BindOnce(&LinearDoubleSpeedVideoDecoder::Create));
  mock_task_runner_->FastForwardBy(base::TimeDelta::FromSeconds(600));
}

TEST_F(AvSyncTest, LinearHalfSpeedVideoDecoder) {
  SetupTest(base::BindOnce(&LinearHalfSpeedVideoDecoder::Create));
  mock_task_runner_->FastForwardBy(base::TimeDelta::FromSeconds(600));
}

TEST_F(AvSyncTest, Linear130PercentSpeedVideoDecoder) {
  SetupTest(base::BindOnce(&Linear130PercentSpeedVideoDecoder::Create));
  mock_task_runner_->FastForwardBy(base::TimeDelta::FromSeconds(600));
}
TEST_F(AvSyncTest, Linear120PercentSpeedVideoDecoder) {
  SetupTest(base::BindOnce(&Linear120PercentSpeedVideoDecoder::Create));
  mock_task_runner_->FastForwardBy(base::TimeDelta::FromSeconds(600));
}
TEST_F(AvSyncTest, Linear110PercentSpeedVideoDecoder) {
  SetupTest(base::BindOnce(&Linear110PercentSpeedVideoDecoder::Create));
  mock_task_runner_->FastForwardBy(base::TimeDelta::FromSeconds(600));
}
TEST_F(AvSyncTest, Linear90PercentSpeedVideoDecoder) {
  SetupTest(base::BindOnce(&Linear90PercentSpeedVideoDecoder::Create));
  mock_task_runner_->FastForwardBy(base::TimeDelta::FromSeconds(600));
}
TEST_F(AvSyncTest, Linear80PercentSpeedVideoDecoder) {
  SetupTest(base::BindOnce(&Linear80PercentSpeedVideoDecoder::Create));
  mock_task_runner_->FastForwardBy(base::TimeDelta::FromSeconds(600));
}
TEST_F(AvSyncTest, Linear70PercentSpeedVideoDecoder) {
  SetupTest(base::BindOnce(&Linear70PercentSpeedVideoDecoder::Create));
  mock_task_runner_->FastForwardBy(base::TimeDelta::FromSeconds(600));
}
TEST_F(AvSyncTest, Linear60PercentSpeedVideoDecoder) {
  SetupTest(base::BindOnce(&Linear60PercentSpeedVideoDecoder::Create));
  mock_task_runner_->FastForwardBy(base::TimeDelta::FromSeconds(600));
}

}  // namespace media
}  // namespace chromecast
