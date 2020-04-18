// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_COMMON_INPUT_TOUCH_EVENT_STREAM_VALIDATOR_H_
#define CONTENT_COMMON_INPUT_TOUCH_EVENT_STREAM_VALIDATOR_H_

#include <string>

#include "base/macros.h"
#include "content/common/content_export.h"
#include "third_party/blink/public/platform/web_touch_event.h"

namespace content {

// Utility class for validating a stream of WebTouchEvents.
class CONTENT_EXPORT TouchEventStreamValidator {
 public:
  TouchEventStreamValidator();
  ~TouchEventStreamValidator();

  // If |event| is valid for the current stream, returns true.
  // Otherwise, returns false with a corresponding error message.
  bool Validate(const blink::WebTouchEvent& event, std::string* error_msg);

 private:
  blink::WebTouchEvent previous_event_;

  DISALLOW_COPY_AND_ASSIGN(TouchEventStreamValidator);
};

}  // namespace content

#endif  // CONTENT_COMMON_INPUT_TOUCH_EVENT_STREAM_VALIDATOR_H_
