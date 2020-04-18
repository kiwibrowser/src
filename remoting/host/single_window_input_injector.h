// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef REMOTING_HOST_SINGLE_WINDOW_INPUT_INJECTOR_H_
#define REMOTING_HOST_SINGLE_WINDOW_INPUT_INJECTOR_H_

#include <memory>

#include "remoting/host/input_injector.h"
#include "third_party/webrtc/modules/desktop_capture/desktop_capture_types.h"

namespace remoting {

// This class is used to wrap an InputInjector. All this class does
// is forward messages to the wrapped InputInjector. When a MouseEvent
// comes in, SingleWindowInputInjector adjusts the MouseEvent's
// coordinates so that they are in respect to the window instead of
// the desktop. Clipboard, Text, and Key events are still global.
class SingleWindowInputInjector : public InputInjector {
 public:
  // This Create method needs to be passed a full desktop
  // InputInjector.
  static std::unique_ptr<InputInjector> CreateForWindow(
      webrtc::WindowId window_id,
      std::unique_ptr<InputInjector> input_injector);
};

}  // namespace remoting

#endif  // REMOTING_HOST_SINGLE_WINDOW_INPUT_INJECTOR_H_
