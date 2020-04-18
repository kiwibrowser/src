// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/modules/gamepad/gamepad_haptic_actuator.h"

#include "third_party/blink/renderer/bindings/core/v8/script_promise_resolver.h"
#include "third_party/blink/renderer/modules/gamepad/gamepad_dispatcher.h"

namespace {

using device::mojom::GamepadHapticsResult;
using device::mojom::GamepadHapticEffectType;

const char kGamepadHapticActuatorTypeVibration[] = "vibration";
const char kGamepadHapticActuatorTypeDualRumble[] = "dual-rumble";

const char kGamepadHapticEffectTypeDualRumble[] = "dual-rumble";

const char kGamepadHapticsResultComplete[] = "complete";
const char kGamepadHapticsResultPreempted[] = "preempted";
const char kGamepadHapticsResultInvalidParameter[] = "invalid-parameter";
const char kGamepadHapticsResultNotSupported[] = "not-supported";

GamepadHapticEffectType EffectTypeFromString(const String& type) {
  if (type == kGamepadHapticEffectTypeDualRumble)
    return GamepadHapticEffectType::GamepadHapticEffectTypeDualRumble;

  NOTREACHED();
  return GamepadHapticEffectType::GamepadHapticEffectTypeDualRumble;
}

String ResultToString(GamepadHapticsResult result) {
  switch (result) {
    case GamepadHapticsResult::GamepadHapticsResultComplete:
      return kGamepadHapticsResultComplete;
    case GamepadHapticsResult::GamepadHapticsResultPreempted:
      return kGamepadHapticsResultPreempted;
    case GamepadHapticsResult::GamepadHapticsResultInvalidParameter:
      return kGamepadHapticsResultInvalidParameter;
    case GamepadHapticsResult::GamepadHapticsResultNotSupported:
      return kGamepadHapticsResultNotSupported;
    default:
      NOTREACHED();
  }
  return kGamepadHapticsResultNotSupported;
}

}  // namespace

namespace blink {

// static
GamepadHapticActuator* GamepadHapticActuator::Create(int pad_index) {
  return new GamepadHapticActuator(
      pad_index, device::GamepadHapticActuatorType::kDualRumble);
}

GamepadHapticActuator::GamepadHapticActuator(
    int pad_index,
    device::GamepadHapticActuatorType type)
    : pad_index_(pad_index) {
  SetType(type);
}

GamepadHapticActuator::~GamepadHapticActuator() = default;

void GamepadHapticActuator::SetType(device::GamepadHapticActuatorType type) {
  switch (type) {
    case device::GamepadHapticActuatorType::kVibration:
      type_ = kGamepadHapticActuatorTypeVibration;
      break;
    case device::GamepadHapticActuatorType::kDualRumble:
      type_ = kGamepadHapticActuatorTypeDualRumble;
      break;
    default:
      NOTREACHED();
  }
}

ScriptPromise GamepadHapticActuator::playEffect(
    ScriptState* script_state,
    const String& type,
    const GamepadEffectParameters& params) {
  ScriptPromiseResolver* resolver = ScriptPromiseResolver::Create(script_state);

  if (params.duration() < 0.0 || params.startDelay() < 0.0 ||
      params.strongMagnitude() < 0.0 || params.strongMagnitude() > 1.0 ||
      params.weakMagnitude() < 0.0 || params.weakMagnitude() > 1.0) {
    ScriptPromise promise = resolver->Promise();
    resolver->Resolve(kGamepadHapticsResultInvalidParameter);
    return promise;
  }

  // Limit the total effect duration.
  double effect_duration = params.duration() + params.startDelay();
  if (effect_duration >
      device::GamepadHapticActuator::kMaxEffectDurationMillis) {
    ScriptPromise promise = resolver->Promise();
    resolver->Resolve(kGamepadHapticsResultInvalidParameter);
    return promise;
  }

  auto callback = WTF::Bind(&GamepadHapticActuator::OnPlayEffectCompleted,
                            WrapPersistent(this), WrapPersistent(resolver));

  GamepadDispatcher::Instance().PlayVibrationEffectOnce(
      pad_index_, EffectTypeFromString(type),
      device::mojom::blink::GamepadEffectParameters::New(
          params.duration(), params.startDelay(), params.strongMagnitude(),
          params.weakMagnitude()),
      std::move(callback));

  return resolver->Promise();
}

void GamepadHapticActuator::OnPlayEffectCompleted(
    ScriptPromiseResolver* resolver,
    device::mojom::GamepadHapticsResult result) {
  if (result == GamepadHapticsResult::GamepadHapticsResultError) {
    resolver->Reject();
    return;
  }
  resolver->Resolve(ResultToString(result));
}

ScriptPromise GamepadHapticActuator::reset(ScriptState* script_state) {
  ScriptPromiseResolver* resolver = ScriptPromiseResolver::Create(script_state);

  auto callback = WTF::Bind(&GamepadHapticActuator::OnResetCompleted,
                            WrapPersistent(this), WrapPersistent(resolver));

  GamepadDispatcher::Instance().ResetVibrationActuator(pad_index_,
                                                       std::move(callback));

  return resolver->Promise();
}

void GamepadHapticActuator::OnResetCompleted(
    ScriptPromiseResolver* resolver,
    device::mojom::GamepadHapticsResult result) {
  if (result == GamepadHapticsResult::GamepadHapticsResultError) {
    resolver->Reject();
    return;
  }
  resolver->Resolve(ResultToString(result));
}

void GamepadHapticActuator::Trace(blink::Visitor* visitor) {
  ScriptWrappable::Trace(visitor);
}

}  // namespace blink
