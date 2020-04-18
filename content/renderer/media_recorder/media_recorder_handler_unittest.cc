// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <stddef.h>

#include <string>

#include "base/macros.h"
#include "base/run_loop.h"
#include "base/strings/utf_string_conversions.h"
#include "base/test/scoped_task_environment.h"
#include "base/time/time.h"
#include "content/child/child_process.h"
#include "content/renderer/media/stream/mock_media_stream_registry.h"
#include "content/renderer/media_recorder/media_recorder_handler.h"
#include "media/audio/simple_sources.h"
#include "media/base/audio_bus.h"
#include "media/base/video_frame.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "third_party/blink/public/platform/scheduler/test/renderer_scheduler_test_support.h"
#include "third_party/blink/public/platform/web_media_recorder_handler_client.h"
#include "third_party/blink/public/platform/web_string.h"
#include "third_party/blink/public/web/web_heap.h"

using ::testing::_;
using ::testing::AtLeast;
using ::testing::InSequence;
using ::testing::Gt;
using ::testing::Lt;
using ::testing::Mock;
using ::testing::TestWithParam;
using ::testing::ValuesIn;

using blink::WebString;

namespace content {

ACTION_P(RunClosure, closure) {
  closure.Run();
}

static const std::string kTestStreamUrl = "stream_url";
static const std::string kTestVideoTrackId = "video_track_id";
static const std::string kTestAudioTrackId = "audio_track_id";
static const int kTestAudioChannels = 2;
static const int kTestAudioSampleRate = 48000;
static const int kTestAudioBufferDurationMs = 10;
// Opus works with 60ms buffers, so 6 MediaStreamAudioTrack Buffers are needed
// to encode one output buffer.
static const int kRatioOpusToTestAudioBuffers = 6;

struct MediaRecorderTestParams {
  const bool has_video;
  const bool has_audio;
  const char* const mime_type;
  const char* const codecs;
  const bool encoder_supports_alpha;
};

// Array of valid combinations of video/audio/codecs and expected collected
// encoded sizes to use for parameterizing MediaRecorderHandlerTest.
static const MediaRecorderTestParams kMediaRecorderTestParams[] = {
    {true, false, "video/webm", "vp8", true},
    {true, false, "video/webm", "vp9", true},
#if BUILDFLAG(RTC_USE_H264)
    {true, false, "video/webm", "h264", false},
#endif
    {false, true, "audio/webm", "opus", true},
    {false, true, "audio/webm", "", true},  // Should default to opus.
    {false, true, "audio/webm", "pcm", true},
};

class MediaRecorderHandlerTest : public TestWithParam<MediaRecorderTestParams>,
                                 public blink::WebMediaRecorderHandlerClient {
 public:
  MediaRecorderHandlerTest()
      : scoped_task_environment_(
            base::test::ScopedTaskEnvironment::MainThreadType::UI),
        media_recorder_handler_(new MediaRecorderHandler(
            blink::scheduler::GetSingleThreadTaskRunnerForTesting())),
        audio_source_(kTestAudioChannels,
                      440 /* freq */,
                      kTestAudioSampleRate) {
    EXPECT_FALSE(media_recorder_handler_->recording_);

    registry_.Init(kTestStreamUrl);
  }

  ~MediaRecorderHandlerTest() {
    registry_.reset();
    blink::WebHeap::CollectAllGarbageForTesting();
  }

  MOCK_METHOD4(WriteData, void(const char*, size_t, bool, double));
  MOCK_METHOD1(OnError, void(const WebString& message));

  bool recording() const { return media_recorder_handler_->recording_; }
  bool hasVideoRecorders() const {
    return !media_recorder_handler_->video_recorders_.empty();
  }
  bool hasAudioRecorders() const {
    return !media_recorder_handler_->audio_recorders_.empty();
  }

  void OnVideoFrameForTesting(const scoped_refptr<media::VideoFrame>& frame) {
    media_recorder_handler_->OnVideoFrameForTesting(frame,
                                                    base::TimeTicks::Now());
  }
  void OnAudioBusForTesting(const media::AudioBus& audio_bus) {
    media_recorder_handler_->OnAudioBusForTesting(audio_bus,
                                                  base::TimeTicks::Now());
  }
  void SetAudioFormatForTesting(const media::AudioParameters& params) {
    media_recorder_handler_->SetAudioFormatForTesting(params);
  }

  void AddTracks() {
    // Avoid issues with non-parameterized tests by calling this outside of ctr.
    if (GetParam().has_video)
      registry_.AddVideoTrack(kTestVideoTrackId);
    if (GetParam().has_audio)
      registry_.AddAudioTrack(kTestAudioTrackId);
  }

  void ForceOneErrorInWebmMuxer() {
    media_recorder_handler_->webm_muxer_->ForceOneLibWebmErrorForTesting();
  }

  std::unique_ptr<media::AudioBus> NextAudioBus() {
    std::unique_ptr<media::AudioBus> bus(media::AudioBus::Create(
        kTestAudioChannels,
        kTestAudioSampleRate * kTestAudioBufferDurationMs / 1000));
    audio_source_.OnMoreData(base::TimeDelta(), base::TimeTicks::Now(), 0,
                             bus.get());
    return bus;
  }

  // A ChildProcess and a MessageLoopForUI are both needed to fool the Tracks
  // and Sources in |registry_| into believing they are on the right threads.
  const base::test::ScopedTaskEnvironment scoped_task_environment_;
  const ChildProcess child_process_;
  MockMediaStreamRegistry registry_;

  // The Class under test. Needs to be scoped_ptr to force its destruction.
  std::unique_ptr<MediaRecorderHandler> media_recorder_handler_;

  // For generating test AudioBuses
  media::SineWaveAudioSource audio_source_;

 private:
  DISALLOW_COPY_AND_ASSIGN(MediaRecorderHandlerTest);
};

// Checks that canSupportMimeType() works as expected, by sending supported
// combinations and unsupported ones.
TEST_F(MediaRecorderHandlerTest, CanSupportMimeType) {
  const WebString unsupported_mime_type(WebString::FromASCII("video/mpeg"));
  EXPECT_FALSE(media_recorder_handler_->CanSupportMimeType(
      unsupported_mime_type, WebString()));

  const WebString mime_type_video(WebString::FromASCII("video/webm"));
  EXPECT_TRUE(media_recorder_handler_->CanSupportMimeType(mime_type_video,
                                                          WebString()));
  const WebString mime_type_video_uppercase(WebString::FromASCII("video/WEBM"));
  EXPECT_TRUE(media_recorder_handler_->CanSupportMimeType(
      mime_type_video_uppercase, WebString()));
  const WebString example_good_codecs_1(WebString::FromASCII("vp8"));
  EXPECT_TRUE(media_recorder_handler_->CanSupportMimeType(
      mime_type_video, example_good_codecs_1));
  const WebString example_good_codecs_2(WebString::FromASCII("vp9,opus"));
  EXPECT_TRUE(media_recorder_handler_->CanSupportMimeType(
      mime_type_video, example_good_codecs_2));
  const WebString example_good_codecs_3(WebString::FromASCII("VP9,opus"));
  EXPECT_TRUE(media_recorder_handler_->CanSupportMimeType(
      mime_type_video, example_good_codecs_3));
  const WebString example_good_codecs_4(WebString::FromASCII("H264"));
  EXPECT_TRUE(media_recorder_handler_->CanSupportMimeType(
      mime_type_video, example_good_codecs_4));

  const WebString example_unsupported_codecs_1(WebString::FromASCII("daala"));
  EXPECT_FALSE(media_recorder_handler_->CanSupportMimeType(
      mime_type_video, example_unsupported_codecs_1));

  const WebString mime_type_audio(WebString::FromASCII("audio/webm"));
  EXPECT_TRUE(media_recorder_handler_->CanSupportMimeType(mime_type_audio,
                                                          WebString()));
  const WebString example_good_codecs_5(WebString::FromASCII("opus"));
  EXPECT_TRUE(media_recorder_handler_->CanSupportMimeType(
      mime_type_audio, example_good_codecs_5));
  const WebString example_good_codecs_6(WebString::FromASCII("OpUs"));
  EXPECT_TRUE(media_recorder_handler_->CanSupportMimeType(
      mime_type_audio, example_good_codecs_6));
  const WebString example_good_codecs_7(WebString::FromASCII("pcm"));
  EXPECT_TRUE(media_recorder_handler_->CanSupportMimeType(
      mime_type_audio, example_good_codecs_7));

  const WebString example_unsupported_codecs_2(WebString::FromASCII("vorbis"));
  EXPECT_FALSE(media_recorder_handler_->CanSupportMimeType(
      mime_type_audio, example_unsupported_codecs_2));
}

// Checks that the initialization-destruction sequence works fine.
TEST_P(MediaRecorderHandlerTest, InitializeStartStop) {
  AddTracks();
  const WebString mime_type(WebString::FromASCII(GetParam().mime_type));
  const WebString codecs(WebString::FromASCII(GetParam().codecs));
  EXPECT_TRUE(media_recorder_handler_->Initialize(this, registry_.test_stream(),
                                                  mime_type, codecs, 0, 0));
  EXPECT_FALSE(recording());
  EXPECT_FALSE(hasVideoRecorders());
  EXPECT_FALSE(hasAudioRecorders());

  EXPECT_TRUE(media_recorder_handler_->Start(0));
  EXPECT_TRUE(recording());

  EXPECT_TRUE(hasVideoRecorders() || !GetParam().has_video);
  EXPECT_TRUE(hasAudioRecorders() || !GetParam().has_audio);

  media_recorder_handler_->Stop();
  EXPECT_FALSE(recording());
  EXPECT_FALSE(hasVideoRecorders());
  EXPECT_FALSE(hasAudioRecorders());

  // Expect a last call on destruction.
  EXPECT_CALL(*this, WriteData(_, _, true, _)).Times(1);
  media_recorder_handler_.reset();
}

// Sends 2 opaque frames and 1 transparent frame and expects them as WebM
// contained encoded data in writeData().
TEST_P(MediaRecorderHandlerTest, EncodeVideoFrames) {
  // Video-only test.
  if (GetParam().has_audio)
    return;

  AddTracks();

  const WebString mime_type(WebString::FromASCII(GetParam().mime_type));
  const WebString codecs(WebString::FromASCII(GetParam().codecs));
  EXPECT_TRUE(media_recorder_handler_->Initialize(this, registry_.test_stream(),
                                                  mime_type, codecs, 0, 0));
  EXPECT_TRUE(media_recorder_handler_->Start(0));

  InSequence s;
  const scoped_refptr<media::VideoFrame> video_frame =
      media::VideoFrame::CreateBlackFrame(gfx::Size(160, 80));

  {
    const size_t kEncodedSizeThreshold = 16;
    base::RunLoop run_loop;
    base::Closure quit_closure = run_loop.QuitClosure();
    // writeData() is pinged a number of times as the WebM header is written;
    // the last time it is called it has the encoded data.
    EXPECT_CALL(*this, WriteData(_, Lt(kEncodedSizeThreshold), _, _))
        .Times(AtLeast(1));
    EXPECT_CALL(*this, WriteData(_, Gt(kEncodedSizeThreshold), _, _))
        .Times(1)
        .WillOnce(RunClosure(std::move(quit_closure)));

    OnVideoFrameForTesting(video_frame);
    run_loop.Run();
  }
  Mock::VerifyAndClearExpectations(this);

  {
    const size_t kEncodedSizeThreshold = 12;
    base::RunLoop run_loop;
    base::Closure quit_closure = run_loop.QuitClosure();
    // The second time around writeData() is called a number of times to write
    // the WebM frame header, and then is pinged with the encoded data.
    EXPECT_CALL(*this, WriteData(_, Lt(kEncodedSizeThreshold), _, _))
        .Times(AtLeast(1));
    EXPECT_CALL(*this, WriteData(_, Gt(kEncodedSizeThreshold), _, _))
        .Times(1)
        .WillOnce(RunClosure(std::move(quit_closure)));

    OnVideoFrameForTesting(video_frame);
    run_loop.Run();
  }
  Mock::VerifyAndClearExpectations(this);

  {
    const scoped_refptr<media::VideoFrame> alpha_frame =
        media::VideoFrame::CreateTransparentFrame(gfx::Size(160, 80));
    const size_t kEncodedSizeThreshold = 16;
    EXPECT_EQ(4u, media::VideoFrame::NumPlanes(alpha_frame->format()));
    base::RunLoop run_loop;
    base::Closure quit_closure = run_loop.QuitClosure();
    // The second time around writeData() is called a number of times to write
    // the WebM frame header, and then is pinged with the encoded data.
    EXPECT_CALL(*this, WriteData(_, Lt(kEncodedSizeThreshold), _, _))
        .Times(AtLeast(1));
    EXPECT_CALL(*this, WriteData(_, Gt(kEncodedSizeThreshold), _, _))
        .Times(1)
        .WillOnce(RunClosure(quit_closure));
    if (GetParam().encoder_supports_alpha) {
      EXPECT_CALL(*this, WriteData(_, Lt(kEncodedSizeThreshold), _, _))
          .Times(AtLeast(1));
      EXPECT_CALL(*this, WriteData(_, Gt(kEncodedSizeThreshold), _, _))
          .Times(1)
          .WillOnce(RunClosure(std::move(quit_closure)));
    }

    OnVideoFrameForTesting(alpha_frame);
    run_loop.Run();
  }

  media_recorder_handler_->Stop();

  // Expect a last call on destruction, with size 0 and |lastInSlice| true.
  EXPECT_CALL(*this, WriteData(nullptr, 0, true, _)).Times(1);
  media_recorder_handler_.reset();
}

INSTANTIATE_TEST_CASE_P(,
                        MediaRecorderHandlerTest,
                        ValuesIn(kMediaRecorderTestParams));

// Sends 2 frames and expect them as WebM (or MKV) contained encoded audio data
// in writeData().
TEST_P(MediaRecorderHandlerTest, OpusEncodeAudioFrames) {
  // Audio-only test.
  if (GetParam().has_video)
    return;

  AddTracks();

  const WebString mime_type(WebString::FromASCII(GetParam().mime_type));
  const WebString codecs(WebString::FromASCII(GetParam().codecs));
  EXPECT_TRUE(media_recorder_handler_->Initialize(this, registry_.test_stream(),
                                                  mime_type, codecs, 0, 0));
  EXPECT_TRUE(media_recorder_handler_->Start(0));

  InSequence s;
  const std::unique_ptr<media::AudioBus> audio_bus1 = NextAudioBus();
  const std::unique_ptr<media::AudioBus> audio_bus2 = NextAudioBus();

  media::AudioParameters params(
      media::AudioParameters::AUDIO_PCM_LINEAR, media::CHANNEL_LAYOUT_STEREO,
      kTestAudioSampleRate,
      kTestAudioSampleRate * kTestAudioBufferDurationMs / 1000);
  SetAudioFormatForTesting(params);

  const size_t kEncodedSizeThreshold = 24;
  {
    base::RunLoop run_loop;
    base::Closure quit_closure = run_loop.QuitClosure();
    // writeData() is pinged a number of times as the WebM header is written;
    // the last time it is called it has the encoded data.
    EXPECT_CALL(*this, WriteData(_, Lt(kEncodedSizeThreshold), _, _))
        .Times(AtLeast(1));
    EXPECT_CALL(*this, WriteData(_, Gt(kEncodedSizeThreshold), _, _))
        .Times(1)
        .WillOnce(RunClosure(std::move(quit_closure)));

    for (int i = 0; i < kRatioOpusToTestAudioBuffers; ++i)
      OnAudioBusForTesting(*audio_bus1);
    run_loop.Run();
  }
  Mock::VerifyAndClearExpectations(this);

  {
    base::RunLoop run_loop;
    base::Closure quit_closure = run_loop.QuitClosure();
    // The second time around writeData() is called a number of times to write
    // the WebM frame header, and then is pinged with the encoded data.
    EXPECT_CALL(*this, WriteData(_, Lt(kEncodedSizeThreshold), _, _))
        .Times(AtLeast(1));
    EXPECT_CALL(*this, WriteData(_, Gt(kEncodedSizeThreshold), _, _))
        .Times(1)
        .WillOnce(RunClosure(std::move(quit_closure)));

    for (int i = 0; i < kRatioOpusToTestAudioBuffers; ++i)
      OnAudioBusForTesting(*audio_bus2);
    run_loop.Run();
  }

  media_recorder_handler_->Stop();

  // Expect a last call on destruction, with size 0 and |lastInSlice| true.
  EXPECT_CALL(*this, WriteData(nullptr, 0, true, _)).Times(1);
  media_recorder_handler_.reset();
}

// Starts up recording and forces a WebmMuxer's libwebm error.
TEST_P(MediaRecorderHandlerTest, WebmMuxerErrorWhileEncoding) {
  // Video-only test: Audio would be very similar.
  if (GetParam().has_audio)
    return;

  AddTracks();

  const WebString mime_type(WebString::FromASCII(GetParam().mime_type));
  const WebString codecs(WebString::FromASCII(GetParam().codecs));
  EXPECT_TRUE(media_recorder_handler_->Initialize(this, registry_.test_stream(),
                                                  mime_type, codecs, 0, 0));
  EXPECT_TRUE(media_recorder_handler_->Start(0));

  InSequence s;
  const scoped_refptr<media::VideoFrame> video_frame =
      media::VideoFrame::CreateBlackFrame(gfx::Size(160, 80));

  {
    const size_t kEncodedSizeThreshold = 16;
    base::RunLoop run_loop;
    base::Closure quit_closure = run_loop.QuitClosure();
    EXPECT_CALL(*this, WriteData(_, _, _, _)).Times(AtLeast(1));
    EXPECT_CALL(*this, WriteData(_, Gt(kEncodedSizeThreshold), _, _))
        .Times(1)
        .WillOnce(RunClosure(std::move(quit_closure)));

    OnVideoFrameForTesting(video_frame);
    run_loop.Run();
  }

  ForceOneErrorInWebmMuxer();

  {
    base::RunLoop run_loop;
    base::Closure quit_closure = run_loop.QuitClosure();
    EXPECT_CALL(*this, WriteData(_, _, _, _)).Times(0);
    EXPECT_CALL(*this, OnError(_))
        .Times(1)
        .WillOnce(RunClosure(std::move(quit_closure)));

    OnVideoFrameForTesting(video_frame);
    run_loop.Run();
  }


  // Expect a last call on destruction, with size 0 and |lastInSlice| true.
  EXPECT_CALL(*this, WriteData(nullptr, 0, true, _)).Times(1);
  media_recorder_handler_.reset();
}

}  // namespace content
