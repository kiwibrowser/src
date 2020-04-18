// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chromecast/media/audio/cast_audio_output_stream.h"

#include <stdint.h>

#include <memory>
#include <utility>

#include "base/memory/ref_counted.h"
#include "base/run_loop.h"
#include "base/test/scoped_task_environment.h"
#include "base/time/time.h"
#include "chromecast/media/audio/cast_audio_manager.h"
#include "chromecast/media/audio/cast_audio_mixer.h"
#include "chromecast/media/cma/backend/cma_backend.h"
#include "chromecast/media/cma/base/decoder_buffer_base.h"
#include "chromecast/media/cma/test/mock_cma_backend_factory.h"
#include "chromecast/public/task_runner.h"
#include "media/audio/mock_audio_source_callback.h"
#include "media/audio/test_audio_thread.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

using testing::_;
using testing::Invoke;
using testing::NiceMock;

namespace chromecast {
namespace media {
namespace {
const char kDefaultDeviceId[] = "";
const int64_t kDelayUs = 123;
const int64_t kDelayTimestampUs = 123456789;

int OnMoreData(base::TimeDelta /* delay */,
               base::TimeTicks /* delay_timestamp */,
               int /* prior_frames_skipped */,
               ::media::AudioBus* dest) {
  dest->Zero();
  return dest->frames();
}

class NotifyPushBufferCompleteTask : public chromecast::TaskRunner::Task {
 public:
  explicit NotifyPushBufferCompleteTask(CmaBackend::Decoder::Delegate* delegate)
      : delegate_(delegate) {}
  ~NotifyPushBufferCompleteTask() override = default;
  void Run() override {
    delegate_->OnPushBufferComplete(CmaBackend::BufferStatus::kBufferSuccess);
  }

 private:
  CmaBackend::Decoder::Delegate* const delegate_;
};

class FakeAudioDecoder : public CmaBackend::AudioDecoder {
 public:
  enum PipelineStatus {
    PIPELINE_STATUS_OK,
    PIPELINE_STATUS_BUSY,
    PIPELINE_STATUS_ERROR,
    PIPELINE_STATUS_ASYNC_ERROR,
  };

  explicit FakeAudioDecoder(const MediaPipelineDeviceParams& params)
      : params_(params),
        volume_(1.0f),
        pipeline_status_(PIPELINE_STATUS_OK),
        pending_push_(false),
        pushed_buffer_count_(0),
        delegate_(nullptr) {}
  ~FakeAudioDecoder() override {}

  // CmaBackend::AudioDecoder implementation:
  void SetDelegate(Delegate* delegate) override {
    DCHECK(delegate);
    delegate_ = delegate;
  }
  BufferStatus PushBuffer(scoped_refptr<DecoderBufferBase> buffer) override {
    last_buffer_ = std::move(buffer);
    ++pushed_buffer_count_;

    switch (pipeline_status_) {
      case PIPELINE_STATUS_OK:
        return CmaBackend::BufferStatus::kBufferSuccess;
      case PIPELINE_STATUS_BUSY:
        pending_push_ = true;
        return CmaBackend::BufferStatus::kBufferPending;
      case PIPELINE_STATUS_ERROR:
        return CmaBackend::BufferStatus::kBufferFailed;
      case PIPELINE_STATUS_ASYNC_ERROR:
        delegate_->OnDecoderError();
        return CmaBackend::BufferStatus::kBufferSuccess;
      default:
        NOTREACHED();
        return CmaBackend::BufferStatus::kBufferFailed;
    }
  }
  void GetStatistics(Statistics* statistics) override {}
  bool SetConfig(const AudioConfig& config) override {
    config_ = config;
    return true;
  }
  bool SetVolume(float volume) override {
    volume_ = volume;
    return true;
  }
  RenderingDelay GetRenderingDelay() override { return rendering_delay_; }
  bool RequiresDecryption() override { return false; }

