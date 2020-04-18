// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <memory>
#include <vector>

#include "base/command_line.h"
#include "base/macros.h"
#include "chrome/browser/browser_process.h"
#include "chrome/browser/chromeos/accessibility/accessibility_manager.h"
#include "chrome/browser/ui/ash/volume_controller.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "chromeos/audio/chromeos_sounds.h"
#include "chromeos/audio/cras_audio_handler.h"
#include "chromeos/chromeos_switches.h"
#include "media/audio/sounds/sounds_manager.h"
#include "ui/base/accelerators/accelerator.h"

namespace {

class SoundsManagerTestImpl : public media::SoundsManager {
 public:
  SoundsManagerTestImpl()
      : is_sound_initialized_(chromeos::SOUND_COUNT),
        num_play_requests_(chromeos::SOUND_COUNT) {}

  ~SoundsManagerTestImpl() override {}

  bool Initialize(SoundKey key, const base::StringPiece& /* data */) override {
    is_sound_initialized_[key] = true;
    return true;
  }

  bool Play(SoundKey key) override {
    ++num_play_requests_[key];
    return true;
  }

  bool Stop(SoundKey key) override { return true; }

  base::TimeDelta GetDuration(SoundKey /* key */) override {
    return base::TimeDelta();
  }

  bool is_sound_initialized(SoundKey key) const {
    return is_sound_initialized_[key];
  }

  int num_play_requests(SoundKey key) const { return num_play_requests_[key]; }

 private:
  std::vector<bool> is_sound_initialized_;
  std::vector<int> num_play_requests_;

  DISALLOW_COPY_AND_ASSIGN(SoundsManagerTestImpl);
};

class VolumeControllerTest : public InProcessBrowserTest {
 public:
  VolumeControllerTest() {}
  ~VolumeControllerTest() override {}

  void SetUpOnMainThread() override {
    volume_controller_.reset(new VolumeController());
    audio_handler_ = chromeos::CrasAudioHandler::Get();
  }

 protected:
  chromeos::CrasAudioHandler* audio_handler_;  // Not owned.
  std::unique_ptr<VolumeController> volume_controller_;

 private:
  DISALLOW_COPY_AND_ASSIGN(VolumeControllerTest);
};

IN_PROC_BROWSER_TEST_F(VolumeControllerTest, VolumeUpAndDown) {
  // Set initial value as 50%
  const int kInitVolume = 50;
  audio_handler_->SetOutputVolumePercent(kInitVolume);

  EXPECT_EQ(audio_handler_->GetOutputVolumePercent(), kInitVolume);

  volume_controller_->VolumeUp();
  EXPECT_LT(kInitVolume, audio_handler_->GetOutputVolumePercent());
  volume_controller_->VolumeDown();
  EXPECT_EQ(kInitVolume, audio_handler_->GetOutputVolumePercent());
  volume_controller_->VolumeDown();
  EXPECT_GT(kInitVolume, audio_handler_->GetOutputVolumePercent());
}

IN_PROC_BROWSER_TEST_F(VolumeControllerTest, VolumeDownToZero) {
  // Setting to very small volume.
  audio_handler_->SetOutputVolumePercent(1);

  volume_controller_->VolumeDown();
  EXPECT_EQ(0, audio_handler_->GetOutputVolumePercent());
  volume_controller_->VolumeDown();
  EXPECT_EQ(0, audio_handler_->GetOutputVolumePercent());
  volume_controller_->VolumeUp();
  EXPECT_LT(0, audio_handler_->GetOutputVolumePercent());
}

IN_PROC_BROWSER_TEST_F(VolumeControllerTest, VolumeUpTo100) {
  // Setting to almost max
  audio_handler_->SetOutputVolumePercent(99);

  volume_controller_->VolumeUp();
  EXPECT_EQ(100, audio_handler_->GetOutputVolumePercent());
  volume_controller_->VolumeUp();
  EXPECT_EQ(100, audio_handler_->GetOutputVolumePercent());
  volume_controller_->VolumeDown();
  EXPECT_GT(100, audio_handler_->GetOutputVolumePercent());
}

IN_PROC_BROWSER_TEST_F(VolumeControllerTest, Mutes) {
  ASSERT_FALSE(audio_handler_->IsOutputMuted());
  const int initial_volume = audio_handler_->GetOutputVolumePercent();

  volume_controller_->VolumeMute();
  EXPECT_TRUE(audio_handler_->IsOutputMuted());

  // Further mute buttons doesn't have effects.
  volume_controller_->VolumeMute();
  EXPECT_TRUE(audio_handler_->IsOutputMuted());

  // Right after the volume up after set_mute recovers to original volume.
  volume_controller_->VolumeUp();
  EXPECT_FALSE(audio_handler_->IsOutputMuted());
  EXPECT_EQ(initial_volume, audio_handler_->GetOutputVolumePercent());

  volume_controller_->VolumeMute();
  // After the volume down, the volume goes down to zero explicitly.
  volume_controller_->VolumeDown();
  EXPECT_TRUE(audio_handler_->IsOutputMuted());
  EXPECT_EQ(0, audio_handler_->GetOutputVolumePercent());

  // Thus, further VolumeUp doesn't recover the volume, it's just slightly
  // bigger than 0.
  volume_controller_->VolumeUp();
  EXPECT_LT(0, audio_handler_->GetOutputVolumePercent());
  EXPECT_GT(initial_volume, audio_handler_->GetOutputVolumePercent());
}

class VolumeControllerSoundsTest : public VolumeControllerTest {
 public:
  VolumeControllerSoundsTest() : sounds_manager_(NULL) {}
  ~VolumeControllerSoundsTest() override {}

