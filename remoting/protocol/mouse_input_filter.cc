// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "remoting/protocol/mouse_input_filter.h"

#include "remoting/proto/event.pb.h"

namespace remoting {
namespace protocol {

MouseInputFilter::MouseInputFilter() = default;

MouseInputFilter::MouseInputFilter(InputStub* input_stub)
    : InputFilter(input_stub) {
}

MouseInputFilter::~MouseInputFilter() = default;

void MouseInputFilter::InjectMouseEvent(const MouseEvent& event) {
  if (input_max_.is_empty() || output_max_.is_empty())
    return;

  // We scale based on the maximum input & output coordinates, rather than the
  // input and output sizes, so that it's possible to reach the edge of the
  // output when up-scaling.  We also take care to round up or down correctly,
  // which is important when down-scaling.
  MouseEvent out_event(event);
  if (out_event.has_x()) {
    int x = out_event.x() * output_max_.width();
    x = (x + input_max_.width() / 2) / input_max_.width();
    out_event.set_x(std::max(0, std::min(output_max_.width(), x)));
  }
  if (out_event.has_y()) {
    int y = out_event.y() * output_max_.height();
    y = (y + input_max_.height() / 2) / input_max_.height();
    out_event.set_y(std::max(0, std::min(output_max_.height(), y)));
  }

  InputFilter::InjectMouseEvent(out_event);
}

void MouseInputFilter::set_input_size(const webrtc::DesktopSize& size) {
  input_max_.set(size.width() - 1, size.height() - 1);
}

void MouseInputFilter::set_output_size(const webrtc::DesktopSize& size) {
  output_max_.set(size.width() - 1, size.height() - 1);
}

}  // namespace protocol
}  // namespace remoting
