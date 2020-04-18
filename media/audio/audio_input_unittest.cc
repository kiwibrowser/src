// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <stdint.h>

#include "base/bind.h"
#include "base/environment.h"
#include "base/macros.h"
#include "base/message_loop/message_loop.h"
#include "base/run_loop.h"
#include "base/test/test_message_loop.h"
#include "base/threading/platform_thread.h"
#include "base/threading/thread_task_runner_handle.h"
#include "build/build_config.h"
#include "media/audio/audio_device_description.h"
#include "media/audio/audio_device_info_accessor_for_tests.h"
#include "media/audio/audio_io.h"
#include "media/audio/audio_manager.h"
#include "media/audio/audio_unittest_util.h"
#include "media/audio/test_audio_thread.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace media {

// This class allows to find out if the callbacks are occurring as
// expected and if any error has been reported.
class TestInputCallback : public AudioInputStream::AudioInputCallback {
 public:
  TestInputCallback() : callback_count_(0), had_error_(0) {}
  void OnData(const AudioBus* source,
              base::TimeTicks capture_time,
              double volume) override {
    ++callback_count_;
  }
  void OnError() override { ++had_error_; }
  // Returns how many times OnData() has been called.
  int callback_count() const {
    return callback_count_;
  }
  // Returns how many times the OnError callback was called.
  int had_error() const {
    return had_error_;
  }

 private:
  int callback_count_;
  int had_error_;
};

class AudioInputTest : public testing::Test {
 public:
  AudioInputTest()
      : message_loop_(base::MessageLoop::TYPE_UI),
        audio_manager_(AudioManager::CreateForTesting(
            std::make_unique<TestAudioThread>())),
        audio_input_stream_(NULL) {
    base::RunLoop().RunUntilIdle();
  }

  ~AudioInputTest() override { audio_manager_->Shutdown(); }

 protected:
  bool InputDevicesAvailable() {
    return AudioDeviceInfoAccessorForTests(audio_manager_.get())
        .HasAudioInputDevices();
  }

  void MakeAudioInputStreamOnAudioThread() {
    RunOnAudioThread(
        base::Bind(&AudioInputTest::MakeAudioInputStream,
                   base::Unretained(this)));
  }

  void CloseAudioInputStreamOnAudioThread() {
    RunOnAudioThread(
        base::Bind(&AudioInputStream::Close,
                   base::Unretained(audio_input_stream_)));
    audio_input_stream_ = NULL;
  }

  void OpenAndCloseAudioInputStreamOnAudioThread() {
    RunOnAudioThread(
        base::Bind(&AudioInputTest::OpenAndClose,
                   base::Unretained(this)));
  }

  void OpenStopAndCloseAudioInputStreamOnAudioThread() {
    RunOnAudioThread(
        base::Bind(&AudioInputTest::OpenStopAndClose,
                   base::Unretained(this)));
  }

  void OpenAndStartAudioInputStreamOnAudioThread(
      AudioInputStream::AudioInputCallback* sink) {
    RunOnAudioThread(
        base::Bind(&AudioInputTest::OpenAndStart,
                   base::Unretained(this),
                   sink));
  }

  void StopAndCloseAudioInputStreamOnAudioThread() {
    RunOnAudioThread(
        base::Bind(&AudioInputTest::StopAndClose,
                   base::Unretained(this)));
  }

  void MakeAudioInputStream() {
    DCHECK(audio_manager_->GetTaskRunner()->BelongsToCurrentThread());
    AudioParameters params =
        AudioDeviceInfoAccessorForTests(audio_manager_.get())
            .GetInputStreamParameters(AudioDeviceDescription::kDefaultDeviceId);
    audio_input_stream_ = audio_manager_->MakeAudioInputStream(
        params, AudioDeviceDescription::kDefaultDeviceId,
        base::Bind(&AudioInputTest::OnLogMessage, base::Unretained(this)));
    EXPECT_TRUE(audio_input_stream_);
  }

  void OpenAndClose() {
    DCHECK(audio_manager_->GetTaskRunner()->BelongsToCurrentThread());
    EXPECT_TRUE(audio_input_stream_->Open());
    audio_input_stream_->Close();
    audio_input_stream_ = NULL;
  }

