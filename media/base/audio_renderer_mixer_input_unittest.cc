// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <stddef.h>
#include <memory>

#include "base/bind.h"
#include "base/bind_helpers.h"
#include "base/macros.h"
#include "base/run_loop.h"
#include "base/test/scoped_task_environment.h"
#include "media/base/audio_latency.h"
#include "media/base/audio_renderer_mixer.h"
#include "media/base/audio_renderer_mixer_input.h"
#include "media/base/audio_renderer_mixer_pool.h"
#include "media/base/fake_audio_render_callback.h"
#include "media/base/mock_audio_renderer_sink.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace {
void LogUma(int value) {}
}

namespace media {

static const int kSampleRate = 48000;
static const int kBufferSize = 8192;
static const int kRenderFrameId = 42;
static const ChannelLayout kChannelLayout = CHANNEL_LAYOUT_STEREO;
static const char kDefaultDeviceId[] = "default";
static const char kAnotherDeviceId[] = "another";
static const char kUnauthorizedDeviceId[] = "unauthorized";
static const char kNonexistentDeviceId[] = "nonexistent";

class AudioRendererMixerInputTest : public testing::Test,
                                    AudioRendererMixerPool {
 public:
  AudioRendererMixerInputTest() {
    audio_parameters_ =
        AudioParameters(AudioParameters::AUDIO_PCM_LINEAR, kChannelLayout,
                        kSampleRate, kBufferSize);

    CreateMixerInput(kDefaultDeviceId);
    fake_callback_.reset(new FakeAudioRenderCallback(0, kSampleRate));
    audio_bus_ = AudioBus::Create(audio_parameters_);
  }

  void CreateMixerInput(const std::string& device_id) {
    mixer_input_ = new AudioRendererMixerInput(this, kRenderFrameId, device_id,

                                               AudioLatency::LATENCY_PLAYBACK);
  }

  AudioRendererMixer* GetMixer(int owner_id,
                               const AudioParameters& params,
                               AudioLatency::LatencyType latency,
                               const std::string& device_id,
                               OutputDeviceStatus* device_status) override {
    EXPECT_TRUE(params.IsValid());
    if (device_id == kNonexistentDeviceId) {
      if (device_status)
        *device_status = OUTPUT_DEVICE_STATUS_ERROR_NOT_FOUND;
      return nullptr;
    }

    if (device_id == kUnauthorizedDeviceId) {
      if (device_status)
        *device_status = OUTPUT_DEVICE_STATUS_ERROR_NOT_AUTHORIZED;
      return nullptr;
    }

    size_t idx = (device_id == kDefaultDeviceId) ? 0 : 1;
    if (!mixers_[idx]) {
      sinks_[idx] =
          new MockAudioRendererSink(device_id, OUTPUT_DEVICE_STATUS_OK);
      EXPECT_CALL(*(sinks_[idx].get()), Start());
      EXPECT_CALL(*(sinks_[idx].get()), Stop());

      mixers_[idx].reset(new AudioRendererMixer(
          audio_parameters_, sinks_[idx].get(), base::Bind(&LogUma)));
    }
    EXPECT_CALL(*this, ReturnMixer(mixers_[idx].get()));

    if (device_status)
      *device_status = OUTPUT_DEVICE_STATUS_OK;
    return mixers_[idx].get();
  }

  double ProvideInput() {
    return mixer_input_->ProvideInput(audio_bus_.get(), 0);
  }

  OutputDeviceInfo GetOutputDeviceInfo(int source_render_frame_id,
                                       int session_id,
                                       const std::string& device_id) override {
    OutputDeviceStatus status = OUTPUT_DEVICE_STATUS_OK;
    if (device_id == kNonexistentDeviceId)
      status = OUTPUT_DEVICE_STATUS_ERROR_NOT_FOUND;
    else if (device_id == kUnauthorizedDeviceId)
      status = OUTPUT_DEVICE_STATUS_ERROR_NOT_AUTHORIZED;

    GetOutputDeviceInfoCalled(device_id);
    return OutputDeviceInfo(device_id, status,
                            AudioParameters::UnavailableDeviceParams());
  }

  MOCK_METHOD1(GetOutputDeviceInfoCalled, void(const std::string&));

  MOCK_METHOD1(ReturnMixer, void(AudioRendererMixer*));

  MOCK_METHOD1(SwitchCallbackCalled, void(OutputDeviceStatus));

  void SwitchCallback(base::RunLoop* loop, OutputDeviceStatus result) {
    SwitchCallbackCalled(result);
    loop->Quit();
  }

  AudioRendererMixer* GetInputMixer() { return mixer_input_->mixer_; }

 protected:
  ~AudioRendererMixerInputTest() override = default;

  base::test::ScopedTaskEnvironment scoped_task_environment_;
  AudioParameters audio_parameters_;
  scoped_refptr<MockAudioRendererSink> sinks_[2];
  std::unique_ptr<AudioRendererMixer> mixers_[2];
  scoped_refptr<AudioRendererMixerInput> mixer_input_;
  std::unique_ptr<FakeAudioRenderCallback> fake_callback_;
  std::unique_ptr<AudioBus> audio_bus_;

 private:
  DISALLOW_COPY_AND_ASSIGN(AudioRendererMixerInputTest);
};

TEST_F(AudioRendererMixerInputTest, GetDeviceInfo) {
  EXPECT_CALL(*this, GetOutputDeviceInfoCalled(kDefaultDeviceId))
      .Times(testing::Exactly(1));

  mixer_input_->Initialize(audio_parameters_, fake_callback_.get());

  // Calling GetOutputDeviceInfo() should result in the mock call, since there
  // is no mixer created yet for mixer input.
  mixer_input_->GetOutputDeviceInfo();
  mixer_input_->Start();

  // This call should be directed to the mixer and should not result in the mock
  // call.
  EXPECT_STREQ(kDefaultDeviceId,
               mixer_input_->GetOutputDeviceInfo().device_id().c_str());

  mixer_input_->Stop();
}

// Test that getting and setting the volume work as expected.  The volume is
// returned from ProvideInput() only when playing.
TEST_F(AudioRendererMixerInputTest, GetSetVolume) {
  mixer_input_->Initialize(audio_parameters_, fake_callback_.get());
  mixer_input_->Start();
  mixer_input_->Play();

  // Starting volume should be 1.0.
  EXPECT_DOUBLE_EQ(ProvideInput(), 1);

  const double kVolume = 0.5;
  EXPECT_TRUE(mixer_input_->SetVolume(kVolume));
  EXPECT_DOUBLE_EQ(ProvideInput(), kVolume);

  mixer_input_->Stop();
}

// Test Start()/Play()/Pause()/Stop()/playing() all work as expected.  Also
// implicitly tests that AddMixerInput() and RemoveMixerInput() work without
// crashing; functional tests for these methods are in AudioRendererMixerTest.
TEST_F(AudioRendererMixerInputTest, StartPlayPauseStopPlaying) {
  mixer_input_->Initialize(audio_parameters_, fake_callback_.get());
  mixer_input_->Start();
  mixer_input_->Play();
  EXPECT_DOUBLE_EQ(ProvideInput(), 1);
  mixer_input_->Pause();
  mixer_input_->Play();
  EXPECT_DOUBLE_EQ(ProvideInput(), 1);
  mixer_input_->Stop();
}

// Test that Stop() can be called before Initialize() and Start().
TEST_F(AudioRendererMixerInputTest, StopBeforeInitializeOrStart) {
  mixer_input_->Stop();

  // Verify Stop() works without Initialize() or Start().
  CreateMixerInput(kDefaultDeviceId);
  mixer_input_->Stop();
}

// Test that Start() can be called after Stop().
TEST_F(AudioRendererMixerInputTest, StartAfterStop) {
  mixer_input_->Initialize(audio_parameters_, fake_callback_.get());
  mixer_input_->Stop();
  mixer_input_->Start();
  mixer_input_->Stop();
}

// Test that Initialize() can be called again after Stop().
TEST_F(AudioRendererMixerInputTest, InitializeAfterStop) {
  mixer_input_->Initialize(audio_parameters_, fake_callback_.get());
  mixer_input_->Start();
  mixer_input_->Stop();
  mixer_input_->Initialize(audio_parameters_, fake_callback_.get());
  mixer_input_->Stop();
}

// Test SwitchOutputDevice().
TEST_F(AudioRendererMixerInputTest, SwitchOutputDevice) {
  EXPECT_CALL(*this, GetOutputDeviceInfoCalled(testing::_))
      .Times(testing::Exactly(0));
  mixer_input_->Initialize(audio_parameters_, fake_callback_.get());
  mixer_input_->Start();
  const std::string kDeviceId("mock-device-id");
  EXPECT_CALL(*this, SwitchCallbackCalled(OUTPUT_DEVICE_STATUS_OK));
  AudioRendererMixer* old_mixer = GetInputMixer();
  EXPECT_EQ(old_mixer, mixers_[0].get());
  base::RunLoop run_loop;
  mixer_input_->SwitchOutputDevice(
      kDeviceId, base::Bind(&AudioRendererMixerInputTest::SwitchCallback,
                            base::Unretained(this), &run_loop));
  run_loop.Run();
  AudioRendererMixer* new_mixer = GetInputMixer();
  EXPECT_EQ(new_mixer, mixers_[1].get());
  EXPECT_NE(old_mixer, new_mixer);
  mixer_input_->Stop();
}

// Test SwitchOutputDevice() to the same device as the current (default) device
TEST_F(AudioRendererMixerInputTest, SwitchOutputDeviceToSameDevice) {
  EXPECT_CALL(*this, GetOutputDeviceInfoCalled(testing::_))
      .Times(testing::Exactly(0));
  mixer_input_->Initialize(audio_parameters_, fake_callback_.get());
  mixer_input_->Start();
  EXPECT_CALL(*this, SwitchCallbackCalled(OUTPUT_DEVICE_STATUS_OK));
  AudioRendererMixer* old_mixer = GetInputMixer();
  base::RunLoop run_loop;
  mixer_input_->SwitchOutputDevice(
      kDefaultDeviceId, base::Bind(&AudioRendererMixerInputTest::SwitchCallback,
                                   base::Unretained(this), &run_loop));
  run_loop.Run();
  AudioRendererMixer* new_mixer = GetInputMixer();
  EXPECT_EQ(old_mixer, new_mixer);
  mixer_input_->Stop();
}

// Test SwitchOutputDevice() to the new device
TEST_F(AudioRendererMixerInputTest, SwitchOutputDeviceToAnotherDevice) {
  EXPECT_CALL(*this, GetOutputDeviceInfoCalled(testing::_))
      .Times(testing::Exactly(0));
  mixer_input_->Initialize(audio_parameters_, fake_callback_.get());
  mixer_input_->Start();
  EXPECT_CALL(*this, SwitchCallbackCalled(OUTPUT_DEVICE_STATUS_OK));
  AudioRendererMixer* old_mixer = GetInputMixer();
  base::RunLoop run_loop;
  mixer_input_->SwitchOutputDevice(
      kAnotherDeviceId, base::Bind(&AudioRendererMixerInputTest::SwitchCallback,
                                   base::Unretained(this), &run_loop));
  run_loop.Run();
  AudioRendererMixer* new_mixer = GetInputMixer();
  EXPECT_NE(old_mixer, new_mixer);
  mixer_input_->Stop();
}

// Test that SwitchOutputDevice() to a nonexistent device fails.
TEST_F(AudioRendererMixerInputTest, SwitchOutputDeviceToNonexistentDevice) {
  EXPECT_CALL(*this, GetOutputDeviceInfoCalled(testing::_))
      .Times(testing::Exactly(0));
  mixer_input_->Initialize(audio_parameters_, fake_callback_.get());
  mixer_input_->Start();
  EXPECT_CALL(*this,
              SwitchCallbackCalled(OUTPUT_DEVICE_STATUS_ERROR_NOT_FOUND));
  base::RunLoop run_loop;
  mixer_input_->SwitchOutputDevice(
      kNonexistentDeviceId,
      base::Bind(&AudioRendererMixerInputTest::SwitchCallback,
                 base::Unretained(this), &run_loop));
  run_loop.Run();
  mixer_input_->Stop();
}

// Test that SwitchOutputDevice() to an unauthorized device fails.
TEST_F(AudioRendererMixerInputTest, SwitchOutputDeviceToUnauthorizedDevice) {
  EXPECT_CALL(*this, GetOutputDeviceInfoCalled(testing::_))
      .Times(testing::Exactly(0));
  mixer_input_->Initialize(audio_parameters_, fake_callback_.get());
  mixer_input_->Start();
  EXPECT_CALL(*this,
              SwitchCallbackCalled(OUTPUT_DEVICE_STATUS_ERROR_NOT_AUTHORIZED));
  base::RunLoop run_loop;
  mixer_input_->SwitchOutputDevice(
      kUnauthorizedDeviceId,
      base::Bind(&AudioRendererMixerInputTest::SwitchCallback,
                 base::Unretained(this), &run_loop));
  run_loop.Run();
  mixer_input_->Stop();
}

// Test that calling SwitchOutputDevice() before Start() succeeds.
TEST_F(AudioRendererMixerInputTest, SwitchOutputDeviceBeforeStart) {
  EXPECT_CALL(*this, GetOutputDeviceInfoCalled(kAnotherDeviceId))
      .Times(testing::Exactly(1));
  mixer_input_->Initialize(audio_parameters_, fake_callback_.get());
  base::RunLoop run_loop;
  EXPECT_CALL(*this, SwitchCallbackCalled(OUTPUT_DEVICE_STATUS_OK));
  mixer_input_->SwitchOutputDevice(
      kAnotherDeviceId, base::Bind(&AudioRendererMixerInputTest::SwitchCallback,
                                   base::Unretained(this), &run_loop));
  mixer_input_->Start();
  run_loop.Run();
  mixer_input_->Stop();
}

// Test that calling SwitchOutputDevice() succeeds even if Start() is never
// called.
TEST_F(AudioRendererMixerInputTest, SwitchOutputDeviceWithoutStart) {
  EXPECT_CALL(*this, GetOutputDeviceInfoCalled(kAnotherDeviceId))
      .Times(testing::Exactly(1));
  mixer_input_->Initialize(audio_parameters_, fake_callback_.get());
  base::RunLoop run_loop;
  EXPECT_CALL(*this, SwitchCallbackCalled(OUTPUT_DEVICE_STATUS_OK));
  mixer_input_->SwitchOutputDevice(
      kAnotherDeviceId, base::Bind(&AudioRendererMixerInputTest::SwitchCallback,
                                   base::Unretained(this), &run_loop));
  mixer_input_->Stop();
  run_loop.Run();
}

// Test creation with an invalid device. OnRenderError() should be called.
// Play(), Pause() and SwitchOutputDevice() should not cause crashes, even if
// they have no effect.
TEST_F(AudioRendererMixerInputTest, CreateWithInvalidDevice) {
  EXPECT_CALL(*this, GetOutputDeviceInfoCalled(kDefaultDeviceId))
      .Times(testing::Exactly(1));
  // |mixer_input_| was initialized during construction.
  mixer_input_->Stop();

  CreateMixerInput(kNonexistentDeviceId);
  EXPECT_CALL(*fake_callback_, OnRenderError());
  mixer_input_->Initialize(audio_parameters_, fake_callback_.get());
  mixer_input_->Start();
  mixer_input_->Play();
  mixer_input_->Pause();
  base::RunLoop run_loop;
  EXPECT_CALL(*this, SwitchCallbackCalled(testing::_));
  mixer_input_->SwitchOutputDevice(
      kDefaultDeviceId, base::Bind(&AudioRendererMixerInputTest::SwitchCallback,
                                   base::Unretained(this), &run_loop));
  mixer_input_->Stop();
  run_loop.Run();
}

// Test that calling SwitchOutputDevice() works after calling Stop(), and that
// restarting works after the call to SwitchOutputDevice().
TEST_F(AudioRendererMixerInputTest, SwitchOutputDeviceAfterStopBeforeRestart) {
  EXPECT_CALL(*this, GetOutputDeviceInfoCalled(kAnotherDeviceId))
      .Times(testing::Exactly(1));
  mixer_input_->Initialize(audio_parameters_, fake_callback_.get());
  mixer_input_->Start();
  mixer_input_->Stop();

  base::RunLoop run_loop;
  EXPECT_CALL(*this, SwitchCallbackCalled(OUTPUT_DEVICE_STATUS_OK));
  mixer_input_->SwitchOutputDevice(
      kAnotherDeviceId, base::Bind(&AudioRendererMixerInputTest::SwitchCallback,
                                   base::Unretained(this), &run_loop));
  run_loop.Run();

  mixer_input_->Start();
  mixer_input_->Stop();
}

// Test that calling SwitchOutputDevice() works before calling Initialize(),
// and that initialization and restart work after the call to
// SwitchOutputDevice().
TEST_F(AudioRendererMixerInputTest, SwitchOutputDeviceBeforeInitialize) {
  EXPECT_CALL(*this, GetOutputDeviceInfoCalled(kAnotherDeviceId))
      .Times(testing::Exactly(1));
  base::RunLoop run_loop;
  EXPECT_CALL(*this, SwitchCallbackCalled(OUTPUT_DEVICE_STATUS_OK));
  mixer_input_->SwitchOutputDevice(
      kAnotherDeviceId, base::Bind(&AudioRendererMixerInputTest::SwitchCallback,
                                   base::Unretained(this), &run_loop));
  run_loop.Run();

  mixer_input_->Initialize(audio_parameters_, fake_callback_.get());
  mixer_input_->Start();
  mixer_input_->Stop();
}

}  // namespace media