  void SetUpInProcessBrowserTestFixture() override {
    sounds_manager_ = new SoundsManagerTestImpl();
    media::SoundsManager::InitializeForTesting(sounds_manager_);
  }

  bool is_sound_initialized() const {
    return sounds_manager_->is_sound_initialized(chromeos::SOUND_VOLUME_ADJUST);
  }

  int num_play_requests() const {
    return sounds_manager_->num_play_requests(chromeos::SOUND_VOLUME_ADJUST);
  }

 private:
  SoundsManagerTestImpl* sounds_manager_;

  DISALLOW_COPY_AND_ASSIGN(VolumeControllerSoundsTest);
};

IN_PROC_BROWSER_TEST_F(VolumeControllerSoundsTest, Simple) {
  audio_handler_->SetOutputVolumePercent(50);

  chromeos::AccessibilityManager::Get()->EnableSpokenFeedback(false);
  volume_controller_->VolumeUp();
  volume_controller_->VolumeDown();
  EXPECT_EQ(0, num_play_requests());

  chromeos::AccessibilityManager::Get()->EnableSpokenFeedback(true);
  volume_controller_->VolumeUp();
  volume_controller_->VolumeDown();
  EXPECT_EQ(2, num_play_requests());
}

IN_PROC_BROWSER_TEST_F(VolumeControllerSoundsTest, EdgeCases) {
  EXPECT_TRUE(is_sound_initialized());
  chromeos::AccessibilityManager::Get()->EnableSpokenFeedback(true);

  // Check that sound is played on volume up and volume down.
  audio_handler_->SetOutputVolumePercent(50);
  volume_controller_->VolumeUp();
  EXPECT_EQ(1, num_play_requests());
  volume_controller_->VolumeDown();
  EXPECT_EQ(2, num_play_requests());

  audio_handler_->SetOutputVolumePercent(99);
  volume_controller_->VolumeUp();
  EXPECT_EQ(3, num_play_requests());

  audio_handler_->SetOutputVolumePercent(100);
  volume_controller_->VolumeUp();
  EXPECT_EQ(3, num_play_requests());

  // Check that sound isn't played when audio is muted.
  audio_handler_->SetOutputVolumePercent(50);
  volume_controller_->VolumeMute();
  volume_controller_->VolumeDown();
  ASSERT_TRUE(audio_handler_->IsOutputMuted());
  EXPECT_EQ(3, num_play_requests());

  // Check that audio is unmuted and sound is played.
  volume_controller_->VolumeUp();
  ASSERT_FALSE(audio_handler_->IsOutputMuted());
  EXPECT_EQ(4, num_play_requests());
}

class VolumeControllerSoundsDisabledTest : public VolumeControllerSoundsTest {
 public:
  VolumeControllerSoundsDisabledTest() {}
  ~VolumeControllerSoundsDisabledTest() override {}

  void SetUpCommandLine(base::CommandLine* command_line) override {
    VolumeControllerSoundsTest::SetUpCommandLine(command_line);
    command_line->AppendSwitch(chromeos::switches::kDisableVolumeAdjustSound);
  }

 private:
  DISALLOW_COPY_AND_ASSIGN(VolumeControllerSoundsDisabledTest);
};

IN_PROC_BROWSER_TEST_F(VolumeControllerSoundsDisabledTest, VolumeAdjustSounds) {
  EXPECT_FALSE(is_sound_initialized());

  // Check that sound isn't played on volume up and volume down.
  audio_handler_->SetOutputVolumePercent(50);
  volume_controller_->VolumeUp();
  volume_controller_->VolumeDown();
  EXPECT_EQ(0, num_play_requests());
}

}  // namespace
