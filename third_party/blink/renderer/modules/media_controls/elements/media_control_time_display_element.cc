// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/modules/media_controls/elements/media_control_time_display_element.h"

#include "third_party/blink/renderer/bindings/core/v8/exception_state.h"
#include "third_party/blink/renderer/modules/media_controls/media_controls_impl.h"

namespace blink {

MediaControlTimeDisplayElement::MediaControlTimeDisplayElement(
    MediaControlsImpl& media_controls,
    MediaControlElementType display_type)
    : MediaControlDivElement(media_controls, display_type) {}

void MediaControlTimeDisplayElement::SetCurrentValue(double time) {
  current_value_ = time;
  setInnerText(FormatTime(), ASSERT_NO_EXCEPTION);
}

double MediaControlTimeDisplayElement::CurrentValue() const {
  return current_value_;
}

String MediaControlTimeDisplayElement::FormatTime() const {
  double time = std::isfinite(current_value_) ? current_value_ : 0;

  int seconds = static_cast<int>(fabs(time));
  int minutes = seconds / 60;
  int hours = minutes / 60;

  seconds %= 60;
  minutes %= 60;

  const char* negative_sign = (time < 0 ? "-" : "");

  // [0-10) minutes duration is m:ss
  // [10-60) minutes duration is mm:ss
  // [1-10) hours duration is h:mm:ss
  // [10-100) hours duration is hh:mm:ss
  // [100-1000) hours duration is hhh:mm:ss
  // etc.

  if (hours > 0) {
    return String::Format("%s%d:%02d:%02d", negative_sign, hours, minutes,
                          seconds);
  }

  return String::Format("%s%d:%02d", negative_sign, minutes, seconds);
}

}  // namespace blink
