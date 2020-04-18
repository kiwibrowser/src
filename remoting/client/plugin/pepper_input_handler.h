// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef REMOTING_CLIENT_PLUGIN_PEPPER_INPUT_HANDLER_H_
#define REMOTING_CLIENT_PLUGIN_PEPPER_INPUT_HANDLER_H_

#include <memory>

#include "base/macros.h"

namespace pp {
class InputEvent;
}  // namespace pp

namespace remoting {

namespace protocol {
class InputEventTracker;
} // namespace protocol

class PepperInputHandler {
 public:
  // Create a PepperInputHandler using the specified InputEventTracker to
  // handle auto-release. The InputEventTracker instance must remain valid
  // for the lifetime of the PepperInputHandler.
  explicit PepperInputHandler(protocol::InputEventTracker* input_tracker);

  // Enable or disable sending mouse input when the plugin does not have input
  // focus.
  void set_send_mouse_input_when_unfocused(bool send) {
    send_mouse_input_when_unfocused_ = send;
  }

  void set_send_mouse_move_deltas(bool enable) {
    send_mouse_move_deltas_ = enable;
  }

  // Enable or disable detection of stuck modifier keys.
  void set_detect_stuck_modifiers(bool detect) {
    detect_stuck_modifiers_ = detect;
  }

  // Processes PPAPI events and dispatches them to |input_tracker_|.
  bool HandleInputEvent(const pp::InputEvent& event);

  // Must be called when the plugin receives or loses focus.
  void DidChangeFocus(bool has_focus);

 private:
  // Check for any missed "keyup" events for modifiers. These can sometimes be
  // missed due to OS-level keyboard shortcuts such as "lock screen" that cause
  // focus to switch to another application. If any modifier keys are held that
  // are not indicated as active on |event|, release all keys.
  void ReleaseAllIfModifiersStuck(const pp::InputEvent& event);

  // Tracks input events to manage auto-release in ReleaseAllIfModifiersStuck
  // and receives input events generated from PPAPI input.
  protocol::InputEventTracker* input_tracker_;

  // True if the plugin has focus.
  bool has_focus_;

  // True if the plugin should respond to mouse input even if it does not have
  // keyboard focus.
  bool send_mouse_input_when_unfocused_;

  // True if the plugin should include mouse move deltas, in addition to
  // absolute position information, in mouse events.
  bool send_mouse_move_deltas_;

  // Accumulated sub-pixel and sub-tick deltas from wheel events.
  float wheel_delta_x_;
  float wheel_delta_y_;
  float wheel_ticks_x_;
  float wheel_ticks_y_;

  // Whether or not to check for stuck modifier keys on keyboard input events.
  bool detect_stuck_modifiers_;

  DISALLOW_COPY_AND_ASSIGN(PepperInputHandler);
};

}  // namespace remoting

#endif  // REMOTING_CLIENT_PLUGIN_PEPPER_INPUT_HANDLER_H_