  const AudioConfig& config() const { return config_; }
  float volume() const { return volume_; }
  void set_pipeline_status(PipelineStatus status) {
    if (status == PIPELINE_STATUS_OK && pending_push_) {
      pending_push_ = false;
      params_.task_runner->PostTask(new NotifyPushBufferCompleteTask(delegate_),
                                    0);
    }
    pipeline_status_ = status;
  }
  void set_rendering_delay(RenderingDelay rendering_delay) {
    rendering_delay_ = rendering_delay;
  }
  unsigned pushed_buffer_count() const { return pushed_buffer_count_; }
  const DecoderBufferBase* last_buffer() { return last_buffer_.get(); }

 private:
  const MediaPipelineDeviceParams params_;
  AudioConfig config_;
  float volume_;

  PipelineStatus pipeline_status_;
  bool pending_push_;
  int pushed_buffer_count_;
  scoped_refptr<DecoderBufferBase> last_buffer_;
  Delegate* delegate_;
  RenderingDelay rendering_delay_;
};

class FakeCmaBackend : public CmaBackend {
 public:
  enum State { kStateStopped, kStateRunning, kStatePaused };

  explicit FakeCmaBackend(const MediaPipelineDeviceParams& params)
      : params_(params), state_(kStateStopped), audio_decoder_(nullptr) {}
  ~FakeCmaBackend() override {}

  // CmaBackend implementation:
  AudioDecoder* CreateAudioDecoder() override {
    DCHECK(!audio_decoder_);
    audio_decoder_ = std::make_unique<FakeAudioDecoder>(params_);
    return audio_decoder_.get();
  }
  VideoDecoder* CreateVideoDecoder() override {
    NOTREACHED();
    return nullptr;
  }

  bool Initialize() override { return true; }
  bool Start(int64_t start_pts) override {
    EXPECT_EQ(kStateStopped, state_);
    state_ = kStateRunning;
    return true;
  }
  void Stop() override {
    EXPECT_TRUE(state_ == kStateRunning || state_ == kStatePaused);
    state_ = kStateStopped;
  }
  bool Pause() override {
    EXPECT_EQ(kStateRunning, state_);
    state_ = kStatePaused;
    return true;
  }
  bool Resume() override {
    EXPECT_EQ(kStatePaused, state_);
    state_ = kStateRunning;
    return true;
  }
  int64_t GetCurrentPts() override { return 0; }
  bool SetPlaybackRate(float rate) override { return true; }

  State state() const { return state_; }
  FakeAudioDecoder* decoder() const { return audio_decoder_.get(); }

 private:
  const MediaPipelineDeviceParams params_;
  State state_;
  std::unique_ptr<FakeAudioDecoder> audio_decoder_;
};

class CastAudioOutputStreamTest : public ::testing::Test {
 public:
  CastAudioOutputStreamTest()
      : media_thread_("CastMediaThread"),
        media_pipeline_backend_(nullptr),
        format_(::media::AudioParameters::AUDIO_PCM_LINEAR),
        channel_layout_(::media::CHANNEL_LAYOUT_MONO),
        sample_rate_(::media::AudioParameters::kAudioCDSampleRate),
        frames_per_buffer_(256) {}
  ~CastAudioOutputStreamTest() override {}

 protected:
  void SetUp() override {
    CHECK(media_thread_.Start());
    auto backend_factory = std::make_unique<NiceMock<MockCmaBackendFactory>>();
    ON_CALL(*backend_factory, CreateBackend(_))
        .WillByDefault(Invoke([this](const MediaPipelineDeviceParams& params) {
          auto backend = std::make_unique<FakeCmaBackend>(params);
          media_pipeline_backend_ = backend.get();
          return backend;
        }));
    audio_manager_ = std::make_unique<CastAudioManager>(
        std::make_unique<::media::TestAudioThread>(), nullptr,
        std::move(backend_factory), media_thread_.task_runner(), false);
  }

  void TearDown() override { audio_manager_->Shutdown(); }

  ::media::AudioParameters GetAudioParams() {
    return ::media::AudioParameters(format_, channel_layout_, sample_rate_,
                                    frames_per_buffer_);
  }

  FakeCmaBackend* GetBackend() { return media_pipeline_backend_; }

  FakeAudioDecoder* GetAudio() {
    FakeCmaBackend* backend = GetBackend();
    return (backend ? backend->decoder() : nullptr);
  }

  ::media::AudioOutputStream* CreateStream() {
    return audio_manager_->MakeAudioOutputStream(
        GetAudioParams(), kDefaultDeviceId,
        ::media::AudioManager::LogCallback());
  }

