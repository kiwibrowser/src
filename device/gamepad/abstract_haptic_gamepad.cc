// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "device/gamepad/abstract_haptic_gamepad.h"

namespace device {

AbstractHapticGamepad::AbstractHapticGamepad()
    : is_shut_down_(false), sequence_id_(0) {}

AbstractHapticGamepad::~AbstractHapticGamepad() {
  // Shutdown() must be called to allow the device a chance to stop vibration
  // and release held resources.
  DCHECK(is_shut_down_);
}

void AbstractHapticGamepad::Shutdown() {
  if (playing_effect_callback_) {
    sequence_id_++;
    SetZeroVibration();
    RunCallbackOnMojoThread(
        mojom::GamepadHapticsResult::GamepadHapticsResultPreempted);
  }
  DoShutdown();
  is_shut_down_ = true;
}

void AbstractHapticGamepad::SetZeroVibration() {
  SetVibration(0.0, 0.0);
}

base::TimeDelta AbstractHapticGamepad::TaskDelayFromMilliseconds(
    double delay_millis) {
  return base::TimeDelta::FromMillisecondsD(delay_millis);
}

void AbstractHapticGamepad::PlayEffect(
    mojom::GamepadHapticEffectType type,
    mojom::GamepadEffectParametersPtr params,
    mojom::GamepadHapticsManager::PlayVibrationEffectOnceCallback callback) {
  DCHECK(!is_shut_down_);
  if (type !=
      mojom::GamepadHapticEffectType::GamepadHapticEffectTypeDualRumble) {
    // Only dual-rumble effects are supported.
    std::move(callback).Run(
        mojom::GamepadHapticsResult::GamepadHapticsResultNotSupported);
    return;
  }

  int sequence_id = ++sequence_id_;

  if (playing_effect_callback_) {
    if (params->start_delay > 0.0)
      SetZeroVibration();
    RunCallbackOnMojoThread(
        mojom::GamepadHapticsResult::GamepadHapticsResultPreempted);
  }

  playing_effect_task_runner_ = base::ThreadTaskRunnerHandle::Get();
  playing_effect_callback_ = std::move(callback);

  PlayDualRumbleEffect(sequence_id, params->duration, params->start_delay,
                       params->strong_magnitude, params->weak_magnitude);
}

void AbstractHapticGamepad::ResetVibration(
    mojom::GamepadHapticsManager::ResetVibrationActuatorCallback callback) {
  DCHECK(!is_shut_down_);
  sequence_id_++;

  if (playing_effect_callback_) {
    SetZeroVibration();
    RunCallbackOnMojoThread(
        mojom::GamepadHapticsResult::GamepadHapticsResultPreempted);
  }

  std::move(callback).Run(
      mojom::GamepadHapticsResult::GamepadHapticsResultComplete);
}

void AbstractHapticGamepad::PlayDualRumbleEffect(int sequence_id,
                                                 double duration,
                                                 double start_delay,
                                                 double strong_magnitude,
                                                 double weak_magnitude) {
  base::ThreadTaskRunnerHandle::Get()->PostDelayedTask(
      FROM_HERE,
      base::BindOnce(&AbstractHapticGamepad::StartVibration,
                     base::Unretained(this), sequence_id_, duration,
                     strong_magnitude, weak_magnitude),
      TaskDelayFromMilliseconds(start_delay));
}

void AbstractHapticGamepad::StartVibration(int sequence_id,
                                           double duration,
                                           double strong_magnitude,
                                           double weak_magnitude) {
  if (is_shut_down_ || sequence_id != sequence_id_)
    return;
  SetVibration(strong_magnitude, weak_magnitude);

  base::ThreadTaskRunnerHandle::Get()->PostDelayedTask(
      FROM_HERE,
      base::BindOnce(&AbstractHapticGamepad::StopVibration,
                     base::Unretained(this), sequence_id),
      TaskDelayFromMilliseconds(duration));
}

void AbstractHapticGamepad::StopVibration(int sequence_id) {
  if (is_shut_down_ || sequence_id != sequence_id_)
    return;
  SetZeroVibration();

  RunCallbackOnMojoThread(
      mojom::GamepadHapticsResult::GamepadHapticsResultComplete);
}

void AbstractHapticGamepad::RunCallbackOnMojoThread(
    mojom::GamepadHapticsResult result) {
  if (playing_effect_task_runner_->RunsTasksInCurrentSequence()) {
    DoRunCallback(std::move(playing_effect_callback_), result);
    return;
  }

  playing_effect_task_runner_->PostTask(
      FROM_HERE, base::BindOnce(&AbstractHapticGamepad::DoRunCallback,
                                std::move(playing_effect_callback_), result));
}

// static
void AbstractHapticGamepad::DoRunCallback(
    mojom::GamepadHapticsManager::PlayVibrationEffectOnceCallback callback,
    mojom::GamepadHapticsResult result) {
  std::move(callback).Run(result);
}

}  // namespace device
