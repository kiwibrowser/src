// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef REMOTING_PROTOCOL_MOUSE_INPUT_FILTER_H_
#define REMOTING_PROTOCOL_MOUSE_INPUT_FILTER_H_

#include "base/compiler_specific.h"
#include "base/macros.h"
#include "remoting/protocol/input_filter.h"
#include "third_party/webrtc/modules/desktop_capture/desktop_geometry.h"

namespace remoting {
namespace protocol {

// Filtering InputStub implementation which scales mouse events based on the
// supplied input and output dimensions, and clamps their coordinates to the
// output dimensions, before passing events on to |input_stub|.
class MouseInputFilter : public InputFilter {
 public:
  MouseInputFilter();
  explicit MouseInputFilter(InputStub* input_stub);
  ~MouseInputFilter() override;

  // Specify the input dimensions for mouse events.
  void set_input_size(const webrtc::DesktopSize& size);

  // Specify the output dimensions.
  void set_output_size(const webrtc::DesktopSize& size);

  // InputStub overrides.
  void InjectMouseEvent(const protocol::MouseEvent& event) override;

 private:
  webrtc::DesktopSize input_max_;
  webrtc::DesktopSize output_max_;

  DISALLOW_COPY_AND_ASSIGN(MouseInputFilter);
};

}  // namespace protocol
}  // namespace remoting

#endif  // REMOTING_PROTOCOL_MOUSE_INPUT_FILTER_H_
