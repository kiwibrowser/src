// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "device/gamepad/abstract_haptic_gamepad.h"

#include <memory>

#include "base/macros.h"
#include "base/run_loop.h"
#include "base/test/test_simple_task_runner.h"
#include "device/gamepad/public/mojom/gamepad.mojom.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace device {

namespace {

// An implementation of AbstractHapticGamepad that records how many times its
// SetVibration and SetZeroVibration methods have been called.
class FakeHapticGamepad : public AbstractHapticGamepad {
 public:
  FakeHapticGamepad() : set_vibration_count_(0), set_zero_vibration_count_(0) {}
  ~FakeHapticGamepad() override = default;

  void SetVibration(double strong_magnitude, double weak_magnitude) override {
    set_vibration_count_++;
  }

  void SetZeroVibration() override { set_zero_vibration_count_++; }

  base::TimeDelta TaskDelayFromMilliseconds(double delay_millis) override {
    // Remove delays for testing.
    return base::TimeDelta();
  }

  int set_vibration_count_;
  int set_zero_vibration_count_;
};

// Main test fixture
class AbstractHapticGamepadTest : public testing::Test {
 public:
  AbstractHapticGamepadTest()
      : first_callback_count_(0),
        second_callback_count_(0),
        first_callback_result_(
            mojom::GamepadHapticsResult::GamepadHapticsResultError),
        second_callback_result_(
            mojom::GamepadHapticsResult::GamepadHapticsResultError),
        gamepad_(std::make_unique<FakeHapticGamepad>()),
        task_runner_(new base::TestSimpleTaskRunner) {}

  void TearDown() override { gamepad_->Shutdown(); }

  void PostPlayEffect(
      mojom::GamepadHapticEffectType type,
      double duration,
      double start_delay,
      mojom::GamepadHapticsManager::PlayVibrationEffectOnceCallback callback) {
    task_runner_->PostDelayedTask(
        FROM_HERE,
        base::BindOnce(&FakeHapticGamepad::PlayEffect,
                       base::Unretained(gamepad_.get()), type,
                       mojom::GamepadEffectParameters::New(
                           duration, start_delay, 1.0, 1.0),
                       std::move(callback)),
        base::TimeDelta());
  }

  void PostResetVibration(
      mojom::GamepadHapticsManager::ResetVibrationActuatorCallback callback) {
    task_runner_->PostDelayedTask(
        FROM_HERE,
        base::BindOnce(&FakeHapticGamepad::ResetVibration,
                       base::Unretained(gamepad_.get()), std::move(callback)),
        base::TimeDelta());
  }

  void FirstCallback(mojom::GamepadHapticsResult result) {
    first_callback_count_++;
    first_callback_result_ = result;
  }

  void SecondCallback(mojom::GamepadHapticsResult result) {
    second_callback_count_++;
    second_callback_result_ = result;
  }

  int first_callback_count_;
  int second_callback_count_;
  mojom::GamepadHapticsResult first_callback_result_;
  mojom::GamepadHapticsResult second_callback_result_;
  std::unique_ptr<FakeHapticGamepad> gamepad_;
  scoped_refptr<base::TestSimpleTaskRunner> task_runner_;