  void OpenAndStart(AudioInputStream::AudioInputCallback* sink) {
    DCHECK(audio_manager_->GetTaskRunner()->BelongsToCurrentThread());
    EXPECT_TRUE(audio_input_stream_->Open());
    audio_input_stream_->Start(sink);
  }

  void OpenStopAndClose() {
    DCHECK(audio_manager_->GetTaskRunner()->BelongsToCurrentThread());
    EXPECT_TRUE(audio_input_stream_->Open());
    audio_input_stream_->Stop();
    audio_input_stream_->Close();
    audio_input_stream_ = NULL;
  }

  void StopAndClose() {
    DCHECK(audio_manager_->GetTaskRunner()->BelongsToCurrentThread());
    audio_input_stream_->Stop();
    audio_input_stream_->Close();
    audio_input_stream_ = NULL;
  }

  // Synchronously runs the provided callback/closure on the audio thread.
  void RunOnAudioThread(const base::Closure& closure) {
    DCHECK(audio_manager_->GetTaskRunner()->BelongsToCurrentThread());
    closure.Run();
  }

  void OnLogMessage(const std::string& message) {}

  base::TestMessageLoop message_loop_;
  std::unique_ptr<AudioManager> audio_manager_;
  AudioInputStream* audio_input_stream_;

 private:
  DISALLOW_COPY_AND_ASSIGN(AudioInputTest);
};

// Test create and close of an AudioInputStream without recording audio.
TEST_F(AudioInputTest, CreateAndClose) {
  ABORT_AUDIO_TEST_IF_NOT(InputDevicesAvailable());
  MakeAudioInputStreamOnAudioThread();
  CloseAudioInputStreamOnAudioThread();
}

#if defined(OS_LINUX) && !defined(OS_CHROMEOS) && defined(ARCH_CPU_ARM_FAMILY)
// This test is failing on ARM linux: http://crbug.com/238490
#define MAYBE_OpenAndClose DISABLED_OpenAndClose
#else
#define MAYBE_OpenAndClose OpenAndClose
#endif
// Test create, open and close of an AudioInputStream without recording audio.
TEST_F(AudioInputTest, MAYBE_OpenAndClose) {
  ABORT_AUDIO_TEST_IF_NOT(InputDevicesAvailable());
  MakeAudioInputStreamOnAudioThread();
  OpenAndCloseAudioInputStreamOnAudioThread();
}

#if defined(OS_LINUX) && !defined(OS_CHROMEOS) && defined(ARCH_CPU_ARM_FAMILY)
// This test is failing on ARM linux: http://crbug.com/238490
#define MAYBE_OpenStopAndClose DISABLED_OpenStopAndClose
#else
#define MAYBE_OpenStopAndClose OpenStopAndClose
#endif
// Test create, open, stop and close of an AudioInputStream without recording.
TEST_F(AudioInputTest, MAYBE_OpenStopAndClose) {
  ABORT_AUDIO_TEST_IF_NOT(InputDevicesAvailable());
  MakeAudioInputStreamOnAudioThread();
  OpenStopAndCloseAudioInputStreamOnAudioThread();
}

#if defined(OS_LINUX) && !defined(OS_CHROMEOS) && defined(ARCH_CPU_ARM_FAMILY)
// This test is failing on ARM linux: http://crbug.com/238490
#define MAYBE_Record DISABLED_Record
#else
#define MAYBE_Record Record
#endif
// Test a normal recording sequence using an AudioInputStream.
// Very simple test which starts capturing during half a second and verifies
// that recording starts.
TEST_F(AudioInputTest, MAYBE_Record) {
  ABORT_AUDIO_TEST_IF_NOT(InputDevicesAvailable());
  MakeAudioInputStreamOnAudioThread();

  TestInputCallback test_callback;
  OpenAndStartAudioInputStreamOnAudioThread(&test_callback);

  base::RunLoop run_loop;
  base::ThreadTaskRunnerHandle::Get()->PostDelayedTask(
      FROM_HERE, run_loop.QuitClosure(),
      base::TimeDelta::FromMilliseconds(500));
  run_loop.Run();
  EXPECT_GE(test_callback.callback_count(), 2);
  EXPECT_FALSE(test_callback.had_error());

  StopAndCloseAudioInputStreamOnAudioThread();
}

}  // namespace media
