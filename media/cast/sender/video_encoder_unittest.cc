// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "media/cast/sender/video_encoder.h"

#include <stdint.h>

#include <memory>
#include <utility>
#include <vector>

#include "base/bind.h"
#include "base/macros.h"
#include "base/memory/ref_counted.h"
#include "build/build_config.h"
#include "media/base/fake_single_thread_task_runner.h"
#include "media/base/video_frame.h"
#include "media/cast/cast_environment.h"
#include "media/cast/common/rtp_time.h"
#include "media/cast/sender/fake_video_encode_accelerator_factory.h"
#include "media/cast/sender/video_frame_factory.h"
#include "media/cast/test/utility/default_config.h"
#include "media/cast/test/utility/video_utility.h"
#include "testing/gtest/include/gtest/gtest.h"

#if defined(OS_MACOSX)
#include "media/cast/sender/h264_vt_encoder.h"
#endif

namespace media {
namespace cast {

class VideoEncoderTest
    : public ::testing::TestWithParam<std::pair<Codec, bool>> {
 protected:
  VideoEncoderTest()
      : task_runner_(new FakeSingleThreadTaskRunner(&testing_clock_)),
        cast_environment_(new CastEnvironment(&testing_clock_,
                                              task_runner_,
                                              task_runner_,
                                              task_runner_)),
        video_config_(GetDefaultVideoSenderConfig()),
        operational_status_(STATUS_UNINITIALIZED),
        count_frames_delivered_(0) {
    testing_clock_.Advance(base::TimeTicks::Now() - base::TimeTicks());
    first_frame_time_ = testing_clock_.NowTicks();
  }

  ~VideoEncoderTest() override = default;

  void SetUp() final {
    video_config_.codec = GetParam().first;
    video_config_.use_external_encoder = GetParam().second;

    if (video_config_.use_external_encoder)
      vea_factory_.reset(new FakeVideoEncodeAcceleratorFactory(task_runner_));
  }

  void TearDown() final {
    video_encoder_.reset();
    RunTasksAndAdvanceClock();
  }

  void CreateEncoder() {
    ASSERT_EQ(STATUS_UNINITIALIZED, operational_status_);
    video_config_.video_codec_params.max_number_of_video_buffers_used = 1;
    video_encoder_ = VideoEncoder::Create(
        cast_environment_, video_config_,
        base::Bind(&VideoEncoderTest::OnOperationalStatusChange,
                   base::Unretained(this)),
        base::Bind(
            &FakeVideoEncodeAcceleratorFactory::CreateVideoEncodeAccelerator,
            base::Unretained(vea_factory_.get())),
        base::Bind(&FakeVideoEncodeAcceleratorFactory::CreateSharedMemory,
                   base::Unretained(vea_factory_.get())));
    RunTasksAndAdvanceClock();
    if (is_encoder_present())
      ASSERT_EQ(STATUS_INITIALIZED, operational_status_);
  }

  bool is_encoder_present() const {
    return !!video_encoder_;
  }

  bool is_testing_software_vp8_encoder() const {
    return video_config_.codec == CODEC_VIDEO_VP8 &&
        !video_config_.use_external_encoder;
  }

  bool is_testing_video_toolbox_encoder() const {
    return
#if defined(OS_MACOSX)
        (!video_config_.use_external_encoder &&
         H264VideoToolboxEncoder::IsSupported(video_config_)) ||
#endif
        false;
  }

  bool is_testing_platform_encoder() const {
    return video_config_.use_external_encoder ||
        is_testing_video_toolbox_encoder();
  }

  bool encoder_has_resize_delay() const {
    return is_testing_platform_encoder() && !is_testing_video_toolbox_encoder();
  }

  VideoEncoder* video_encoder() const {
    return video_encoder_.get();
  }

  void DestroyEncoder() {
    video_encoder_.reset();
  }

  base::TimeTicks Now() { return testing_clock_.NowTicks(); }

  void RunTasksAndAdvanceClock() {
    DCHECK_GT(video_config_.max_frame_rate, 0);
    const base::TimeDelta frame_duration = base::TimeDelta::FromMicroseconds(
        1000000.0 / video_config_.max_frame_rate);
#if defined(OS_MACOSX)
    if (is_testing_video_toolbox_encoder()) {
      // The H264VideoToolboxEncoder (on MAC_OSX and IOS) is not a faked
      // implementation in these tests, and performs its encoding asynchronously
      // on an unknown set of threads.  Therefore, sleep the current thread for
      // the real amount of time to avoid excessively spinning the CPU while
      // waiting for something to happen.
      base::PlatformThread::Sleep(frame_duration);
    }
#endif
    task_runner_->RunTasks();
    testing_clock_.Advance(frame_duration);
  }

  int count_frames_delivered() const {
    return count_frames_delivered_;
  }

  void WaitForAllFramesToBeDelivered(int total_expected) {
    video_encoder_->EmitFrames();
    while (count_frames_delivered_ < total_expected)
      RunTasksAndAdvanceClock();
  }

  // Creates a new VideoFrame of the given |size|, filled with a test pattern.
  // When available, it attempts to use the VideoFrameFactory provided by the
  // encoder.
  scoped_refptr<media::VideoFrame> CreateTestVideoFrame(const gfx::Size& size) {
    const base::TimeDelta timestamp =
        testing_clock_.NowTicks() - first_frame_time_;
    scoped_refptr<media::VideoFrame> frame;
    if (video_frame_factory_)
      frame = video_frame_factory_->MaybeCreateFrame(size, timestamp);
    if (!frame) {
      frame = media::VideoFrame::CreateFrame(PIXEL_FORMAT_I420, size,
                                             gfx::Rect(size), size, timestamp);
    }
    PopulateVideoFrame(frame.get(), 123);
    return frame;
  }

  // Requests encoding the |video_frame| and has the resulting frame delivered
  // via a callback that checks for expected results.  Returns false if the
  // encoder rejected the request.
  bool EncodeAndCheckDelivery(
      const scoped_refptr<media::VideoFrame>& video_frame,
      FrameId frame_id,
      FrameId reference_frame_id) {
    return video_encoder_->EncodeVideoFrame(
        video_frame, Now(),
        base::Bind(&VideoEncoderTest::DeliverEncodedVideoFrame,
                   base::Unretained(this), frame_id, reference_frame_id,
                   RtpTimeTicks::FromTimeDelta(video_frame->timestamp(),
                                               kVideoFrequency),
                   Now()));
  }

  // If the implementation of |video_encoder_| is ExternalVideoEncoder, check
  // that the VEA factory has responded (by running the callbacks) a specific
  // number of times.  Otherwise, check that the VEA factory is inactive.
  void ExpectVEAResponsesForExternalVideoEncoder(
      int vea_response_count,
      int shm_response_count) const {
    if (!vea_factory_)
      return;
    EXPECT_EQ(vea_response_count, vea_factory_->vea_response_count());
    EXPECT_EQ(shm_response_count, vea_factory_->shm_response_count());
  }

  void SetVEAFactoryAutoRespond(bool auto_respond) {
    if (vea_factory_)
      vea_factory_->SetAutoRespond(auto_respond);
  }

 private:
  void OnOperationalStatusChange(OperationalStatus status) {
    DVLOG(1) << "OnOperationalStatusChange: from " << operational_status_
             << " to " << status;
    operational_status_ = status;

    EXPECT_TRUE(operational_status_ == STATUS_CODEC_REINIT_PENDING ||
                operational_status_ == STATUS_INITIALIZED);

    // Create the VideoFrameFactory the first time status changes to
    // STATUS_INITIALIZED.
    if (operational_status_ == STATUS_INITIALIZED && !video_frame_factory_)
      video_frame_factory_ = video_encoder_->CreateVideoFrameFactory();
  }

  // Checks that |encoded_frame| matches expected values.  This is the method
  // bound in the callback returned from EncodeAndCheckDelivery().
  void DeliverEncodedVideoFrame(
      FrameId expected_frame_id,
      FrameId expected_last_referenced_frame_id,
      RtpTimeTicks expected_rtp_timestamp,
      const base::TimeTicks& expected_reference_time,
      std::unique_ptr<SenderEncodedFrame> encoded_frame) {
    EXPECT_TRUE(cast_environment_->CurrentlyOn(CastEnvironment::MAIN));

    EXPECT_EQ(expected_frame_id, encoded_frame->frame_id);
    EXPECT_EQ(expected_rtp_timestamp, encoded_frame->rtp_timestamp);
    EXPECT_EQ(expected_reference_time, encoded_frame->reference_time);

    // The platform encoders are "black boxes" and may choose to vend key frames
    // and/or empty data at any time.  The software encoders, however, should
    // strictly adhere to expected behavior.
    if (is_testing_platform_encoder()) {
      const bool expected_key_frame =
          expected_frame_id == expected_last_referenced_frame_id;
      const bool have_key_frame =
          encoded_frame->dependency == EncodedFrame::KEY;
      EXPECT_EQ(have_key_frame,
                encoded_frame->frame_id == encoded_frame->referenced_frame_id);
      LOG_IF(WARNING, expected_key_frame != have_key_frame)
          << "Platform encoder chose to emit a "
          << (have_key_frame ? "key" : "delta")
          << " frame instead of the expected kind @ frame_id="
          << encoded_frame->frame_id;
      LOG_IF(WARNING, encoded_frame->data.empty())
          << "Platform encoder returned an empty frame @ frame_id="
          << encoded_frame->frame_id;
    } else {
      if (expected_frame_id != expected_last_referenced_frame_id) {
        EXPECT_EQ(EncodedFrame::DEPENDENT, encoded_frame->dependency);
      } else if (video_config_.video_codec_params
                     .max_number_of_video_buffers_used == 1) {
        EXPECT_EQ(EncodedFrame::KEY, encoded_frame->dependency);
      }
      EXPECT_EQ(expected_last_referenced_frame_id,
                encoded_frame->referenced_frame_id);
      EXPECT_FALSE(encoded_frame->data.empty());
      ASSERT_TRUE(std::isfinite(encoded_frame->encoder_utilization));
      EXPECT_LE(0.0, encoded_frame->encoder_utilization);
      ASSERT_TRUE(std::isfinite(encoded_frame->lossy_utilization));
      EXPECT_LE(0.0, encoded_frame->lossy_utilization);
    }

    ++count_frames_delivered_;
  }

  base::SimpleTestTickClock testing_clock_;
  const scoped_refptr<FakeSingleThreadTaskRunner> task_runner_;
  const scoped_refptr<CastEnvironment> cast_environment_;
  FrameSenderConfig video_config_;
  std::unique_ptr<FakeVideoEncodeAcceleratorFactory> vea_factory_;
  base::TimeTicks first_frame_time_;
  OperationalStatus operational_status_;
  std::unique_ptr<VideoEncoder> video_encoder_;
  std::unique_ptr<VideoFrameFactory> video_frame_factory_;

  int count_frames_delivered_;

  DISALLOW_COPY_AND_ASSIGN(VideoEncoderTest);
};

// A simple test to encode three frames of video, expecting to see one key frame
// followed by two delta frames.
TEST_P(VideoEncoderTest, GeneratesKeyFrameThenOnlyDeltaFrames) {
  CreateEncoder();
  SetVEAFactoryAutoRespond(true);

  EXPECT_EQ(0, count_frames_delivered());
  ExpectVEAResponsesForExternalVideoEncoder(0, 0);

  FrameId frame_id = FrameId::first();
  FrameId reference_frame_id = FrameId::first();
  const gfx::Size frame_size(1280, 720);

  // Some encoders drop one or more frames initially while the encoder
  // initializes. Then, for all encoders, expect one key frame is delivered.
  bool accepted_first_frame = false;
  do {
    accepted_first_frame = EncodeAndCheckDelivery(
        CreateTestVideoFrame(frame_size), frame_id, reference_frame_id);
    if (!encoder_has_resize_delay())
      EXPECT_TRUE(accepted_first_frame);
    RunTasksAndAdvanceClock();
  } while (!accepted_first_frame);
  ExpectVEAResponsesForExternalVideoEncoder(1, 3);

  // Expect the remaining frames are encoded as delta frames.
  for (++frame_id; frame_id < FrameId::first() + 3;
       ++frame_id, ++reference_frame_id) {
    EXPECT_TRUE(EncodeAndCheckDelivery(CreateTestVideoFrame(frame_size),
                                       frame_id,
                                       reference_frame_id));
    RunTasksAndAdvanceClock();
  }

  WaitForAllFramesToBeDelivered(3);
  ExpectVEAResponsesForExternalVideoEncoder(1, 3);
}

// Tests that the encoder continues to output EncodedFrames as the frame size
// changes.  See media/cast/receiver/video_decoder_unittest.cc for a complete
// encode/decode cycle of varied frame sizes that actually checks the frame
// content.
TEST_P(VideoEncoderTest, EncodesVariedFrameSizes) {
  CreateEncoder();
  SetVEAFactoryAutoRespond(true);

  EXPECT_EQ(0, count_frames_delivered());
  ExpectVEAResponsesForExternalVideoEncoder(0, 0);

  std::vector<gfx::Size> frame_sizes;
  frame_sizes.push_back(gfx::Size(128, 72));
  frame_sizes.push_back(gfx::Size(64, 36));    // Shrink both dimensions.
  frame_sizes.push_back(gfx::Size(30, 20));    // Shrink both dimensions again.
  frame_sizes.push_back(gfx::Size(20, 30));    // Same area.
  frame_sizes.push_back(gfx::Size(60, 40));    // Grow both dimensions.
  frame_sizes.push_back(gfx::Size(58, 40));    // Shrink only one dimension.
  frame_sizes.push_back(gfx::Size(58, 38));    // Shrink the other dimension.
  frame_sizes.push_back(gfx::Size(32, 18));    // Shrink both dimensions again.
  frame_sizes.push_back(gfx::Size(34, 18));    // Grow only one dimension.
  frame_sizes.push_back(gfx::Size(34, 20));    // Grow the other dimension.
  frame_sizes.push_back(gfx::Size(192, 108));  // Grow both dimensions again.

  FrameId frame_id = FrameId::first();

  // Encode one frame at each size. For encoders with a resize delay, except no
  // frames to be delivered since each frame size change will sprun
  // re-initialization of the underlying encoder. Otherwise expect all key
  // frames to come out.
  for (const auto& frame_size : frame_sizes) {
    EXPECT_EQ(!encoder_has_resize_delay(),
              EncodeAndCheckDelivery(CreateTestVideoFrame(frame_size), frame_id,
                                     frame_id));
    RunTasksAndAdvanceClock();
    if (!encoder_has_resize_delay())
      ++frame_id;
  }

  // Encode three frames at each size. For encoders with a resize delay, expect
  // the first one or more frames are dropped while the encoder re-inits. Then,
  // for all encoders, expect one key frame followed by all delta frames.
  for (const auto& frame_size : frame_sizes) {
    bool accepted_first_frame = false;
    do {
      accepted_first_frame = EncodeAndCheckDelivery(
          CreateTestVideoFrame(frame_size), frame_id, frame_id);
      if (!encoder_has_resize_delay())
        EXPECT_TRUE(accepted_first_frame);
      RunTasksAndAdvanceClock();
    } while (!accepted_first_frame);
    ++frame_id;
    for (int i = 1; i < 3; ++i, ++frame_id) {
      EXPECT_TRUE(EncodeAndCheckDelivery(CreateTestVideoFrame(frame_size),
                                         frame_id,
                                         frame_id - 1));
      RunTasksAndAdvanceClock();
    }
  }

  WaitForAllFramesToBeDelivered(3 * frame_sizes.size());
  ExpectVEAResponsesForExternalVideoEncoder(
      2 * frame_sizes.size(), 6 * frame_sizes.size());
}

// Verify that everything goes well even if ExternalVideoEncoder is destroyed
// before it has a chance to receive the VEA creation callback.  For all other
// encoders, this tests that the encoder can be safely destroyed before the task
// is run that delivers the first EncodedFrame.
TEST_P(VideoEncoderTest, CanBeDestroyedBeforeVEAIsCreated) {
  CreateEncoder();

  // Send a frame to spawn creation of the ExternalVideoEncoder instance.
  EncodeAndCheckDelivery(CreateTestVideoFrame(gfx::Size(128, 72)),
                         FrameId::first(), FrameId::first());

  // Destroy the encoder, and confirm the VEA Factory did not respond yet.
  DestroyEncoder();
  ExpectVEAResponsesForExternalVideoEncoder(0, 0);

  // Allow the VEA Factory to respond by running the creation callback.  When
  // the task runs, it will be a no-op since the weak pointers to the
  // ExternalVideoEncoder were invalidated.
  SetVEAFactoryAutoRespond(true);
  RunTasksAndAdvanceClock();
  ExpectVEAResponsesForExternalVideoEncoder(1, 0);
}

namespace {
std::vector<std::pair<Codec, bool>> DetermineEncodersToTest() {
  std::vector<std::pair<Codec, bool>> values;
  // Fake encoder.
  values.push_back(std::make_pair(CODEC_VIDEO_FAKE, false));
  // Software VP8 encoder.
  values.push_back(std::make_pair(CODEC_VIDEO_VP8, false));
  // Hardware-accelerated encoder (faked).
  values.push_back(std::make_pair(CODEC_VIDEO_VP8, true));
#if defined(OS_MACOSX)
  // VideoToolbox encoder (when VideoToolbox is present).
  FrameSenderConfig video_config = GetDefaultVideoSenderConfig();
  video_config.use_external_encoder = false;
  video_config.codec = CODEC_VIDEO_H264;
  if (H264VideoToolboxEncoder::IsSupported(video_config))
    values.push_back(std::make_pair(CODEC_VIDEO_H264, false));
#endif
  return values;
}
}  // namespace

INSTANTIATE_TEST_CASE_P(
    , VideoEncoderTest, ::testing::ValuesIn(DetermineEncodersToTest()));

}  // namespace cast
}  // namespace media
