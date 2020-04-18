// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ash/voice_interaction/voice_interaction_controller.h"

#include <memory>

#include "ash/shell.h"
#include "ash/test/ash_test_base.h"
#include "ash/voice_interaction/voice_interaction_observer.h"

namespace ash {
namespace {

class TestVoiceInteractionObserver : public VoiceInteractionObserver {
 public:
  TestVoiceInteractionObserver() = default;
  ~TestVoiceInteractionObserver() override = default;

  // VoiceInteractionObserver overrides:
  void OnVoiceInteractionStatusChanged(
      mojom::VoiceInteractionState state) override {
    state_ = state;
  }
  void OnVoiceInteractionSettingsEnabled(bool enabled) override {
    settings_enabled_ = enabled;
  }
  void OnVoiceInteractionContextEnabled(bool enabled) override {
    context_enabled_ = enabled;
  }
  void OnVoiceInteractionSetupCompleted(bool completed) override {
    setup_completed_ = completed;
  }

  mojom::VoiceInteractionState voice_interaction_state() const {
    return state_;
  }
  bool settings_enabled() const { return settings_enabled_; }
  bool context_enabled() const { return context_enabled_; }
  bool setup_completed() const { return setup_completed_; }

 private:
  mojom::VoiceInteractionState state_ = mojom::VoiceInteractionState::STOPPED;
  bool settings_enabled_ = false;
  bool context_enabled_ = false;
  bool setup_completed_ = false;

  DISALLOW_COPY_AND_ASSIGN(TestVoiceInteractionObserver);
};

class VoiceInteractionControllerTest : public AshTestBase {
 public:
  VoiceInteractionControllerTest() = default;
  ~VoiceInteractionControllerTest() override = default;

  void SetUp() override {
    AshTestBase::SetUp();

    observer_ = std::make_unique<TestVoiceInteractionObserver>();
    controller()->AddObserver(observer_.get());
  }

  void TearDown() override {
    controller()->RemoveObserver(observer_.get());
    observer_.reset();

    AshTestBase::TearDown();
  }

 protected:
  VoiceInteractionController* controller() {
    return Shell::Get()->voice_interaction_controller();
  }

  TestVoiceInteractionObserver* observer() { return observer_.get(); }

 private:
  std::unique_ptr<TestVoiceInteractionObserver> observer_;

  DISALLOW_COPY_AND_ASSIGN(VoiceInteractionControllerTest);
};

}  // namespace

TEST_F(VoiceInteractionControllerTest, NotifyStatusChanged) {
  controller()->NotifyStatusChanged(mojom::VoiceInteractionState::RUNNING);
  // The cached state should be updated.
  EXPECT_EQ(mojom::VoiceInteractionState::RUNNING,
            controller()->voice_interaction_state());
  // The observers should be notified.
  EXPECT_EQ(mojom::VoiceInteractionState::RUNNING,
            observer()->voice_interaction_state());
}

TEST_F(VoiceInteractionControllerTest, NotifySettingsEnabled) {
  controller()->NotifySettingsEnabled(true);
  // The cached state should be updated.
  EXPECT_TRUE(controller()->settings_enabled());
  // The observers should be notified.
  EXPECT_TRUE(observer()->settings_enabled());
}

TEST_F(VoiceInteractionControllerTest, NotifyContextEnabled) {
  controller()->NotifyContextEnabled(true);
  // The observers should be notified.
  EXPECT_TRUE(observer()->context_enabled());
}

TEST_F(VoiceInteractionControllerTest, NotifySetupCompleted) {
  controller()->NotifySetupCompleted(true);
  // The cached state should be updated.
  EXPECT_TRUE(controller()->setup_completed());
  // The observers should be notified.
  EXPECT_TRUE(observer()->setup_completed());
}

}  // namespace ash
