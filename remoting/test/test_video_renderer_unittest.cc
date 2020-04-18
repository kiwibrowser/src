// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "remoting/test/test_video_renderer.h"

#include <stdint.h>

#include <cmath>
#include <memory>
#include <utility>
#include <vector>

#include "base/macros.h"
#include "base/memory/ptr_util.h"
#include "base/message_loop/message_loop.h"
#include "base/message_loop/message_loop_current.h"
#include "base/run_loop.h"
#include "base/threading/thread_task_runner_handle.h"
#include "base/timer/timer.h"
#include "remoting/codec/video_encoder.h"
#include "remoting/codec/video_encoder_verbatim.h"
#include "remoting/codec/video_encoder_vpx.h"
#include "remoting/proto/video.pb.h"
#include "remoting/test/rgb_value.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "third_party/webrtc/modules/desktop_capture/desktop_frame.h"
#include "third_party/webrtc/modules/desktop_capture/desktop_region.h"

namespace {

// Used to verify if image pattern is matched.
void ProcessPacketDoneHandler(const base::Closure& done_closure,
                              bool* handler_called) {
  *handler_called = true;
  done_closure.Run();
}

const int kDefaultScreenWidthPx = 1024;
const int kDefaultScreenHeightPx = 768;

// Default max error for encoding and decoding, measured in percent.
const double kDefaultErrorLimit = 0.02;

// Default expected rect for image pattern, measured in pixels.
const webrtc::DesktopRect kDefaultExpectedRect =
    webrtc::DesktopRect::MakeLTRB(100, 100, 200, 200);
}  // namespace

namespace remoting {
namespace test {

// Provides basic functionality for for the TestVideoRenderer Tests below.
// This fixture also creates an MessageLoop to test decoding video packets.
class TestVideoRendererTest : public testing::Test {
 public:
  TestVideoRendererTest();
  ~TestVideoRendererTest() override;

  // Handles creating a frame and sending to TestVideoRenderer for processing.
  void TestVideoPacketProcessing(int screen_width, int screen_height,
                                 double error_limit);

  // Handles setting an image pattern and sending a frame to TestVideoRenderer.
  // |expect_to_match| indicates if the image pattern is expected to match.
  void TestImagePatternMatch(int screen_width,
                             int screen_height,
                             const webrtc::DesktopRect& expected_rect,
                             bool expect_to_match);

  // Generate a basic desktop frame containing a gradient.
  std::unique_ptr<webrtc::DesktopFrame> CreateDesktopFrameWithGradient(
      int screen_width,
      int screen_height) const;

 protected:
  // Used to post tasks to the message loop.
  std::unique_ptr<base::RunLoop> run_loop_;

  // Used to set timeouts and delays.
  std::unique_ptr<base::Timer> timer_;

  // Manages the decoder and process generated video packets.
  std::unique_ptr<TestVideoRenderer> test_video_renderer_;

  // Used to encode desktop frames to generate video packets.
  std::unique_ptr<VideoEncoder> encoder_;

 private:
  // testing::Test interface.
  void SetUp() override;

  // Set image pattern, send video packet and returns if the expected pattern is
  // matched.
  bool SendPacketAndWaitForMatch(std::unique_ptr<VideoPacket> packet,
                                 const webrtc::DesktopRect& expected_rect,
                                 const RGBValue& expected_average_color);

  // Returns the average color value of pixels fall within |rect|.
  // NOTE: Callers should not release the objects.
  RGBValue CalculateAverageColorValueForFrame(
      const webrtc::DesktopFrame* frame,
      const webrtc::DesktopRect& rect) const;

  // Return the mean error of two frames over all pixels, where error at each
  // pixel is the root mean square of the errors in the R, G and B components,
  // each normalized to [0, 1].
  double CalculateError(const webrtc::DesktopFrame* original_frame,
                        const webrtc::DesktopFrame* decoded_frame) const;

  // Fill a desktop frame with a gradient.
  void FillFrameWithGradient(webrtc::DesktopFrame* frame) const;

  // The thread's message loop. Valid only when the thread is alive.
  std::unique_ptr<base::MessageLoop> message_loop_;