  // Runs the messsage loop for duration equivalent to the given number of
  // audio |frames|.
  void RunMessageLoopFor(int frames) {
    ::media::AudioParameters audio_params = GetAudioParams();
    base::TimeDelta duration = audio_params.GetBufferDuration() * frames;

    base::RunLoop run_loop;
    scoped_task_environment_.GetMainThreadTaskRunner()->PostDelayedTask(
        FROM_HERE, run_loop.QuitClosure(), duration);
    run_loop.Run();
  }

  base::test::ScopedTaskEnvironment scoped_task_environment_;
  base::Thread media_thread_;
  std::unique_ptr<CastAudioManager> audio_manager_;
  FakeCmaBackend* media_pipeline_backend_;
  // AudioParameters used to create AudioOutputStream.
  // Tests can modify these parameters before calling CreateStream.
  ::media::AudioParameters::Format format_;
  ::media::ChannelLayout channel_layout_;
  int sample_rate_;
  int frames_per_buffer_;
};

TEST_F(CastAudioOutputStreamTest, Format) {
  ::media::AudioParameters::Format format[] = {
      ::media::AudioParameters::AUDIO_PCM_LINEAR,
      ::media::AudioParameters::AUDIO_PCM_LOW_LATENCY};
  for (size_t i = 0; i < arraysize(format); ++i) {
    format_ = format[i];
    ::media::AudioOutputStream* stream = CreateStream();
    ASSERT_TRUE(stream);
    EXPECT_TRUE(stream->Open());

    FakeAudioDecoder* audio_decoder = GetAudio();
    ASSERT_TRUE(audio_decoder);
    const AudioConfig& audio_config = audio_decoder->config();
    EXPECT_EQ(kCodecPCM, audio_config.codec);
    EXPECT_EQ(kSampleFormatS16, audio_config.sample_format);
    EXPECT_FALSE(audio_config.encryption_scheme.is_encrypted());

    stream->Close();
  }
}

TEST_F(CastAudioOutputStreamTest, ChannelLayout) {
  ::media::ChannelLayout layout[] = {::media::CHANNEL_LAYOUT_MONO,
                                     ::media::CHANNEL_LAYOUT_STEREO};
  for (size_t i = 0; i < arraysize(layout); ++i) {
    channel_layout_ = layout[i];
    ::media::AudioOutputStream* stream = CreateStream();
    ASSERT_TRUE(stream);
    EXPECT_TRUE(stream->Open());

    FakeAudioDecoder* audio_decoder = GetAudio();
    ASSERT_TRUE(audio_decoder);
    const AudioConfig& audio_config = audio_decoder->config();
    EXPECT_EQ(::media::ChannelLayoutToChannelCount(channel_layout_),
              audio_config.channel_number);

    stream->Close();
  }
}

TEST_F(CastAudioOutputStreamTest, SampleRate) {
  sample_rate_ = ::media::AudioParameters::kAudioCDSampleRate;
  ::media::AudioOutputStream* stream = CreateStream();
  ASSERT_TRUE(stream);
  EXPECT_TRUE(stream->Open());

  FakeAudioDecoder* audio_decoder = GetAudio();
  ASSERT_TRUE(audio_decoder);
  const AudioConfig& audio_config = audio_decoder->config();
  EXPECT_EQ(sample_rate_, audio_config.samples_per_second);

  stream->Close();
}

TEST_F(CastAudioOutputStreamTest, DeviceState) {
  ::media::AudioOutputStream* stream = CreateStream();
  ASSERT_TRUE(stream);

  EXPECT_TRUE(stream->Open());
  FakeAudioDecoder* audio_decoder = GetAudio();
  ASSERT_TRUE(audio_decoder);
  FakeCmaBackend* backend = GetBackend();
  ASSERT_TRUE(backend);
  EXPECT_EQ(FakeCmaBackend::kStateStopped, backend->state());

  ::media::MockAudioSourceCallback source_callback;
  EXPECT_CALL(source_callback, OnMoreData(_, _, _, _))
      .WillRepeatedly(Invoke(OnMoreData));
  stream->Start(&source_callback);
  media_thread_.FlushForTesting();
  EXPECT_EQ(FakeCmaBackend::kStateRunning, backend->state());

  stream->Stop();
  media_thread_.FlushForTesting();
  EXPECT_EQ(FakeCmaBackend::kStatePaused, backend->state());

  stream->Close();
}

TEST_F(CastAudioOutputStreamTest, PushFrame) {
  ::media::AudioOutputStream* stream = CreateStream();
  ASSERT_TRUE(stream);
  EXPECT_TRUE(stream->Open());

  FakeAudioDecoder* audio_decoder = GetAudio();
  ASSERT_TRUE(audio_decoder);
  // Verify initial state.
  EXPECT_EQ(0u, audio_decoder->pushed_buffer_count());
  EXPECT_FALSE(audio_decoder->last_buffer());

  ::media::MockAudioSourceCallback source_callback;
  EXPECT_CALL(source_callback, OnMoreData(_, _, _, _))
      .WillRepeatedly(Invoke(OnMoreData));
  // No error must be reported to source callback.
  EXPECT_CALL(source_callback, OnError()).Times(0);
  stream->Start(&source_callback);
  RunMessageLoopFor(2);
  stream->Stop();

  // Verify that the stream pushed frames to the backend.
  EXPECT_LT(0u, audio_decoder->pushed_buffer_count());
  EXPECT_TRUE(audio_decoder->last_buffer());

  // Verify decoder buffer.
  ::media::AudioParameters audio_params = GetAudioParams();
  const size_t expected_frame_size =
      audio_params.GetBytesPerBuffer(::media::kSampleFormatS16);
  const DecoderBufferBase* buffer = audio_decoder->last_buffer();
  EXPECT_TRUE(buffer->data());
  EXPECT_EQ(expected_frame_size, buffer->data_size());
  EXPECT_FALSE(buffer->decrypt_config());  // Null because of raw audio.
  EXPECT_FALSE(buffer->end_of_stream());

  stream->Close();
}

TEST_F(CastAudioOutputStreamTest, DeviceBusy) {
  ::media::AudioOutputStream* stream = CreateStream();
  ASSERT_TRUE(stream);
  EXPECT_TRUE(stream->Open());

  FakeAudioDecoder* audio_decoder = GetAudio();
  ASSERT_TRUE(audio_decoder);
  audio_decoder->set_pipeline_status(FakeAudioDecoder::PIPELINE_STATUS_BUSY);

  ::media::MockAudioSourceCallback source_callback;
  EXPECT_CALL(source_callback, OnMoreData(_, _, _, _))
      .WillRepeatedly(Invoke(OnMoreData));
  // No error must be reported to source callback.
  EXPECT_CALL(source_callback, OnError()).Times(0);
  stream->Start(&source_callback);
  RunMessageLoopFor(5);
  // Make sure that one frame was pushed.
  EXPECT_EQ(1u, audio_decoder->pushed_buffer_count());

  // Sleep for a few frames and verify that more frames were not pushed
  // because the backend device was busy.
  RunMessageLoopFor(5);
  EXPECT_EQ(1u, audio_decoder->pushed_buffer_count());

  // Unblock the pipeline and verify that PushFrame resumes.
  audio_decoder->set_pipeline_status(FakeAudioDecoder::PIPELINE_STATUS_OK);
  RunMessageLoopFor(5);
  EXPECT_LT(1u, audio_decoder->pushed_buffer_count());

  stream->Stop();
  stream->Close();
}

TEST_F(CastAudioOutputStreamTest, DeviceError) {
  ::media::AudioOutputStream* stream = CreateStream();
  ASSERT_TRUE(stream);
  EXPECT_TRUE(stream->Open());

  FakeAudioDecoder* audio_decoder = GetAudio();
  ASSERT_TRUE(audio_decoder);
  audio_decoder->set_pipeline_status(FakeAudioDecoder::PIPELINE_STATUS_ERROR);

  ::media::MockAudioSourceCallback source_callback;
  EXPECT_CALL(source_callback, OnMoreData(_, _, _, _))
      .WillRepeatedly(Invoke(OnMoreData));
  // AudioOutputStream must report error to source callback.
  EXPECT_CALL(source_callback, OnError());
  stream->Start(&source_callback);
  RunMessageLoopFor(2);
  // Make sure that AudioOutputStream attempted to push the initial frame.
  EXPECT_LT(0u, audio_decoder->pushed_buffer_count());

  stream->Stop();
  stream->Close();
}

TEST_F(CastAudioOutputStreamTest, DeviceAsyncError) {
  ::media::AudioOutputStream* stream = CreateStream();
  ASSERT_TRUE(stream);
  EXPECT_TRUE(stream->Open());

  FakeAudioDecoder* audio_decoder = GetAudio();
  ASSERT_TRUE(audio_decoder);
  audio_decoder->set_pipeline_status(
      FakeAudioDecoder::PIPELINE_STATUS_ASYNC_ERROR);

  ::media::MockAudioSourceCallback source_callback;
  EXPECT_CALL(source_callback, OnMoreData(_, _, _, _))
      .WillRepeatedly(Invoke(OnMoreData));
  // AudioOutputStream must report error to source callback.
  EXPECT_CALL(source_callback, OnError());
  stream->Start(&source_callback);
  RunMessageLoopFor(5);

  // Make sure that one frame was pushed.
  EXPECT_EQ(1u, audio_decoder->pushed_buffer_count());

  stream->Stop();
  stream->Close();
}

TEST_F(CastAudioOutputStreamTest, Volume) {
  ::media::AudioOutputStream* stream = CreateStream();
  ASSERT_TRUE(stream);
  ASSERT_TRUE(stream->Open());
  FakeAudioDecoder* audio_decoder = GetAudio();
  ASSERT_TRUE(audio_decoder);

  double volume = 0.0;
  stream->GetVolume(&volume);
  EXPECT_EQ(1.0, volume);
  EXPECT_EQ(1.0f, audio_decoder->volume());

  stream->SetVolume(0.5);
  media_thread_.FlushForTesting();
  stream->GetVolume(&volume);
  EXPECT_EQ(0.5, volume);
  EXPECT_EQ(0.5f, audio_decoder->volume());

  stream->Close();
}

TEST_F(CastAudioOutputStreamTest, StartStopStart) {
  ::media::AudioOutputStream* stream = CreateStream();
  ASSERT_TRUE(stream);
  ASSERT_TRUE(stream->Open());

  ::media::MockAudioSourceCallback source_callback;
  EXPECT_CALL(source_callback, OnMoreData(_, _, _, _))
      .WillRepeatedly(Invoke(OnMoreData));
  stream->Start(&source_callback);
  RunMessageLoopFor(2);
  stream->Stop();
  stream->Start(&source_callback);
  RunMessageLoopFor(2);

  FakeAudioDecoder* audio_device = GetAudio();
  EXPECT_TRUE(audio_device);
  EXPECT_EQ(FakeCmaBackend::kStateRunning, GetBackend()->state());

  stream->Stop();
  stream->Close();
}

TEST_F(CastAudioOutputStreamTest, CloseWithoutStart) {
  ::media::AudioOutputStream* stream = CreateStream();
  ASSERT_TRUE(stream);
  ASSERT_TRUE(stream->Open());
  stream->Close();
}

TEST_F(CastAudioOutputStreamTest, AudioDelay) {
  ::media::AudioOutputStream* stream = CreateStream();
  ASSERT_TRUE(stream);
  ASSERT_TRUE(stream->Open());

  FakeAudioDecoder* audio_decoder = GetAudio();
  ASSERT_TRUE(audio_decoder);
  audio_decoder->set_rendering_delay(
      CmaBackend::AudioDecoder::RenderingDelay(kDelayUs, kDelayTimestampUs));

  ::media::MockAudioSourceCallback source_callback;
  const base::TimeDelta delay(base::TimeDelta::FromMicroseconds(kDelayUs));
  const base::TimeTicks delay_timestamp(
      base::TimeTicks() + base::TimeDelta::FromMicroseconds(kDelayTimestampUs));
  EXPECT_CALL(source_callback, OnMoreData(delay, delay_timestamp, _, _))
      .WillRepeatedly(Invoke(OnMoreData));

  stream->Start(&source_callback);
  RunMessageLoopFor(2);
  stream->Stop();
  stream->Close();
}

}  // namespace
}  // namespace media
}  // namespace chromecast
