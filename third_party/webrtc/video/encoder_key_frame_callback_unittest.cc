/*
 *  Copyright (c) 2012 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#include "video/encoder_key_frame_callback.h"

#include <memory>

#include "test/gmock.h"
#include "test/gtest.h"
#include "video/test/mock_video_stream_encoder.h"

using ::testing::NiceMock;

namespace webrtc {

class VieKeyRequestTest : public ::testing::Test {
 public:
  VieKeyRequestTest()
      : simulated_clock_(123456789),
        encoder_(),
        encoder_key_frame_callback_(
            &simulated_clock_,
            std::vector<uint32_t>(1, VieKeyRequestTest::kSsrc),
            &encoder_) {}

 protected:
  const uint32_t kSsrc = 1234;

  SimulatedClock simulated_clock_;
  testing::StrictMock<MockVideoStreamEncoder> encoder_;
  EncoderKeyFrameCallback encoder_key_frame_callback_;
};

TEST_F(VieKeyRequestTest, CreateAndTriggerRequests) {
  EXPECT_CALL(encoder_, SendKeyFrame()).Times(1);
  encoder_key_frame_callback_.OnReceivedIntraFrameRequest(kSsrc);
}

TEST_F(VieKeyRequestTest, TooManyOnReceivedIntraFrameRequest) {
  EXPECT_CALL(encoder_, SendKeyFrame()).Times(1);
  encoder_key_frame_callback_.OnReceivedIntraFrameRequest(kSsrc);
  encoder_key_frame_callback_.OnReceivedIntraFrameRequest(kSsrc);
  simulated_clock_.AdvanceTimeMilliseconds(10);
  encoder_key_frame_callback_.OnReceivedIntraFrameRequest(kSsrc);

  EXPECT_CALL(encoder_, SendKeyFrame()).Times(1);
  simulated_clock_.AdvanceTimeMilliseconds(300);
  encoder_key_frame_callback_.OnReceivedIntraFrameRequest(kSsrc);
  encoder_key_frame_callback_.OnReceivedIntraFrameRequest(kSsrc);
  encoder_key_frame_callback_.OnReceivedIntraFrameRequest(kSsrc);
}

TEST_F(VieKeyRequestTest, TriggerRequestFromMediaTransport) {
  EXPECT_CALL(encoder_, SendKeyFrame()).Times(1);
  encoder_key_frame_callback_.OnKeyFrameRequested(kSsrc);
}

}  // namespace webrtc
