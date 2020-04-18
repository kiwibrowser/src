// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef THIRD_PARTY_BLINK_RENDERER_MODULES_GAMEPAD_GAMEPAD_BUTTON_H_
#define THIRD_PARTY_BLINK_RENDERER_MODULES_GAMEPAD_GAMEPAD_BUTTON_H_

#include "device/gamepad/public/cpp/gamepad.h"
#include "third_party/blink/renderer/platform/bindings/script_wrappable.h"
#include "third_party/blink/renderer/platform/heap/handle.h"
#include "third_party/blink/renderer/platform/wtf/vector.h"

namespace blink {

class GamepadButton final : public ScriptWrappable {
  DEFINE_WRAPPERTYPEINFO();

 public:
  static GamepadButton* Create();

  double value() const { return value_; }
  void SetValue(double val) { value_ = val; }

  bool pressed() const { return pressed_; }
  void SetPressed(bool val) { pressed_ = val; }

  bool touched() const { return touched_; }
  void SetTouched(bool val) { touched_ = val; }

  bool IsEqual(const device::GamepadButton&) const;
  void UpdateValuesFrom(const device::GamepadButton&);

 private:
  GamepadButton();
  double value_;
  bool pressed_;
  bool touched_;
};

typedef HeapVector<Member<GamepadButton>> GamepadButtonVector;

}  // namespace blink

#endif  // THIRD_PARTY_BLINK_RENDERER_MODULES_GAMEPAD_GAMEPAD_BUTTON_H_
