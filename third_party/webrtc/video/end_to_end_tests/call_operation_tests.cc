/*
 *  Copyright 2018 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#include "system_wrappers/include/sleep.h"
#include "test/call_test.h"
#include "test/field_trial.h"
#include "test/frame_generator.h"
#include "test/gtest.h"
#include "test/null_transport.h"

namespace webrtc {

class CallOperationEndToEndTest
    : public test::CallTest,
      public testing::WithParamInterface<std::string> {
 public:
  CallOperationEndToEndTest() : field_trial_(GetParam()) {}

  virtual ~CallOperationEndToEndTest() {
    EXPECT_EQ(nullptr, video_send_stream_);
    EXPECT_TRUE(video_receive_streams_.empty());
  }

 private:
  test::ScopedFieldTrials field_trial_;
};

INSTANTIATE_TEST_CASE_P(
    FieldTrials,
    CallOperationEndToEndTest,
    ::testing::Values("WebRTC-RoundRobinPacing/Disabled/",
                      "WebRTC-RoundRobinPacing/Enabled/",
                      "WebRTC-TaskQueueCongestionControl/Enabled/"));

TEST_P(CallOperationEndToEndTest, ReceiverCanBeStartedTwice) {
  CreateCalls(Call::Config(event_log_.get()), Call::Config(event_log_.get()));

  test::NullTransport transport;
  CreateSendConfig(1, 0, 0, &transport);
  CreateMatchingReceiveConfigs(&transport);

  CreateVideoStreams();

  video_receive_streams_[0]->Start();
  video_receive_streams_[0]->Start();

  DestroyStreams();
}

TEST_P(CallOperationEndToEndTest, ReceiverCanBeStoppedTwice) {
  CreateCalls(Call::Config(event_log_.get()), Call::Config(event_log_.get()));

  test::NullTransport transport;
  CreateSendConfig(1, 0, 0, &transport);
  CreateMatchingReceiveConfigs(&transport);

  CreateVideoStreams();

  video_receive_streams_[0]->Stop();
  video_receive_streams_[0]->Stop();

  DestroyStreams();
}

TEST_P(CallOperationEndToEndTest, ReceiverCanBeStoppedAndRestarted) {
  CreateCalls(Call::Config(event_log_.get()), Call::Config(event_log_.get()));

  test::NullTransport transport;
  CreateSendConfig(1, 0, 0, &transport);
  CreateMatchingReceiveConfigs(&transport);

  CreateVideoStreams();

  video_receive_streams_[0]->Stop();
  video_receive_streams_[0]->Start();
  video_receive_streams_[0]->Stop();

  DestroyStreams();
}

TEST_P(CallOperationEndToEndTest, RendersSingleDelayedFrame) {
  static const int kWidth = 320;
  static const int kHeight = 240;
  // This constant is chosen to be higher than the timeout in the video_render
  // module. This makes sure that frames aren't dropped if there are no other
  // frames in the queue.
  static const int kRenderDelayMs = 1000;

  class Renderer : public rtc::VideoSinkInterface<VideoFrame> {
   public:
    Renderer() : event_(false, false) {}

    void OnFrame(const VideoFrame& video_frame) override {
      SleepMs(kRenderDelayMs);
      event_.Set();
    }

    bool Wait() { return event_.Wait(kDefaultTimeoutMs); }

    rtc::Event event_;
  } renderer;

  test::FrameForwarder frame_forwarder;
  std::unique_ptr<test::DirectTransport> sender_transport;
  std::unique_ptr<test::DirectTransport> receiver_transport;

  task_queue_.SendTask([this, &renderer, &frame_forwarder, &sender_transport,
                        &receiver_transport]() {
    CreateCalls(Call::Config(event_log_.get()), Call::Config(event_log_.get()));

    sender_transport = rtc::MakeUnique<test::DirectTransport>(
        &task_queue_, sender_call_.get(), payload_type_map_);
    receiver_transport = rtc::MakeUnique<test::DirectTransport>(
        &task_queue_, receiver_call_.get(), payload_type_map_);
    sender_transport->SetReceiver(receiver_call_->Receiver());
    receiver_transport->SetReceiver(sender_call_->Receiver());

    CreateSendConfig(1, 0, 0, sender_transport.get());
    CreateMatchingReceiveConfigs(receiver_transport.get());

    video_receive_configs_[0].renderer = &renderer;

    CreateVideoStreams();
    Start();

    // Create frames that are smaller than the send width/height, this is done
    // to check that the callbacks are done after processing video.
    std::unique_ptr<test::FrameGenerator> frame_generator(
        test::FrameGenerator::CreateSquareGenerator(
            kWidth, kHeight, rtc::nullopt, rtc::nullopt));
    video_send_stream_->SetSource(&frame_forwarder,
                                  DegradationPreference::MAINTAIN_FRAMERATE);

    frame_forwarder.IncomingCapturedFrame(*frame_generator->NextFrame());
  });

  EXPECT_TRUE(renderer.Wait())
      << "Timed out while waiting for the frame to render.";

  task_queue_.SendTask([this, &sender_transport, &receiver_transport]() {
    Stop();
    DestroyStreams();
    sender_transport.reset();
    receiver_transport.reset();
    DestroyCalls();
  });
}

TEST_P(CallOperationEndToEndTest, TransmitsFirstFrame) {
  class Renderer : public rtc::VideoSinkInterface<VideoFrame> {
   public:
    Renderer() : event_(false, false) {}

    void OnFrame(const VideoFrame& video_frame) override { event_.Set(); }

    bool Wait() { return event_.Wait(kDefaultTimeoutMs); }

    rtc::Event event_;
  } renderer;

  std::unique_ptr<test::FrameGenerator> frame_generator;
  test::FrameForwarder frame_forwarder;

  std::unique_ptr<test::DirectTransport> sender_transport;
  std::unique_ptr<test::DirectTransport> receiver_transport;

  task_queue_.SendTask([this, &renderer, &frame_generator, &frame_forwarder,
                        &sender_transport, &receiver_transport]() {
    CreateCalls(Call::Config(event_log_.get()), Call::Config(event_log_.get()));

    sender_transport = rtc::MakeUnique<test::DirectTransport>(
        &task_queue_, sender_call_.get(), payload_type_map_);
    receiver_transport = rtc::MakeUnique<test::DirectTransport>(
        &task_queue_, receiver_call_.get(), payload_type_map_);
    sender_transport->SetReceiver(receiver_call_->Receiver());
    receiver_transport->SetReceiver(sender_call_->Receiver());

    CreateSendConfig(1, 0, 0, sender_transport.get());
    CreateMatchingReceiveConfigs(receiver_transport.get());
    video_receive_configs_[0].renderer = &renderer;

    CreateVideoStreams();
    Start();

    frame_generator = test::FrameGenerator::CreateSquareGenerator(
        kDefaultWidth, kDefaultHeight, rtc::nullopt, rtc::nullopt);
    video_send_stream_->SetSource(&frame_forwarder,
                                  DegradationPreference::MAINTAIN_FRAMERATE);
    frame_forwarder.IncomingCapturedFrame(*frame_generator->NextFrame());
  });

  EXPECT_TRUE(renderer.Wait())
      << "Timed out while waiting for the frame to render.";

  task_queue_.SendTask([this, &sender_transport, &receiver_transport]() {
    Stop();
    DestroyStreams();
    sender_transport.reset();
    receiver_transport.reset();
    DestroyCalls();
  });
}

TEST_P(CallOperationEndToEndTest, ObserversEncodedFrames) {
  class EncodedFrameTestObserver : public EncodedFrameObserver {
   public:
    EncodedFrameTestObserver()
        : length_(0), frame_type_(kEmptyFrame), called_(false, false) {}
    virtual ~EncodedFrameTestObserver() {}

    virtual void EncodedFrameCallback(const EncodedFrame& encoded_frame) {
      frame_type_ = encoded_frame.frame_type_;
      length_ = encoded_frame.length_;
      buffer_.reset(new uint8_t[length_]);
      memcpy(buffer_.get(), encoded_frame.data_, length_);
      called_.Set();
    }

    bool Wait() { return called_.Wait(kDefaultTimeoutMs); }

    void ExpectEqualFrames(const EncodedFrameTestObserver& observer) {
      ASSERT_EQ(length_, observer.length_)
          << "Observed frames are of different lengths.";
      EXPECT_EQ(frame_type_, observer.frame_type_)
          << "Observed frames have different frame types.";
      EXPECT_EQ(0, memcmp(buffer_.get(), observer.buffer_.get(), length_))
          << "Observed encoded frames have different content.";
    }

   private:
    std::unique_ptr<uint8_t[]> buffer_;
    size_t length_;
    FrameType frame_type_;
    rtc::Event called_;
  };

  EncodedFrameTestObserver post_encode_observer;
  EncodedFrameTestObserver pre_decode_observer;
  test::FrameForwarder forwarder;
  std::unique_ptr<test::FrameGenerator> frame_generator;

  std::unique_ptr<test::DirectTransport> sender_transport;
  std::unique_ptr<test::DirectTransport> receiver_transport;

  task_queue_.SendTask([&]() {
    CreateCalls(Call::Config(event_log_.get()), Call::Config(event_log_.get()));

    sender_transport = rtc::MakeUnique<test::DirectTransport>(
        &task_queue_, sender_call_.get(), payload_type_map_);
    receiver_transport = rtc::MakeUnique<test::DirectTransport>(
        &task_queue_, receiver_call_.get(), payload_type_map_);
    sender_transport->SetReceiver(receiver_call_->Receiver());
    receiver_transport->SetReceiver(sender_call_->Receiver());

    CreateSendConfig(1, 0, 0, sender_transport.get());
    CreateMatchingReceiveConfigs(receiver_transport.get());
    video_send_config_.post_encode_callback = &post_encode_observer;
    video_receive_configs_[0].pre_decode_callback = &pre_decode_observer;

    CreateVideoStreams();
    Start();

    frame_generator = test::FrameGenerator::CreateSquareGenerator(
        kDefaultWidth, kDefaultHeight, rtc::nullopt, rtc::nullopt);
    video_send_stream_->SetSource(&forwarder,
                                  DegradationPreference::MAINTAIN_FRAMERATE);
    forwarder.IncomingCapturedFrame(*frame_generator->NextFrame());
  });

  EXPECT_TRUE(post_encode_observer.Wait())
      << "Timed out while waiting for send-side encoded-frame callback.";

  EXPECT_TRUE(pre_decode_observer.Wait())
      << "Timed out while waiting for pre-decode encoded-frame callback.";

  post_encode_observer.ExpectEqualFrames(pre_decode_observer);

  task_queue_.SendTask([this, &sender_transport, &receiver_transport]() {
    Stop();
    DestroyStreams();
    sender_transport.reset();
    receiver_transport.reset();
    DestroyCalls();
  });
}

}  // namespace webrtc