  DISALLOW_COPY_AND_ASSIGN(AbstractHapticGamepadTest);
};

TEST_F(AbstractHapticGamepadTest, PlayEffectTest) {
  EXPECT_EQ(0, gamepad_->set_vibration_count_);
  EXPECT_EQ(0, gamepad_->set_zero_vibration_count_);
  EXPECT_EQ(0, first_callback_count_);

  PostPlayEffect(
      mojom::GamepadHapticEffectType::GamepadHapticEffectTypeDualRumble, 1.0,
      0.0,
      base::BindOnce(&AbstractHapticGamepadTest::FirstCallback,
                     base::Unretained(this)));

  // Run the queued task, but stop before executing any new tasks queued by that
  // task. This should pause before calling SetVibration.
  task_runner_->RunPendingTasks();

  EXPECT_EQ(0, gamepad_->set_vibration_count_);
  EXPECT_EQ(0, gamepad_->set_zero_vibration_count_);
  EXPECT_EQ(0, first_callback_count_);
  EXPECT_TRUE(task_runner_->HasPendingTask());

  // Run the next task, but pause before completing the effect.
  task_runner_->RunPendingTasks();

  EXPECT_EQ(1, gamepad_->set_vibration_count_);
  EXPECT_EQ(0, gamepad_->set_zero_vibration_count_);
  EXPECT_EQ(0, first_callback_count_);
  EXPECT_TRUE(task_runner_->HasPendingTask());

  // Complete the effect and issue the callback. After this, there should be no
  // more pending tasks.
  task_runner_->RunPendingTasks();

  EXPECT_EQ(1, gamepad_->set_vibration_count_);
  EXPECT_EQ(1, gamepad_->set_zero_vibration_count_);
  EXPECT_EQ(1, first_callback_count_);
  EXPECT_EQ(mojom::GamepadHapticsResult::GamepadHapticsResultComplete,
            first_callback_result_);
  EXPECT_FALSE(task_runner_->HasPendingTask());
}

TEST_F(AbstractHapticGamepadTest, ResetVibrationTest) {
  EXPECT_EQ(0, gamepad_->set_vibration_count_);
  EXPECT_EQ(0, gamepad_->set_zero_vibration_count_);
  EXPECT_EQ(0, first_callback_count_);

  PostResetVibration(base::BindOnce(&AbstractHapticGamepadTest::FirstCallback,
                                    base::Unretained(this)));

  task_runner_->RunUntilIdle();

  // ResetVibration should return a "complete" result without calling
  // SetVibration or SetZeroVibration.
  EXPECT_EQ(0, gamepad_->set_vibration_count_);
  EXPECT_EQ(0, gamepad_->set_zero_vibration_count_);
  EXPECT_EQ(1, first_callback_count_);
  EXPECT_EQ(mojom::GamepadHapticsResult::GamepadHapticsResultComplete,
            first_callback_result_);
}

TEST_F(AbstractHapticGamepadTest, UnsupportedEffectTypeTest) {
  EXPECT_EQ(0, gamepad_->set_vibration_count_);
  EXPECT_EQ(0, gamepad_->set_zero_vibration_count_);
  EXPECT_EQ(0, first_callback_count_);

  mojom::GamepadHapticEffectType unsupported_effect_type =
      static_cast<mojom::GamepadHapticEffectType>(123);
  PostPlayEffect(unsupported_effect_type, 1.0, 0.0,
                 base::BindOnce(&AbstractHapticGamepadTest::FirstCallback,
                                base::Unretained(this)));

  task_runner_->RunUntilIdle();

  // An unsupported effect should return a "not-supported" result without
  // calling SetVibration or SetZeroVibration.
  EXPECT_EQ(0, gamepad_->set_vibration_count_);
  EXPECT_EQ(0, gamepad_->set_zero_vibration_count_);
  EXPECT_EQ(1, first_callback_count_);
  EXPECT_EQ(mojom::GamepadHapticsResult::GamepadHapticsResultNotSupported,
            first_callback_result_);
}

TEST_F(AbstractHapticGamepadTest, StartDelayTest) {
  EXPECT_EQ(0, gamepad_->set_vibration_count_);
  EXPECT_EQ(0, gamepad_->set_zero_vibration_count_);
  EXPECT_EQ(0, first_callback_count_);

  // This test establishes the behavior for the start_delay parameter when
  // PlayEffect is called without preempting an existing effect.
  PostPlayEffect(
      mojom::GamepadHapticEffectType::GamepadHapticEffectTypeDualRumble, 1.0,
      0.0,
      base::BindOnce(&AbstractHapticGamepadTest::FirstCallback,
                     base::Unretained(this)));

  task_runner_->RunUntilIdle();

  // With zero start_delay, SetVibration and SetZeroVibration should be called
  // exactly once each, to start and stop the effect.
  EXPECT_EQ(1, gamepad_->set_vibration_count_);
  EXPECT_EQ(1, gamepad_->set_zero_vibration_count_);
  EXPECT_EQ(1, first_callback_count_);
  EXPECT_EQ(mojom::GamepadHapticsResult::GamepadHapticsResultComplete,
            first_callback_result_);

  PostPlayEffect(
      mojom::GamepadHapticEffectType::GamepadHapticEffectTypeDualRumble, 1.0,
      1.0,
      base::BindOnce(&AbstractHapticGamepadTest::FirstCallback,
                     base::Unretained(this)));

  task_runner_->RunUntilIdle();

  // With non-zero start_delay, we still SetVibration and SetZeroVibration to be
  // called exactly once each.
  EXPECT_EQ(2, gamepad_->set_vibration_count_);
  EXPECT_EQ(2, gamepad_->set_zero_vibration_count_);
  EXPECT_EQ(2, first_callback_count_);
  EXPECT_EQ(mojom::GamepadHapticsResult::GamepadHapticsResultComplete,
            first_callback_result_);
}

TEST_F(AbstractHapticGamepadTest, ZeroStartDelayPreemptionTest) {
  EXPECT_EQ(0, gamepad_->set_vibration_count_);
  EXPECT_EQ(0, gamepad_->set_zero_vibration_count_);
  EXPECT_EQ(0, first_callback_count_);
  EXPECT_EQ(0, second_callback_count_);

  // Start an ongoing effect. We'll preempt this one with another effect.
  PostPlayEffect(
      mojom::GamepadHapticEffectType::GamepadHapticEffectTypeDualRumble, 1.0,
      0.0,
      base::BindOnce(&AbstractHapticGamepadTest::FirstCallback,
                     base::Unretained(this)));

  // Start a second effect with zero start_delay. This should cause the first
  // effect to be preempted before it calls SetVibration.
  PostPlayEffect(
      mojom::GamepadHapticEffectType::GamepadHapticEffectTypeDualRumble, 1.0,
      0.0,
      base::BindOnce(&AbstractHapticGamepadTest::SecondCallback,
                     base::Unretained(this)));

  // Execute the pending tasks, but stop before executing any newly queued
  // tasks.
  task_runner_->RunPendingTasks();

  // The first effect should have already returned with a "preempted" result.
  EXPECT_EQ(0, gamepad_->set_vibration_count_);
  EXPECT_EQ(0, gamepad_->set_zero_vibration_count_);
  EXPECT_EQ(1, first_callback_count_);
  EXPECT_EQ(0, second_callback_count_);
  EXPECT_EQ(mojom::GamepadHapticsResult::GamepadHapticsResultPreempted,
            first_callback_result_);

  task_runner_->RunUntilIdle();

  // Now the second effect should have returned with a "complete" result.
  EXPECT_EQ(1, gamepad_->set_vibration_count_);
  EXPECT_EQ(1, gamepad_->set_zero_vibration_count_);
  EXPECT_EQ(1, first_callback_count_);
  EXPECT_EQ(1, second_callback_count_);
  EXPECT_EQ(mojom::GamepadHapticsResult::GamepadHapticsResultComplete,
            second_callback_result_);
}

TEST_F(AbstractHapticGamepadTest, NonZeroStartDelayPreemptionTest) {
  EXPECT_EQ(0, gamepad_->set_vibration_count_);
  EXPECT_EQ(0, gamepad_->set_zero_vibration_count_);
  EXPECT_EQ(0, first_callback_count_);
  EXPECT_EQ(0, second_callback_count_);

  // Start an ongoing effect. We'll preempt this one with another effect.
  PostPlayEffect(
      mojom::GamepadHapticEffectType::GamepadHapticEffectTypeDualRumble, 1.0,
      0.0,
      base::BindOnce(&AbstractHapticGamepadTest::FirstCallback,
                     base::Unretained(this)));

  // Start a second effect with non-zero start_delay. This should cause the
  // first effect to be preempted before it calls SetVibration.
  PostPlayEffect(
      mojom::GamepadHapticEffectType::GamepadHapticEffectTypeDualRumble, 1.0,
      1.0,
      base::BindOnce(&AbstractHapticGamepadTest::SecondCallback,
                     base::Unretained(this)));

  // Execute the pending tasks, but stop before executing any newly queued
  // tasks.
  task_runner_->RunPendingTasks();

  // The first effect should have already returned with a "preempted" result.
  // Because the second effect has a non-zero start_delay and is preempting
  // another effect, it will call SetZeroVibration to ensure no vibration
  // occurs during its start_delay period.
  EXPECT_EQ(0, gamepad_->set_vibration_count_);
  EXPECT_EQ(1, gamepad_->set_zero_vibration_count_);
  EXPECT_EQ(1, first_callback_count_);
  EXPECT_EQ(0, second_callback_count_);
  EXPECT_EQ(mojom::GamepadHapticsResult::GamepadHapticsResultPreempted,
            first_callback_result_);

  task_runner_->RunUntilIdle();

  EXPECT_EQ(1, gamepad_->set_vibration_count_);
  EXPECT_EQ(2, gamepad_->set_zero_vibration_count_);
  EXPECT_EQ(1, first_callback_count_);
  EXPECT_EQ(1, second_callback_count_);
  EXPECT_EQ(mojom::GamepadHapticsResult::GamepadHapticsResultPreempted,
            first_callback_result_);
  EXPECT_EQ(mojom::GamepadHapticsResult::GamepadHapticsResultComplete,
            second_callback_result_);
}

TEST_F(AbstractHapticGamepadTest, ResetVibrationPreemptionTest) {
  EXPECT_EQ(0, gamepad_->set_vibration_count_);
  EXPECT_EQ(0, gamepad_->set_zero_vibration_count_);
  EXPECT_EQ(0, first_callback_count_);
  EXPECT_EQ(0, second_callback_count_);

  // Start an ongoing effect. We'll preempt it with a reset.
  PostPlayEffect(
      mojom::GamepadHapticEffectType::GamepadHapticEffectTypeDualRumble, 1.0,
      0.0,
      base::BindOnce(&AbstractHapticGamepadTest::FirstCallback,
                     base::Unretained(this)));

  // Reset vibration. This should cause the effect to be preempted before it
  // calls SetVibration.
  PostResetVibration(base::BindOnce(&AbstractHapticGamepadTest::SecondCallback,
                                    base::Unretained(this)));

  task_runner_->RunUntilIdle();

  EXPECT_EQ(0, gamepad_->set_vibration_count_);
  EXPECT_EQ(1, gamepad_->set_zero_vibration_count_);
  EXPECT_EQ(1, first_callback_count_);
  EXPECT_EQ(1, second_callback_count_);
  EXPECT_EQ(mojom::GamepadHapticsResult::GamepadHapticsResultPreempted,
            first_callback_result_);
  EXPECT_EQ(mojom::GamepadHapticsResult::GamepadHapticsResultComplete,
            second_callback_result_);
}

}  // namespace

}  // namespace device
