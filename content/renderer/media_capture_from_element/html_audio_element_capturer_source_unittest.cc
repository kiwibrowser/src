// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/renderer/media_capture_from_element/html_audio_element_capturer_source.h"
#include "base/memory/weak_ptr.h"
#include "base/run_loop.h"
#include "base/test/scoped_task_environment.h"
#include "base/threading/thread_task_runner_handle.h"
#include "content/public/renderer/media_stream_audio_sink.h"
#include "content/renderer/media/stream/media_stream_audio_track.h"
#include "media/audio/null_audio_sink.h"
#include "media/base/audio_parameters.h"
#include "media/base/fake_audio_render_callback.h"
#include "media/base/media_log.h"
#include "media/blink/webaudiosourceprovider_impl.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "third_party/blink/public/platform/scheduler/test/renderer_scheduler_test_support.h"
#include "third_party/blink/public/platform/web_string.h"
#include "third_party/blink/public/web/web_heap.h"

using ::testing::_;
using ::testing::AllOf;
using ::testing::InSequence;
using ::testing::Mock;
using ::testing::Property;

namespace content {

static const int kNumChannelsForTest = 1;
static const int kBufferDurationMs = 10;

static const int kAudioTrackSampleRate = 48000;
static const int kAudioTrackSamplesPerBuffer =
    kAudioTrackSampleRate * kBufferDurationMs /
    base::Time::kMillisecondsPerSecond;

ACTION_P(RunClosure, closure) {
  closure.Run();
}

//
class MockMediaStreamAudioSink final : public MediaStreamAudioSink {
 public:
  MockMediaStreamAudioSink() : MediaStreamAudioSink() {}
  ~MockMediaStreamAudioSink() override = default;

  MOCK_METHOD1(OnSetFormat, void(const media::AudioParameters& params));
  MOCK_METHOD2(OnData,
               void(const media::AudioBus& audio_bus,
                    base::TimeTicks estimated_capture_time));

  DISALLOW_COPY_AND_ASSIGN(MockMediaStreamAudioSink);
};

// This test needs to bundle together plenty of objects, namely:
// - a WebAudioSourceProviderImpl, which in turn needs an Audio Sink, in this
//  case a NullAudioSink. This is needed to plug HTMLAudioElementCapturerSource
//  and inject audio.
// - a WebMediaStreamSource, that owns the HTMLAudioElementCapturerSource under
//  test, and a WebMediaStreamAudioTrack, that the class under test needs to
//  connect to in order to operate correctly. This class has an inner content
//  MediaStreamAudioTrack.
// - finally, a MockMediaStreamAudioSink to observe captured audio frames, and
//  that plugs into the former MediaStreamAudioTrack.
class HTMLAudioElementCapturerSourceTest : public testing::Test {
 public:
  HTMLAudioElementCapturerSourceTest()
      : fake_callback_(0.1, kAudioTrackSampleRate),
        audio_source_(new media::WebAudioSourceProviderImpl(
            new media::NullAudioSink(
                blink::scheduler::GetSingleThreadTaskRunnerForTesting()),
            &media_log_)) {}

  void SetUp() final {
    SetUpAudioTrack();
    base::RunLoop().RunUntilIdle();
  }

  void TearDown() override {
    blink_audio_track_.Reset();
    blink_audio_source_.Reset();
    blink::WebHeap::CollectAllGarbageForTesting();
  }

  HtmlAudioElementCapturerSource* source() const {
    return static_cast<HtmlAudioElementCapturerSource*>(
        MediaStreamAudioSource::From(blink_audio_source_));
  }

  MediaStreamAudioTrack* track() const {
    return MediaStreamAudioTrack::From(blink_audio_track_);
  }

  int InjectAudio(media::AudioBus* audio_bus) {
    return audio_source_->RenderForTesting(audio_bus);
  }

 protected:
  void SetUpAudioTrack() {
    const media::AudioParameters params(
        media::AudioParameters::AUDIO_PCM_LOW_LATENCY,
        media::GuessChannelLayout(kNumChannelsForTest),
        kAudioTrackSampleRate /* sample_rate */,
        kAudioTrackSamplesPerBuffer /* frames_per_buffer */);
    audio_source_->Initialize(params, &fake_callback_);

    blink_audio_source_.Initialize(blink::WebString::FromUTF8("audio_id"),
                                   blink::WebMediaStreamSource::kTypeAudio,
                                   blink::WebString::FromUTF8("audio_track"),
                                   false /* remote */);
    blink_audio_track_.Initialize(blink_audio_source_.Id(),
                                  blink_audio_source_);

    // |blink_audio_source_| takes ownership of HtmlAudioElementCapturerSource.
    blink_audio_source_.SetExtraData(
        new HtmlAudioElementCapturerSource(audio_source_.get()));
    ASSERT_TRUE(source()->ConnectToTrack(blink_audio_track_));
  }

  const base::test::ScopedTaskEnvironment scoped_task_environment_;

  blink::WebMediaStreamSource blink_audio_source_;
  blink::WebMediaStreamTrack blink_audio_track_;

  media::MediaLog media_log_;
  media::FakeAudioRenderCallback fake_callback_;
  scoped_refptr<media::WebAudioSourceProviderImpl> audio_source_;
};

// Constructs and destructs all objects. This is a non trivial sequence.
TEST_F(HTMLAudioElementCapturerSourceTest, ConstructAndDestruct) {
}

// This test verifies that Audio can be properly captured when injected in the
// WebAudioSourceProviderImpl.
TEST_F(HTMLAudioElementCapturerSourceTest, CaptureAudio) {
  InSequence s;

  base::RunLoop run_loop;
  base::Closure quit_closure = run_loop.QuitClosure();

  MockMediaStreamAudioSink sink;
  track()->AddSink(&sink);
  EXPECT_CALL(sink, OnSetFormat(_)).Times(1);
  EXPECT_CALL(
      sink,
      OnData(AllOf(Property(&media::AudioBus::channels, kNumChannelsForTest),
                   Property(&media::AudioBus::frames,
                            kAudioTrackSamplesPerBuffer)),
             _))
      .Times(1)
      .WillOnce(RunClosure(std::move(quit_closure)));

  std::unique_ptr<media::AudioBus> bus = media::AudioBus::Create(
      kNumChannelsForTest, kAudioTrackSamplesPerBuffer);
  InjectAudio(bus.get());
  run_loop.Run();

  track()->Stop();
  track()->RemoveSink(&sink);
}

// When a new source is created and started, it is stopped in the same task
// when cross-origin data is detected. This test checks that no data is
// delivered in this case.
TEST_F(HTMLAudioElementCapturerSourceTest,
       StartAndStopInSameTaskCapturesZeroFrames) {
  InSequence s;

  // Stop the original track and start a new one so that it can be stopped in
  // in the same task.
  track()->Stop();
  base::RunLoop().RunUntilIdle();
  SetUpAudioTrack();

  MockMediaStreamAudioSink sink;
  track()->AddSink(&sink);
  EXPECT_CALL(
      sink,
      OnData(AllOf(Property(&media::AudioBus::channels, kNumChannelsForTest),
                   Property(&media::AudioBus::frames,
                            kAudioTrackSamplesPerBuffer)),
             _))
      .Times(0);

  std::unique_ptr<media::AudioBus> bus =
      media::AudioBus::Create(kNumChannelsForTest, kAudioTrackSamplesPerBuffer);
  InjectAudio(bus.get());

  track()->Stop();
  base::RunLoop().RunUntilIdle();
  track()->RemoveSink(&sink);
}

}  // namespace content
