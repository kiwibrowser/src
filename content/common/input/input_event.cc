// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/common/input/input_event.h"

#include "ui/events/blink/web_input_event_traits.h"

namespace content {

InputEvent::InputEvent() {}

InputEvent::InputEvent(ui::WebScopedInputEvent event,
                       const ui::LatencyInfo& info)
    : web_event(std::move(event)), latency_info(info) {}

InputEvent::InputEvent(const blink::WebInputEvent& web_event,
                       const ui::LatencyInfo& latency_info)
    : web_event(ui::WebInputEventTraits::Clone(web_event)),
      latency_info(latency_info) {}

InputEvent::~InputEvent() {}

}  // namespace content
