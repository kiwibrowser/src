// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_SHELL_TEST_RUNNER_GAMEPAD_CONTROLLER_H_
#define CONTENT_SHELL_TEST_RUNNER_GAMEPAD_CONTROLLER_H_

#include <map>

#include "base/macros.h"
#include "base/memory/weak_ptr.h"
#include "content/shell/test_runner/test_runner_export.h"
#include "device/gamepad/public/cpp/gamepads.h"

namespace blink {
class WebGamepadListener;
class WebLocalFrame;
}

namespace test_runner {

class WebTestDelegate;

class TEST_RUNNER_EXPORT GamepadController
    : public base::SupportsWeakPtr<GamepadController> {
 public:
  static base::WeakPtr<GamepadController> Create(WebTestDelegate* delegate);
  ~GamepadController();

  void Reset();
  void Install(blink::WebLocalFrame* frame);

  void SampleGamepads(device::Gamepads& gamepads);
  void SetListener(blink::WebGamepadListener* listener);

 private:
  friend class GamepadControllerBindings;
  GamepadController();

  // TODO(b.kelemen): for historical reasons Connect just initializes the
  // object. The 'gamepadconnected' event will be dispatched via
  // DispatchConnected. Tests for connected events need to first connect(),
  // then set the gamepad data and finally call dispatchConnected().
  // We should consider renaming Connect to Init and DispatchConnected to
  // Connect and at the same time updating all the gamepad tests.
  void Connect(int index);
  void DispatchConnected(int index);

  void Disconnect(int index);
  void SetId(int index, const std::string& src);
  void SetButtonCount(int index, int buttons);
  void SetButtonData(int index, int button, double data);
  void SetAxisCount(int index, int axes);
  void SetAxisData(int index, int axis, double data);
  void SetDualRumbleVibrationActuator(int index, bool enabled);

  blink::WebGamepadListener* listener_;

  device::Gamepads gamepads_;

  // Mapping from gamepad index to connection state.
  std::map<int, bool> pending_changes_;

  base::WeakPtrFactory<GamepadController> weak_factory_;

  DISALLOW_COPY_AND_ASSIGN(GamepadController);
};

}  // namespace test_runner

#endif  // CONTENT_SHELL_TEST_RUNNER_GAMEPAD_CONTROLLER_H_
