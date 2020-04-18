// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef DEVICE_GAMEPAD_ABSTRACT_HAPTIC_GAMEPAD_
#define DEVICE_GAMEPAD_ABSTRACT_HAPTIC_GAMEPAD_

#include "base/memory/scoped_refptr.h"
#include "base/sequenced_task_runner.h"
#include "base/time/time.h"
#include "device/gamepad/gamepad_export.h"
#include "device/gamepad/public/mojom/gamepad.mojom.h"

namespace device {

// AbstractHapticGamepad is a base class for gamepads that support dual-rumble
// vibration effects. To use it, override the SetVibration method so that it
// sets the vibration intensity on the device. Then, calling PlayEffect or
// ResetVibration will call your SetVibration method at the appropriate times
// to produce the desired vibration effect. When the effect is complete, or when
// it has been preempted by another effect, the callback is invoked with a
// result code.
//
// By default, SetZeroVibration simply calls SetVibration with both parameters
// set to zero. You may optionally override SetZeroVibration if the device has a
// more efficient means of stopping an ongoing effect.
class DEVICE_GAMEPAD_EXPORT AbstractHapticGamepad {
 public:
  AbstractHapticGamepad();
  virtual ~AbstractHapticGamepad();

  // Start playing an effect.
  void PlayEffect(
      mojom::GamepadHapticEffectType,
      mojom::GamepadEffectParametersPtr,
      mojom::GamepadHapticsManager::PlayVibrationEffectOnceCallback);

  // Reset vibration on the gamepad, perhaps interrupting an ongoing effect.
  void ResetVibration(
      mojom::GamepadHapticsManager::ResetVibrationActuatorCallback);

  // Stop vibration and release held resources.
  void Shutdown();

  // Set the vibration magnitude for the strong and weak vibration actuators.
  virtual void SetVibration(double strong_magnitude, double weak_magnitude) = 0;

  // Set the vibration magnitude for both actuators to zero.
  virtual void SetZeroVibration();

 private:
  // Override to perform additional shutdown actions after vibration effects
  // are halted and callbacks are issued.
  virtual void DoShutdown() {}

  // For testing.
  virtual base::TimeDelta TaskDelayFromMilliseconds(double delay_millis);

  void PlayDualRumbleEffect(int sequence_id,
                            double duration,
                            double start_delay,
                            double strong_magnitude,
                            double weak_magnitude);
  void StartVibration(int sequence_id,
                      double duration,
                      double strong_magnitude,
                      double weak_magnitude);
  void StopVibration(int sequence_id);

  void RunCallbackOnMojoThread(mojom::GamepadHapticsResult result);

  static void DoRunCallback(
      mojom::GamepadHapticsManager::PlayVibrationEffectOnceCallback,
      mojom::GamepadHapticsResult);

  bool is_shut_down_;
  int sequence_id_;
  scoped_refptr<base::SequencedTaskRunner> playing_effect_task_runner_;
  mojom::GamepadHapticsManager::PlayVibrationEffectOnceCallback
      playing_effect_callback_;
};

}  // namespace device

#endif  // DEVICE_GAMEPAD_ABSTRACT_HAPTIC_GAMEPAD_
