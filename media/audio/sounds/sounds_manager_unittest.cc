// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <vector>

#include "base/compiler_specific.h"
#include "base/logging.h"
#include "base/macros.h"
#include "base/run_loop.h"
#include "base/strings/string_piece.h"
#include "base/test/test_message_loop.h"
#include "base/threading/thread_task_runner_handle.h"
#include "media/audio/audio_manager.h"
#include "media/audio/simple_sources.h"
#include "media/audio/sounds/audio_stream_handler.h"
#include "media/audio/sounds/sounds_manager.h"
#include "media/audio/sounds/test_data.h"
#include "media/audio/test_audio_thread.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace media {

class SoundsManagerTest : public testing::Test {
 public:
  SoundsManagerTest() = default;
  ~SoundsManagerTest() override = default;

  void SetUp() override {
    audio_manager_ =
        AudioManager::CreateForTesting(std::make_unique<TestAudioThread>());
    SoundsManager::Create();
    base::RunLoop().RunUntilIdle();
  }

  void TearDown() override {
    SoundsManager::Shutdown();
    audio_manager_->Shutdown();
    base::RunLoop().RunUntilIdle();
  }

  void SetObserverForTesting(AudioStreamHandler::TestObserver* observer) {
    AudioStreamHandler::SetObserverForTesting(observer);
  }

  void SetAudioSourceForTesting(
      AudioOutputStream::AudioSourceCallback* source) {
    AudioStreamHandler::SetAudioSourceForTesting(source);
  }

 private:
  base::TestMessageLoop message_loop_;
  std::unique_ptr<AudioManager> audio_manager_;
};

TEST_F(SoundsManagerTest, Play) {
  ASSERT_TRUE(SoundsManager::Get());

  base::RunLoop run_loop;
  TestObserver observer(run_loop.QuitClosure());

  SetObserverForTesting(&observer);

  ASSERT_TRUE(SoundsManager::Get()->Initialize(
      kTestAudioKey,
      base::StringPiece(kTestAudioData, arraysize(kTestAudioData))));
  ASSERT_EQ(20,
            SoundsManager::Get()->GetDuration(kTestAudioKey).InMicroseconds());
  ASSERT_TRUE(SoundsManager::Get()->Play(kTestAudioKey));
  run_loop.Run();

  ASSERT_EQ(1, observer.num_play_requests());
  ASSERT_EQ(1, observer.num_stop_requests());
  ASSERT_EQ(4, observer.cursor());

  SetObserverForTesting(NULL);
}

TEST_F(SoundsManagerTest, Stop) {
  ASSERT_TRUE(SoundsManager::Get());

  base::RunLoop run_loop;
  TestObserver observer(run_loop.QuitClosure());

  SetObserverForTesting(&observer);

  ASSERT_TRUE(SoundsManager::Get()->Initialize(
      kTestAudioKey,
      base::StringPiece(kTestAudioData, arraysize(kTestAudioData))));

  // This overrides the wav data set by kTestAudioData and results in
  // a never-ending sine wave being played.
  const int kChannels = 1;
  const double kFreq = 200;
  const double kSampleFreq = 44100;
  SineWaveAudioSource sine_source(kChannels, kFreq, kSampleFreq);
  SetAudioSourceForTesting(&sine_source);

  ASSERT_EQ(0, observer.num_play_requests());
  ASSERT_EQ(0, observer.num_stop_requests());

  ASSERT_TRUE(SoundsManager::Get()->Play(kTestAudioKey));
  ASSERT_TRUE(SoundsManager::Get()->Stop(kTestAudioKey));
  run_loop.Run();

  ASSERT_EQ(1, observer.num_play_requests());
  ASSERT_EQ(1, observer.num_stop_requests());

  SetObserverForTesting(NULL);
}

TEST_F(SoundsManagerTest, Uninitialized) {
  ASSERT_TRUE(SoundsManager::Get());
  ASSERT_FALSE(SoundsManager::Get()->Play(kTestAudioKey));
  ASSERT_FALSE(SoundsManager::Get()->Stop(kTestAudioKey));
}

}  // namespace media