  DISALLOW_COPY_AND_ASSIGN(TestVideoRendererTest);
};

TestVideoRendererTest::TestVideoRendererTest()
    : timer_(new base::Timer(true, false)) {}

TestVideoRendererTest::~TestVideoRendererTest() = default;

void TestVideoRendererTest::SetUp() {
  if (!base::MessageLoopCurrent::Get()) {
    // Create a temporary message loop if the current thread does not already
    // have one.
    message_loop_.reset(new base::MessageLoop);
  }
  test_video_renderer_.reset(new TestVideoRenderer());
}

void TestVideoRendererTest::TestVideoPacketProcessing(int screen_width,
                                                      int screen_height,
                                                      double error_limit) {
  DCHECK(encoder_);
  DCHECK(test_video_renderer_);

  // Generate a frame containing a gradient.
  std::unique_ptr<webrtc::DesktopFrame> original_frame =
      CreateDesktopFrameWithGradient(screen_width, screen_height);
  EXPECT_TRUE(original_frame);

  std::unique_ptr<VideoPacket> packet = encoder_->Encode(*original_frame.get());

  DCHECK(!run_loop_ || !run_loop_->running());
  DCHECK(!timer_->IsRunning());
  run_loop_.reset(new base::RunLoop());

  // Set an extremely long time: 10 min to prevent bugs from hanging the system.
  // NOTE: We've seen cases which take up to 1 min to process a packet, so an
  // extremely long time as 10 min is chosen to avoid being variable/flaky.
  timer_->Start(FROM_HERE, base::TimeDelta::FromMinutes(10),
                run_loop_->QuitClosure());

  // Wait for the video packet to be processed and rendered to buffer.
  test_video_renderer_->ProcessVideoPacket(std::move(packet),
                                           run_loop_->QuitClosure());

  run_loop_->Run();
  EXPECT_TRUE(timer_->IsRunning());
  timer_->Stop();
  run_loop_.reset();

  std::unique_ptr<webrtc::DesktopFrame> buffer_copy =
      test_video_renderer_->GetCurrentFrameForTest();
  EXPECT_NE(buffer_copy, nullptr);

  // The original frame is compared to the decoded video frame to check that
  // the mean error over all pixels does not exceed a given limit.
  double error = CalculateError(original_frame.get(), buffer_copy.get());
  EXPECT_LT(error, error_limit);
}

bool TestVideoRendererTest::SendPacketAndWaitForMatch(
    std::unique_ptr<VideoPacket> packet,
    const webrtc::DesktopRect& expected_rect,
    const RGBValue& expected_average_color) {
  DCHECK(!run_loop_ || !run_loop_->running());
  DCHECK(!timer_->IsRunning());
  run_loop_.reset(new base::RunLoop());

  // Set an extremely long time: 10 min to prevent bugs from hanging the system.
  // NOTE: We've seen cases which take up to 1 min to process a packet, so an
  // extremely long time as 10 min is chosen to avoid being variable/flaky.
  timer_->Start(FROM_HERE, base::TimeDelta::FromMinutes(10),
                run_loop_->QuitClosure());

  // Set expected image pattern.
  test_video_renderer_->ExpectAverageColorInRect(
      expected_rect, expected_average_color, run_loop_->QuitClosure());

  // Used to verify if the expected image pattern will be matched by |packet|.
  std::unique_ptr<VideoPacket> packet_copy(new VideoPacket(*packet.get()));

  // Post first test packet: |packet|.
  test_video_renderer_->ProcessVideoPacket(std::move(packet),
                                           base::DoNothing());

  // Second packet: |packet_copy| is posted, and |second_packet_done_callback|
  // will always be posted back to main thread, however, whether it will be
  // called depends on whether the expected pattern is matched or not.
  bool second_packet_done_is_called = false;
  base::Closure second_packet_done_callback =
      base::Bind(&ProcessPacketDoneHandler, run_loop_->QuitClosure(),
                 &second_packet_done_is_called);

  test_video_renderer_->ProcessVideoPacket(std::move(packet_copy),
                                           second_packet_done_callback);

  run_loop_->Run();
  EXPECT_TRUE(timer_->IsRunning());
  timer_->Stop();
  run_loop_.reset();

  // if expected image pattern is matched, the QuitClosure of |run_loop_| will
  // be called before |second_packet_done_callback|, which leaves
  // |second_packet_done_is_called| be false.
  bool image_pattern_is_matched = !second_packet_done_is_called;

  return image_pattern_is_matched;
}

void TestVideoRendererTest::TestImagePatternMatch(
    int screen_width,
    int screen_height,
    const webrtc::DesktopRect& expected_rect,
    bool expect_to_match) {
  DCHECK(encoder_);
  DCHECK(test_video_renderer_);

  std::unique_ptr<webrtc::DesktopFrame> frame =
      CreateDesktopFrameWithGradient(screen_width, screen_height);
  RGBValue expected_average_color =
      CalculateAverageColorValueForFrame(frame.get(), expected_rect);
  std::unique_ptr<VideoPacket> packet = encoder_->Encode(*frame.get());

  if (expect_to_match) {
    EXPECT_TRUE(SendPacketAndWaitForMatch(std::move(packet), expected_rect,
                                          expected_average_color));
  } else {
    // Shift each channel by 128.
    // e.g. (10, 127, 200) -> (138, 255, 73).
    // In this way, the error between expected color and true value is always
    // around 0.5.
    int red_shift = (expected_average_color.red + 128) % 255;
    int green_shift = (expected_average_color.green + 128) % 255;
    int blue_shift = (expected_average_color.blue + 128) % 255;

    RGBValue expected_average_color_shift =
        RGBValue(red_shift, green_shift, blue_shift);

    EXPECT_FALSE(SendPacketAndWaitForMatch(std::move(packet), expected_rect,
                                           expected_average_color_shift));
  }
}

RGBValue TestVideoRendererTest::CalculateAverageColorValueForFrame(
    const webrtc::DesktopFrame* frame,
    const webrtc::DesktopRect& rect) const {
  int red_sum = 0;
  int green_sum = 0;
  int blue_sum = 0;

  // Loop through pixels that fall within |accumulating_rect_| to obtain the
  // average color value.
  for (int y = rect.top(); y < rect.bottom(); ++y) {
    uint8_t* frame_pos =
        frame->data() + (y * frame->stride() +
                         rect.left() * webrtc::DesktopFrame::kBytesPerPixel);

    // Pixels of decoded video frame are presented in ARGB format.
    for (int x = 0; x < rect.width(); ++x) {
      red_sum += frame_pos[2];
      green_sum += frame_pos[1];
      blue_sum += frame_pos[0];
      frame_pos += 4;
    }
  }

  int area = rect.width() * rect.height();

  return RGBValue(red_sum / area, green_sum / area, blue_sum / area);
}

double TestVideoRendererTest::CalculateError(
    const webrtc::DesktopFrame* original_frame,
    const webrtc::DesktopFrame* decoded_frame) const {
  DCHECK(original_frame);
  DCHECK(decoded_frame);

  // Check size remains the same after encoding and decoding.
  EXPECT_EQ(original_frame->size().width(), decoded_frame->size().width());
  EXPECT_EQ(original_frame->size().height(), decoded_frame->size().height());
  EXPECT_EQ(original_frame->stride(), decoded_frame->stride());
  int screen_width = original_frame->size().width();
  int screen_height = original_frame->size().height();

  // Error is calculated as the sum of the square error at each pixel in the
  // R, G and B components, each normalized to [0, 1].
  double error_sum_squares = 0.0;

  // The mapping between the position of a pixel on 3-dimensional image
  // (origin at top left corner) and its position in 1-dimensional buffer.
  //
  //  _______________
  // |      |        |      stride = 4 * width;
  // |      |        |
  // |      | height |      height * stride + width + 0; Red channel.
  // |      |        |  =>  height * stride + width + 1; Green channel.
  // |-------        |      height * stride + width + 2; Blue channel.
  // | width         |
  // |_______________|
  //
  for (int height = 0; height < screen_height; ++height) {
    uint8_t* original_ptr = original_frame->data() +
                            height * original_frame->stride();
    uint8_t* decoded_ptr = decoded_frame->data() +
                           height * decoded_frame->stride();

    for (int width = 0; width < screen_width; ++width) {
      // Errors are calculated in the R, G, B components.
      for (int j = 0; j < 3; ++j) {
        int offset = webrtc::DesktopFrame::kBytesPerPixel * width + j;
        double original_value = static_cast<double>(*(original_ptr + offset));
        double decoded_value = static_cast<double>(*(decoded_ptr + offset));
        double error = original_value - decoded_value;

        // Normalize the error to [0, 1].
        error /= 255.0;
        error_sum_squares += error * error;
      }
    }
  }
  return sqrt(error_sum_squares / (3 * screen_width * screen_height));
}

std::unique_ptr<webrtc::DesktopFrame>
TestVideoRendererTest::CreateDesktopFrameWithGradient(int screen_width,
                                                      int screen_height) const {
  webrtc::DesktopSize screen_size(screen_width, screen_height);
  std::unique_ptr<webrtc::DesktopFrame> frame(
      new webrtc::BasicDesktopFrame(screen_size));
  frame->mutable_updated_region()->SetRect(
      webrtc::DesktopRect::MakeSize(screen_size));
  FillFrameWithGradient(frame.get());
  return frame;
}

void TestVideoRendererTest::FillFrameWithGradient(
    webrtc::DesktopFrame* frame) const {
  for (int y = 0; y < frame->size().height(); ++y) {
    uint8_t* p = frame->data() + y * frame->stride();
    for (int x = 0; x < frame->size().width(); ++x) {
      *p++ = (255.0 * x) / frame->size().width();
      *p++ = (164.0 * y) / frame->size().height();
      *p++ = (82.0 * (x + y)) /
          (frame->size().width() + frame->size().height());
      *p++ = 0;
    }
  }
}

// Verify video decoding for VP8 Codec.
TEST_F(TestVideoRendererTest, VerifyVideoDecodingForVP8) {
  encoder_ = VideoEncoderVpx::CreateForVP8();
  test_video_renderer_->SetCodecForDecoding(
      protocol::ChannelConfig::CODEC_VP8);
  TestVideoPacketProcessing(kDefaultScreenWidthPx, kDefaultScreenHeightPx,
                            kDefaultErrorLimit);
}

// Verify video decoding for VP9 Codec.
TEST_F(TestVideoRendererTest, VerifyVideoDecodingForVP9) {
  encoder_ = VideoEncoderVpx::CreateForVP9();
  test_video_renderer_->SetCodecForDecoding(
      protocol::ChannelConfig::CODEC_VP9);
  TestVideoPacketProcessing(kDefaultScreenWidthPx, kDefaultScreenHeightPx,
                            kDefaultErrorLimit);
}


// Verify video decoding for VERBATIM Codec.
TEST_F(TestVideoRendererTest, VerifyVideoDecodingForVERBATIM) {
  encoder_.reset(new VideoEncoderVerbatim());
  test_video_renderer_->SetCodecForDecoding(
      protocol::ChannelConfig::CODEC_VERBATIM);
  TestVideoPacketProcessing(kDefaultScreenWidthPx, kDefaultScreenHeightPx,
                            kDefaultErrorLimit);
}

// Verify a set of video packets are processed correctly.
TEST_F(TestVideoRendererTest, VerifyMultipleVideoProcessing) {
  encoder_ = VideoEncoderVpx::CreateForVP8();
  test_video_renderer_->SetCodecForDecoding(
      protocol::ChannelConfig::CODEC_VP8);

  // Post multiple tasks to |test_video_renderer_|, and it should not crash.
  // 20 is chosen because it's large enough to make sure that there will be
  // more than one task on the video decode thread, while not too large to wait
  // for too long for the unit test to complete.
  const int task_num = 20;
  std::vector<std::unique_ptr<VideoPacket>> video_packets;
  for (int i = 0; i < task_num; ++i) {
    std::unique_ptr<webrtc::DesktopFrame> original_frame =
        CreateDesktopFrameWithGradient(kDefaultScreenWidthPx,
                                       kDefaultScreenHeightPx);
    video_packets.push_back(encoder_->Encode(*original_frame.get()));
  }

  for (auto& packet : video_packets) {
    test_video_renderer_->ProcessVideoPacket(std::move(packet),
                                             base::DoNothing());
  }
}

// Verify video packet size change is handled properly.
TEST_F(TestVideoRendererTest, VerifyVideoPacketSizeChange) {
  encoder_ = VideoEncoderVpx::CreateForVP8();
  test_video_renderer_->SetCodecForDecoding(
      protocol::ChannelConfig::Codec::CODEC_VP8);

  TestVideoPacketProcessing(kDefaultScreenWidthPx, kDefaultScreenHeightPx,
                            kDefaultErrorLimit);

  TestVideoPacketProcessing(2 * kDefaultScreenWidthPx,
                            2 * kDefaultScreenHeightPx, kDefaultErrorLimit);

  TestVideoPacketProcessing(kDefaultScreenWidthPx / 2,
                            kDefaultScreenHeightPx / 2, kDefaultErrorLimit);
}

// Verify setting expected image pattern doesn't break video packet processing.
TEST_F(TestVideoRendererTest, VerifySetExpectedImagePattern) {
  encoder_ = VideoEncoderVpx::CreateForVP8();
  test_video_renderer_->SetCodecForDecoding(
      protocol::ChannelConfig::Codec::CODEC_VP8);

  DCHECK(encoder_);
  DCHECK(test_video_renderer_);

  std::unique_ptr<webrtc::DesktopFrame> frame = CreateDesktopFrameWithGradient(
      kDefaultScreenWidthPx, kDefaultScreenHeightPx);

  // Since we don't care whether expected image pattern is matched or not in
  // this case, an expected color is chosen arbitrarily.
  RGBValue black_color = RGBValue();

  // Set expected image pattern.
  test_video_renderer_->ExpectAverageColorInRect(
      kDefaultExpectedRect, black_color, base::DoNothing());

  // Post test video packet.
  test_video_renderer_->ProcessVideoPacket(encoder_->Encode(*frame.get()),
                                           base::DoNothing());
}

// Verify correct image pattern can be matched for VP8.
TEST_F(TestVideoRendererTest, VerifyImagePatternMatchForVP8) {
  encoder_ = VideoEncoderVpx::CreateForVP8();
  test_video_renderer_->SetCodecForDecoding(
      protocol::ChannelConfig::Codec::CODEC_VP8);
  TestImagePatternMatch(kDefaultScreenWidthPx, kDefaultScreenHeightPx,
                        kDefaultExpectedRect, true);
}

// Verify expected image pattern can be matched for VP9.
TEST_F(TestVideoRendererTest, VerifyImagePatternMatchForVP9) {
  encoder_ = VideoEncoderVpx::CreateForVP9();
  test_video_renderer_->SetCodecForDecoding(
      protocol::ChannelConfig::Codec::CODEC_VP9);
  TestImagePatternMatch(kDefaultScreenWidthPx, kDefaultScreenHeightPx,
                        kDefaultExpectedRect, true);
}

// Verify expected image pattern can be matched for VERBATIM.
TEST_F(TestVideoRendererTest, VerifyImagePatternMatchForVERBATIM) {
  encoder_.reset(new VideoEncoderVerbatim());
  test_video_renderer_->SetCodecForDecoding(
      protocol::ChannelConfig::Codec::CODEC_VERBATIM);
  TestImagePatternMatch(kDefaultScreenWidthPx, kDefaultScreenHeightPx,
                        kDefaultExpectedRect, true);
}

// Verify incorrect image pattern shouldn't be matched for VP8.
TEST_F(TestVideoRendererTest, VerifyImagePatternNotMatchForVP8) {
  encoder_ = VideoEncoderVpx::CreateForVP8();
  test_video_renderer_->SetCodecForDecoding(
      protocol::ChannelConfig::Codec::CODEC_VP8);
  TestImagePatternMatch(kDefaultScreenWidthPx, kDefaultScreenHeightPx,
                        kDefaultExpectedRect, false);
}

// Verify incorrect image pattern shouldn't be matched for VP9.
TEST_F(TestVideoRendererTest, VerifyImagePatternNotMatchForVP9) {
  encoder_ = VideoEncoderVpx::CreateForVP9();
  test_video_renderer_->SetCodecForDecoding(
      protocol::ChannelConfig::Codec::CODEC_VP9);
  TestImagePatternMatch(kDefaultScreenWidthPx, kDefaultScreenWidthPx,
                        kDefaultExpectedRect, false);
}

// Verify incorrect image pattern shouldn't be matched for VERBATIM.
TEST_F(TestVideoRendererTest, VerifyImagePatternNotMatchForVERBATIM) {
  encoder_.reset(new VideoEncoderVerbatim());
  test_video_renderer_->SetCodecForDecoding(
      protocol::ChannelConfig::Codec::CODEC_VERBATIM);
  TestImagePatternMatch(kDefaultScreenWidthPx, kDefaultScreenHeightPx,
                        kDefaultExpectedRect, false);
}

}  // namespace test
}  // namespace remoting
