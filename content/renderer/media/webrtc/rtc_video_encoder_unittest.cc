// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <stdint.h>

#include "base/bind.h"
#include "base/single_thread_task_runner.h"
#include "base/synchronization/waitable_event.h"
#include "base/threading/thread.h"
#include "content/renderer/media/webrtc/rtc_video_encoder.h"
#include "media/video/mock_gpu_video_accelerator_factories.h"
#include "media/video/mock_video_encode_accelerator.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "third_party/libyuv/include/libyuv/planar_functions.h"
#include "third_party/webrtc/api/video/i420_buffer.h"
#include "third_party/webrtc/api/video_codecs/video_encoder.h"
#include "third_party/webrtc/modules/video_coding/include/video_codec_interface.h"

using ::testing::_;
using ::testing::AtLeast;
using ::testing::Invoke;
using ::testing::Return;
using ::testing::SaveArg;
using ::testing::Values;
using ::testing::WithArgs;

namespace content {

namespace {

const int kInputFrameFillY = 12;
const int kInputFrameFillU = 23;
const int kInputFrameFillV = 34;
const unsigned short kInputFrameHeight = 234;
const unsigned short kInputFrameWidth = 345;
const unsigned short kStartBitrate = 100;

class EncodedImageCallbackWrapper : public webrtc::EncodedImageCallback {
 public:
  using EncodedCallback =
      base::Callback<void(const webrtc::EncodedImage& encoded_image,
                          const webrtc::CodecSpecificInfo* codec_specific_info,
                          const webrtc::RTPFragmentationHeader* fragmentation)>;

  EncodedImageCallbackWrapper(const EncodedCallback& encoded_callback)
      : encoded_callback_(encoded_callback) {}

  Result OnEncodedImage(
      const webrtc::EncodedImage& encoded_image,
      const webrtc::CodecSpecificInfo* codec_specific_info,
      const webrtc::RTPFragmentationHeader* fragmentation) override {
    encoded_callback_.Run(encoded_image, codec_specific_info, fragmentation);
    return Result(Result::OK);
  };

