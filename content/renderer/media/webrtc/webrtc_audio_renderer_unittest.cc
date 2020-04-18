// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/renderer/media/webrtc/webrtc_audio_renderer.h"

#include <string>
#include <utility>
#include <vector>

#include "base/bind.h"
#include "base/message_loop/message_loop.h"
#include "base/run_loop.h"
#include "build/build_config.h"
#include "content/public/renderer/media_stream_audio_renderer.h"
#include "content/renderer/media/audio_device_factory.h"
#include "content/renderer/media/webrtc/webrtc_audio_device_impl.h"
#include "media/base/audio_capturer_source.h"
#include "media/base/mock_audio_renderer_sink.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "third_party/blink/public/platform/web_media_stream.h"
#include "third_party/blink/public/platform/web_media_stream_track.h"
#include "third_party/blink/public/platform/web_string.h"
#include "third_party/blink/public/web/web_heap.h"
#include "third_party/webrtc/api/mediastreaminterface.h"

using testing::Return;
using testing::_;

namespace content {

namespace {

const int kHardwareSampleRate = 44100;
const int kHardwareBufferSize = 512;
const char kDefaultOutputDeviceId[] = "";
const char kOtherOutputDeviceId[] = "other-output-device";
const char kInvalidOutputDeviceId[] = "invalid-device";

class MockAudioRendererSource : public WebRtcAudioRendererSource {
 public:
  MockAudioRendererSource() {}
  ~MockAudioRendererSource() override {}
  MOCK_METHOD4(RenderData, void(media::AudioBus* audio_bus,
                                int sample_rate,
                                int audio_delay_milliseconds,
                                base::TimeDelta* current_time));
  MOCK_METHOD1(RemoveAudioRenderer, void(WebRtcAudioRenderer* renderer));
  MOCK_METHOD0(AudioRendererThreadStopped, void());
  MOCK_METHOD1(SetOutputDeviceForAec, void(const std::string&));
};

}  // namespace

class WebRtcAudioRendererTest : public testing::Test,
                                public AudioDeviceFactory {
 public:
  MOCK_METHOD1(MockSwitchDeviceCallback, void(media::OutputDeviceStatus));
  void SwitchDeviceCallback(base::RunLoop* loop,
                            media::OutputDeviceStatus result) {
    MockSwitchDeviceCallback(result);
    loop->Quit();
  }

 protected:
  WebRtcAudioRendererTest()
      : message_loop_(new base::MessageLoopForIO),
        source_(new MockAudioRendererSource()) {
    blink::WebVector<blink::WebMediaStreamTrack> dummy_tracks;
    stream_.Initialize(blink::WebString::FromUTF8("new stream"), dummy_tracks,
                       dummy_tracks);
  }

  void SetupRenderer(const std::string& device_id) {
    renderer_ = new WebRtcAudioRenderer(message_loop_->task_runner(), stream_,
                                        1, 1, device_id);
    EXPECT_CALL(*this, MockCreateAudioRendererSink(
                           AudioDeviceFactory::kSourceWebRtc, _, _, device_id));
    EXPECT_CALL(*source_.get(), SetOutputDeviceForAec(device_id));
    EXPECT_TRUE(renderer_->Initialize(source_.get()));

    renderer_proxy_ = renderer_->CreateSharedAudioRendererProxy(stream_);
  }
  MOCK_METHOD1(CreateAudioCapturerSource,
               scoped_refptr<media::AudioCapturerSource>(int));
  MOCK_METHOD3(CreateFinalAudioRendererSink,
               scoped_refptr<media::AudioRendererSink>(int,
                                                       int,
                                                       const std::string&));
  MOCK_METHOD4(
      CreateSwitchableAudioRendererSink,
      scoped_refptr<media::SwitchableAudioRendererSink>(SourceType,
                                                        int,
                                                        int,
                                                        const std::string&));
  MOCK_METHOD4(MockCreateAudioRendererSink,
               void(SourceType, int, int, const std::string&));

  scoped_refptr<media::AudioRendererSink> CreateAudioRendererSink(
      SourceType source_type,
      int render_frame_id,
      int session_id,
      const std::string& device_id) override {
    mock_sink_ = new media::MockAudioRendererSink(
        device_id,
        device_id == kInvalidOutputDeviceId
            ? media::OUTPUT_DEVICE_STATUS_ERROR_INTERNAL
            : media::OUTPUT_DEVICE_STATUS_OK,
        media::AudioParameters(media::AudioParameters::AUDIO_PCM_LOW_LATENCY,
                               media::CHANNEL_LAYOUT_STEREO,
                               kHardwareSampleRate, kHardwareBufferSize));

    if (device_id != kInvalidOutputDeviceId) {
      EXPECT_CALL(*mock_sink_.get(), Start());
      EXPECT_CALL(*mock_sink_.get(), Play());
    } else {
      EXPECT_CALL(*mock_sink_.get(), Stop());
    }

    MockCreateAudioRendererSink(source_type, render_frame_id, session_id,
                                device_id);
    return mock_sink_;
  }

  void TearDown() override {
    renderer_proxy_ = nullptr;
    renderer_ = nullptr;
    stream_.Reset();
    source_.reset();
    mock_sink_ = nullptr;
    blink::WebHeap::CollectAllGarbageForTesting();
  }

  std::unique_ptr<base::MessageLoopForIO> message_loop_;
  scoped_refptr<media::MockAudioRendererSink> mock_sink_;
  std::unique_ptr<MockAudioRendererSource> source_;
  blink::WebMediaStream stream_;
  scoped_refptr<WebRtcAudioRenderer> renderer_;
  scoped_refptr<MediaStreamAudioRenderer> renderer_proxy_;
};

// Verify that the renderer will be stopped if the only proxy is stopped.
TEST_F(WebRtcAudioRendererTest, StopRenderer) {
  SetupRenderer(kDefaultOutputDeviceId);
  renderer_proxy_->Start();

  // |renderer_| has only one proxy, stopping the proxy should stop the sink of
  // |renderer_|.
  EXPECT_CALL(*mock_sink_.get(), Stop());
  EXPECT_CALL(*source_.get(), RemoveAudioRenderer(renderer_.get()));
  renderer_proxy_->Stop();
}

// Verify that the renderer will not be stopped unless the last proxy is
// stopped.
TEST_F(WebRtcAudioRendererTest, MultipleRenderers) {
  SetupRenderer(kDefaultOutputDeviceId);
  renderer_proxy_->Start();

  // Create a vector of renderer proxies from the |renderer_|.
  std::vector<scoped_refptr<MediaStreamAudioRenderer> > renderer_proxies_;
  static const int kNumberOfRendererProxy = 5;
  for (int i = 0; i < kNumberOfRendererProxy; ++i) {
    scoped_refptr<MediaStreamAudioRenderer> renderer_proxy(
        renderer_->CreateSharedAudioRendererProxy(stream_));
    renderer_proxy->Start();
    renderer_proxies_.push_back(renderer_proxy);
  }

  // Stop the |renderer_proxy_| should not stop the sink since it is used by
  // other proxies.
  EXPECT_CALL(*mock_sink_.get(), Stop()).Times(0);
  renderer_proxy_->Stop();

  for (int i = 0; i < kNumberOfRendererProxy; ++i) {
    if (i != kNumberOfRendererProxy -1) {
      EXPECT_CALL(*mock_sink_.get(), Stop()).Times(0);
    } else {
      // When the last proxy is stopped, the sink will stop.
      EXPECT_CALL(*source_.get(), RemoveAudioRenderer(renderer_.get()));
      EXPECT_CALL(*mock_sink_.get(), Stop());
    }
    renderer_proxies_[i]->Stop();
  }
}

// Verify that the sink of the renderer is using the expected sample rate and
// buffer size.
TEST_F(WebRtcAudioRendererTest, VerifySinkParameters) {
  SetupRenderer(kDefaultOutputDeviceId);
  renderer_proxy_->Start();
#if defined(OS_LINUX) || defined(OS_MACOSX) || defined(OS_FUCHSIA)
  static const int kExpectedBufferSize = kHardwareSampleRate / 100;
#elif defined(OS_ANDROID)
  static const int kExpectedBufferSize = 2 * kHardwareSampleRate / 100;
#elif defined(OS_WIN)
  static const int kExpectedBufferSize = kHardwareBufferSize;
#else
#error Unknown platform.
#endif
  EXPECT_EQ(kExpectedBufferSize, renderer_->frames_per_buffer());
  EXPECT_EQ(kHardwareSampleRate, renderer_->sample_rate());
  EXPECT_EQ(2, renderer_->channels());

  EXPECT_CALL(*mock_sink_.get(), Stop());
  EXPECT_CALL(*source_.get(), RemoveAudioRenderer(renderer_.get()));
  renderer_proxy_->Stop();
}

TEST_F(WebRtcAudioRendererTest, NonDefaultDevice) {
  SetupRenderer(kDefaultOutputDeviceId);
  EXPECT_EQ(kDefaultOutputDeviceId,
            mock_sink_->GetOutputDeviceInfo().device_id());
  renderer_proxy_->Start();

  EXPECT_CALL(*mock_sink_.get(), Stop());
  EXPECT_CALL(*source_.get(), RemoveAudioRenderer(renderer_.get()));
  renderer_proxy_->Stop();

  SetupRenderer(kOtherOutputDeviceId);
  EXPECT_EQ(kOtherOutputDeviceId,
            mock_sink_->GetOutputDeviceInfo().device_id());
  renderer_proxy_->Start();

  EXPECT_CALL(*mock_sink_.get(), Stop());
  EXPECT_CALL(*source_.get(), RemoveAudioRenderer(renderer_.get()));
  renderer_proxy_->Stop();
}

TEST_F(WebRtcAudioRendererTest, SwitchOutputDevice) {
  SetupRenderer(kDefaultOutputDeviceId);
  EXPECT_EQ(kDefaultOutputDeviceId,
            mock_sink_->GetOutputDeviceInfo().device_id());
  renderer_proxy_->Start();

  EXPECT_CALL(*mock_sink_.get(), Stop());
  EXPECT_CALL(*this,
              MockCreateAudioRendererSink(AudioDeviceFactory::kSourceWebRtc, _,
                                          _, kOtherOutputDeviceId));
  EXPECT_CALL(*source_.get(), AudioRendererThreadStopped());
  EXPECT_CALL(*source_.get(), SetOutputDeviceForAec(kOtherOutputDeviceId));
  EXPECT_CALL(*this, MockSwitchDeviceCallback(media::OUTPUT_DEVICE_STATUS_OK));
  base::RunLoop loop;
  renderer_proxy_->SwitchOutputDevice(
      kOtherOutputDeviceId,
      base::Bind(&WebRtcAudioRendererTest::SwitchDeviceCallback,
                 base::Unretained(this), &loop));
  loop.Run();
  EXPECT_EQ(kOtherOutputDeviceId,
            mock_sink_->GetOutputDeviceInfo().device_id());

  EXPECT_CALL(*mock_sink_.get(), Stop());
  EXPECT_CALL(*source_.get(), RemoveAudioRenderer(renderer_.get()));
  renderer_proxy_->Stop();
}

TEST_F(WebRtcAudioRendererTest, SwitchOutputDeviceInvalidDevice) {
  SetupRenderer(kDefaultOutputDeviceId);
  EXPECT_EQ(kDefaultOutputDeviceId,
            mock_sink_->GetOutputDeviceInfo().device_id());
  auto original_sink = mock_sink_;
  renderer_proxy_->Start();

  EXPECT_CALL(*this,
              MockCreateAudioRendererSink(AudioDeviceFactory::kSourceWebRtc, _,
                                          _, kInvalidOutputDeviceId));
  EXPECT_CALL(*this, MockSwitchDeviceCallback(
                         media::OUTPUT_DEVICE_STATUS_ERROR_INTERNAL));
  base::RunLoop loop;
  renderer_proxy_->SwitchOutputDevice(
      kInvalidOutputDeviceId,
      base::Bind(&WebRtcAudioRendererTest::SwitchDeviceCallback,
                 base::Unretained(this), &loop));
  loop.Run();
  EXPECT_EQ(kDefaultOutputDeviceId,
            original_sink->GetOutputDeviceInfo().device_id());

  EXPECT_CALL(*original_sink.get(), Stop());
  EXPECT_CALL(*source_.get(), RemoveAudioRenderer(renderer_.get()));
  renderer_proxy_->Stop();
}

TEST_F(WebRtcAudioRendererTest, InitializeWithInvalidDevice) {
  renderer_ = new WebRtcAudioRenderer(message_loop_->task_runner(), stream_, 1,
                                      1, kInvalidOutputDeviceId);

  EXPECT_CALL(*this,
              MockCreateAudioRendererSink(AudioDeviceFactory::kSourceWebRtc, _,
                                          _, kInvalidOutputDeviceId));

  EXPECT_FALSE(renderer_->Initialize(source_.get()));

  renderer_proxy_ = renderer_->CreateSharedAudioRendererProxy(stream_);

  EXPECT_EQ(kInvalidOutputDeviceId,
            mock_sink_->GetOutputDeviceInfo().device_id());
}

TEST_F(WebRtcAudioRendererTest, SwitchOutputDeviceStoppedSource) {
  SetupRenderer(kDefaultOutputDeviceId);
  auto original_sink = mock_sink_;
  renderer_proxy_->Start();

  EXPECT_CALL(*original_sink.get(), Stop());
  EXPECT_CALL(*source_.get(), RemoveAudioRenderer(renderer_.get()));
  EXPECT_CALL(*this, MockSwitchDeviceCallback(
                         media::OUTPUT_DEVICE_STATUS_ERROR_INTERNAL));
  base::RunLoop loop;
  renderer_proxy_->Stop();
  renderer_proxy_->SwitchOutputDevice(
      kInvalidOutputDeviceId,
      base::BindRepeating(&WebRtcAudioRendererTest::SwitchDeviceCallback,
                          base::Unretained(this), &loop));
  loop.Run();
}

}  // namespace content
