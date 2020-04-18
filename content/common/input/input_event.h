// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_COMMON_INPUT_INPUT_EVENT_H_
#define CONTENT_COMMON_INPUT_INPUT_EVENT_H_

#include <memory>

#include "base/macros.h"
#include "content/common/content_export.h"
#include "ui/events/blink/web_input_event_traits.h"
#include "ui/latency/latency_info.h"

namespace blink {
class WebInputEvent;
}

namespace content {

// An content-specific wrapper for WebInputEvents and associated metadata.
class CONTENT_EXPORT InputEvent {
 public:
  InputEvent();
  InputEvent(ui::WebScopedInputEvent web_event,
             const ui::LatencyInfo& latency_info);
  InputEvent(const blink::WebInputEvent& web_event,
             const ui::LatencyInfo& latency_info);
  ~InputEvent();

  ui::WebScopedInputEvent web_event;
  ui::LatencyInfo latency_info;

 private:
  DISALLOW_COPY_AND_ASSIGN(InputEvent);
};

}  // namespace content

#endif  // CONTENT_COMMON_INPUT_INPUT_EVENT_H_