 private:
  EncodedCallback encoded_callback_;
};
}  // anonymous namespace

class RTCVideoEncoderTest
    : public ::testing::TestWithParam<webrtc::VideoCodecType> {
 public:
  RTCVideoEncoderTest()
      : encoder_thread_("vea_thread"),
        mock_gpu_factories_(
            new media::MockGpuVideoAcceleratorFactories(nullptr)),
        idle_waiter_(base::WaitableEvent::ResetPolicy::AUTOMATIC,
                     base::WaitableEvent::InitialState::NOT_SIGNALED) {}

  media::MockVideoEncodeAccelerator* ExpectCreateInitAndDestroyVEA() {
    // The VEA will be owned by the RTCVideoEncoder once
    // factory.CreateVideoEncodeAccelerator() is called.
    media::MockVideoEncodeAccelerator* mock_vea =
        new media::MockVideoEncodeAccelerator();

    EXPECT_CALL(*mock_gpu_factories_.get(), DoCreateVideoEncodeAccelerator())
        .WillRepeatedly(Return(mock_vea));
    EXPECT_CALL(*mock_vea, Initialize(_, _, _, _, _))
        .WillOnce(Invoke(this, &RTCVideoEncoderTest::Initialize));
    EXPECT_CALL(*mock_vea, UseOutputBitstreamBuffer(_)).Times(AtLeast(3));
    EXPECT_CALL(*mock_vea, Destroy()).Times(1);
    return mock_vea;
  }

  void SetUp() override {
    DVLOG(3) << __func__;
    ASSERT_TRUE(encoder_thread_.Start());

    EXPECT_CALL(*mock_gpu_factories_.get(), GetTaskRunner())
        .WillRepeatedly(Return(encoder_thread_.task_runner()));
    mock_vea_ = ExpectCreateInitAndDestroyVEA();
  }

  void TearDown() override {
    DVLOG(3) << __func__;
    EXPECT_TRUE(encoder_thread_.IsRunning());
    RunUntilIdle();
    rtc_encoder_->Release();
    RunUntilIdle();
    encoder_thread_.Stop();
  }

  void RunUntilIdle() {
    DVLOG(3) << __func__;
    encoder_thread_.task_runner()->PostTask(
        FROM_HERE, base::BindOnce(&base::WaitableEvent::Signal,
                                  base::Unretained(&idle_waiter_)));
    idle_waiter_.Wait();
  }

  void CreateEncoder(webrtc::VideoCodecType codec_type) {
    DVLOG(3) << __func__;
    media::VideoCodecProfile media_profile;
    switch (codec_type) {
      case webrtc::kVideoCodecVP8:
        media_profile = media::VP8PROFILE_ANY;
        break;
      case webrtc::kVideoCodecH264:
        media_profile = media::H264PROFILE_BASELINE;
        break;
      default:
        ADD_FAILURE() << "Unexpected codec type: " << codec_type;
        media_profile = media::VIDEO_CODEC_PROFILE_UNKNOWN;
    }
    rtc_encoder_ = std::make_unique<RTCVideoEncoder>(media_profile,
                                                     mock_gpu_factories_.get());
  }

  // media::VideoEncodeAccelerator implementation.
  bool Initialize(media::VideoPixelFormat input_format,
                  const gfx::Size& input_visible_size,
                  media::VideoCodecProfile output_profile,
                  uint32_t initial_bitrate,
                  media::VideoEncodeAccelerator::Client* client) {
    DVLOG(3) << __func__;
    client_ = client;
    client_->RequireBitstreamBuffers(0, input_visible_size,
                                     input_visible_size.GetArea());
    return true;
  }

  void RegisterEncodeCompleteCallback(
      const EncodedImageCallbackWrapper::EncodedCallback& callback) {
    callback_wrapper_.reset(new EncodedImageCallbackWrapper(callback));
    rtc_encoder_->RegisterEncodeCompleteCallback(callback_wrapper_.get());
  }

  webrtc::VideoCodec GetDefaultCodec() {
    webrtc::VideoCodec codec = {};
    memset(&codec, 0, sizeof(codec));
    codec.width = kInputFrameWidth;
    codec.height = kInputFrameHeight;
    codec.codecType = webrtc::kVideoCodecVP8;
    codec.startBitrate = kStartBitrate;
    return codec;
  }

  void FillFrameBuffer(rtc::scoped_refptr<webrtc::I420Buffer> frame) {
    CHECK(libyuv::I420Rect(frame->MutableDataY(), frame->StrideY(),
                           frame->MutableDataU(), frame->StrideU(),
                           frame->MutableDataV(), frame->StrideV(), 0, 0,
                           frame->width(), frame->height(), kInputFrameFillY,
                           kInputFrameFillU, kInputFrameFillV) == 0);
  }

  void VerifyEncodedFrame(const scoped_refptr<media::VideoFrame>& frame,
                          bool force_keyframe) {
    DVLOG(3) << __func__;
    EXPECT_EQ(kInputFrameWidth, frame->visible_rect().width());
    EXPECT_EQ(kInputFrameHeight, frame->visible_rect().height());
    EXPECT_EQ(kInputFrameFillY,
              frame->visible_data(media::VideoFrame::kYPlane)[0]);
    EXPECT_EQ(kInputFrameFillU,
              frame->visible_data(media::VideoFrame::kUPlane)[0]);
    EXPECT_EQ(kInputFrameFillV,
              frame->visible_data(media::VideoFrame::kVPlane)[0]);
  }

  void ReturnFrameWithTimeStamp(const scoped_refptr<media::VideoFrame>& frame,
                                bool force_keyframe) {
    client_->BitstreamBufferReady(0, 0, force_keyframe, frame->timestamp());
  }

  void VerifyTimestamp(uint32_t rtp_timestamp,
                       int64_t capture_time_ms,
                       const webrtc::EncodedImage& encoded_image,
                       const webrtc::CodecSpecificInfo* codec_specific_info,
                       const webrtc::RTPFragmentationHeader* fragmentation) {
    DVLOG(3) << __func__;
    EXPECT_EQ(rtp_timestamp, encoded_image._timeStamp);
    EXPECT_EQ(capture_time_ms, encoded_image.capture_time_ms_);
  }

 protected:
  media::MockVideoEncodeAccelerator* mock_vea_;
  std::unique_ptr<RTCVideoEncoder> rtc_encoder_;
  media::VideoEncodeAccelerator::Client* client_;
  base::Thread encoder_thread_;

 private:
  std::unique_ptr<media::MockGpuVideoAcceleratorFactories> mock_gpu_factories_;
  std::unique_ptr<EncodedImageCallbackWrapper> callback_wrapper_;
  base::WaitableEvent idle_waiter_;
};

TEST_P(RTCVideoEncoderTest, CreateAndInitSucceeds) {
  const webrtc::VideoCodecType codec_type = GetParam();
  CreateEncoder(codec_type);
  webrtc::VideoCodec codec = GetDefaultCodec();
  codec.codecType = codec_type;
  EXPECT_EQ(WEBRTC_VIDEO_CODEC_OK, rtc_encoder_->InitEncode(&codec, 1, 12345));
}

TEST_P(RTCVideoEncoderTest, RepeatedInitSucceeds) {
  const webrtc::VideoCodecType codec_type = GetParam();
  CreateEncoder(codec_type);
  webrtc::VideoCodec codec = GetDefaultCodec();
  codec.codecType = codec_type;
  EXPECT_EQ(WEBRTC_VIDEO_CODEC_OK, rtc_encoder_->InitEncode(&codec, 1, 12345));

  ExpectCreateInitAndDestroyVEA();
  EXPECT_EQ(WEBRTC_VIDEO_CODEC_OK, rtc_encoder_->InitEncode(&codec, 1, 12345));
}

INSTANTIATE_TEST_CASE_P(CodecProfiles,
                        RTCVideoEncoderTest,
                        Values(webrtc::kVideoCodecVP8,
                               webrtc::kVideoCodecH264));

// Checks that WEBRTC_VIDEO_CODEC_FALLBACK_SOFTWARE is returned when there is
// platform error.
TEST_F(RTCVideoEncoderTest, SoftwareFallbackAfterError) {
  const webrtc::VideoCodecType codec_type = webrtc::kVideoCodecVP8;
  CreateEncoder(codec_type);
  webrtc::VideoCodec codec = GetDefaultCodec();
  codec.codecType = codec_type;
  EXPECT_EQ(WEBRTC_VIDEO_CODEC_OK, rtc_encoder_->InitEncode(&codec, 1, 12345));

  EXPECT_CALL(*mock_vea_, Encode(_, _))
      .WillOnce(Invoke([this](const scoped_refptr<media::VideoFrame>&, bool) {
        encoder_thread_.task_runner()->PostTask(
            FROM_HERE,
            base::BindOnce(
                &media::VideoEncodeAccelerator::Client::NotifyError,
                base::Unretained(client_),
                media::VideoEncodeAccelerator::kPlatformFailureError));
      }));

  const rtc::scoped_refptr<webrtc::I420Buffer> buffer =
      webrtc::I420Buffer::Create(kInputFrameWidth, kInputFrameHeight);
  FillFrameBuffer(buffer);
  std::vector<webrtc::FrameType> frame_types;
  EXPECT_EQ(WEBRTC_VIDEO_CODEC_OK,
            rtc_encoder_->Encode(
                webrtc::VideoFrame(buffer, 0, 0, webrtc::kVideoRotation_0),
                nullptr, &frame_types));
  RunUntilIdle();

  // Expect the next frame to return SW fallback.
  EXPECT_EQ(WEBRTC_VIDEO_CODEC_FALLBACK_SOFTWARE,
            rtc_encoder_->Encode(
                webrtc::VideoFrame(buffer, 0, 0, webrtc::kVideoRotation_0),
                nullptr, &frame_types));
}

TEST_F(RTCVideoEncoderTest, EncodeScaledFrame) {
  const webrtc::VideoCodecType codec_type = webrtc::kVideoCodecVP8;
  CreateEncoder(codec_type);
  webrtc::VideoCodec codec = GetDefaultCodec();
  EXPECT_EQ(WEBRTC_VIDEO_CODEC_OK, rtc_encoder_->InitEncode(&codec, 1, 12345));

  EXPECT_CALL(*mock_vea_, Encode(_, _))
      .Times(2)
      .WillRepeatedly(Invoke(this, &RTCVideoEncoderTest::VerifyEncodedFrame));

  const rtc::scoped_refptr<webrtc::I420Buffer> buffer =
      webrtc::I420Buffer::Create(kInputFrameWidth, kInputFrameHeight);
  FillFrameBuffer(buffer);
  std::vector<webrtc::FrameType> frame_types;
  EXPECT_EQ(WEBRTC_VIDEO_CODEC_OK,
            rtc_encoder_->Encode(
                webrtc::VideoFrame(buffer, 0, 0, webrtc::kVideoRotation_0),
                nullptr, &frame_types));

  const rtc::scoped_refptr<webrtc::I420Buffer> upscaled_buffer =
      webrtc::I420Buffer::Create(2 * kInputFrameWidth, 2 * kInputFrameHeight);
  FillFrameBuffer(upscaled_buffer);
  webrtc::VideoFrame rtc_frame(upscaled_buffer, 0, 0, webrtc::kVideoRotation_0);
  rtc_frame.set_ntp_time_ms(123456);
  EXPECT_EQ(WEBRTC_VIDEO_CODEC_OK,
            rtc_encoder_->Encode(rtc_frame, nullptr, &frame_types));
}

TEST_F(RTCVideoEncoderTest, PreserveTimestamps) {
  const webrtc::VideoCodecType codec_type = webrtc::kVideoCodecVP8;
  CreateEncoder(codec_type);
  webrtc::VideoCodec codec = GetDefaultCodec();
  EXPECT_EQ(WEBRTC_VIDEO_CODEC_OK, rtc_encoder_->InitEncode(&codec, 1, 12345));

  const uint32_t rtp_timestamp = 1234567;
  const uint32_t capture_time_ms = 3456789;
  RegisterEncodeCompleteCallback(
      base::Bind(&RTCVideoEncoderTest::VerifyTimestamp, base::Unretained(this),
                 rtp_timestamp, capture_time_ms));

  EXPECT_CALL(*mock_vea_, Encode(_, _))
      .WillOnce(Invoke(this, &RTCVideoEncoderTest::ReturnFrameWithTimeStamp));
  const rtc::scoped_refptr<webrtc::I420Buffer> buffer =
      webrtc::I420Buffer::Create(kInputFrameWidth, kInputFrameHeight);
  FillFrameBuffer(buffer);
  std::vector<webrtc::FrameType> frame_types;
  webrtc::VideoFrame rtc_frame(buffer, rtp_timestamp, 0,
                               webrtc::kVideoRotation_0);
  rtc_frame.set_timestamp_us(capture_time_ms * rtc::kNumMicrosecsPerMillisec);
  // We need to set ntp_time_ms because it will be used to derive
  // media::VideoFrame timestamp.
  rtc_frame.set_ntp_time_ms(4567891);
  EXPECT_EQ(WEBRTC_VIDEO_CODEC_OK,
            rtc_encoder_->Encode(rtc_frame, nullptr, &frame_types));
}

}  // namespace content
