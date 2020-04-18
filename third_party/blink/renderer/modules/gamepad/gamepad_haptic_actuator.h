// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef THIRD_PARTY_BLINK_RENDERER_MODULES_GAMEPAD_GAMEPAD_HAPTIC_ACTUATOR_H_
#define THIRD_PARTY_BLINK_RENDERER_MODULES_GAMEPAD_GAMEPAD_HAPTIC_ACTUATOR_H_

#include "device/gamepad/public/cpp/gamepad.h"
#include "device/gamepad/public/mojom/gamepad.mojom-blink.h"
#include "third_party/blink/renderer/bindings/core/v8/script_promise.h"
#include "third_party/blink/renderer/bindings/core/v8/script_promise_resolver.h"
#include "third_party/blink/renderer/modules/gamepad/gamepad_effect_parameters.h"
#include "third_party/blink/renderer/platform/bindings/script_wrappable.h"
#include "third_party/blink/renderer/platform/heap/handle.h"
#include "third_party/blink/renderer/platform/wtf/text/wtf_string.h"
#include "third_party/blink/renderer/platform/wtf/vector.h"

namespace blink {

class GamepadHapticActuator final : public ScriptWrappable {
  DEFINE_WRAPPERTYPEINFO();

 public:
  static GamepadHapticActuator* Create(int pad_index);
  ~GamepadHapticActuator() override;

  const String& type() const { return type_; }
  void SetType(device::GamepadHapticActuatorType);

  ScriptPromise playEffect(ScriptState*,
                           const String&,
                           const GamepadEffectParameters&);

  ScriptPromise reset(ScriptState*);

  void Trace(blink::Visitor*) override;

 private:
  GamepadHapticActuator(int pad_index, device::GamepadHapticActuatorType);

  void OnPlayEffectCompleted(ScriptPromiseResolver*,
                             device::mojom::GamepadHapticsResult);
  void OnResetCompleted(ScriptPromiseResolver*,
                        device::mojom::GamepadHapticsResult);

  int pad_index_;
  String type_;
};

typedef HeapVector<Member<GamepadHapticActuator>> GamepadHapticActuatorVector;

}  // namespace blink

#endif  // THIRD_PARTY_BLINK_RENDERER_MODULES_GAMEPAD_GAMEPAD_HAPTIC_ACTUATOR_H_
