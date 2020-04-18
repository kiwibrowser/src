// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef REMOTING_HOST_REMOTE_INPUT_FILTER_H_
#define REMOTING_HOST_REMOTE_INPUT_FILTER_H_

#include <list>

#include "base/compiler_specific.h"
#include "base/macros.h"
#include "base/time/time.h"
#include "remoting/protocol/input_event_tracker.h"
#include "remoting/protocol/input_stub.h"

namespace remoting {

// Filtering InputStub that filters remotely-injected input if it has been
// notified of local input recently.
class RemoteInputFilter : public protocol::InputStub {
 public:
  // Creates a filter forwarding events to the specified InputEventTracker.
  // The filter needs a tracker to release buttons & keys when blocking input.
  explicit RemoteInputFilter(protocol::InputEventTracker* event_tracker);
  ~RemoteInputFilter() override;

  // Informs the filter that local mouse activity has been detected.  If the
  // activity does not match events we injected then we assume that it is local,
  // and block remote input for a short while.
  void LocalMouseMoved(const webrtc::DesktopVector& mouse_pos);

  // Informs the filter that injecting input causes an echo.
  void SetExpectLocalEcho(bool expect_local_echo);

  // InputStub overrides.
  void InjectKeyEvent(const protocol::KeyEvent& event) override;
  void InjectTextEvent(const protocol::TextEvent& event) override;
  void InjectMouseEvent(const protocol::MouseEvent& event) override;
  void InjectTouchEvent(const protocol::TouchEvent& event) override;

 private:
  bool ShouldIgnoreInput() const;

  protocol::InputEventTracker* event_tracker_;

  // Queue of recently-injected mouse positions used to distinguish echoes of
  // injected events from movements from a local input device.
  std::list<webrtc::DesktopVector> injected_mouse_positions_;

  // Time at which local input events were most recently observed.
  base::TimeTicks latest_local_input_time_;

  // If |true| than the filter assumes that injecting input causes an echo.
  bool expect_local_echo_;

  DISALLOW_COPY_AND_ASSIGN(RemoteInputFilter);
};

}  // namespace remoting

#endif  // REMOTING_HOST_REMOTE_INPUT_FILTER_H_
