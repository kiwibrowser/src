// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/modules/media_controls/elements/media_control_remaining_time_display_element.h"

#include "third_party/blink/renderer/modules/media_controls/media_controls_impl.h"

namespace blink {

MediaControlRemainingTimeDisplayElement::
    MediaControlRemainingTimeDisplayElement(MediaControlsImpl& media_controls)
    : MediaControlTimeDisplayElement(media_controls,
                                     kMediaTimeRemainingDisplay) {
  SetShadowPseudoId(
      AtomicString("-webkit-media-controls-time-remaining-display"));
}

String MediaControlRemainingTimeDisplayElement::FormatTime() const {
  // For the duration display, we prepend a "/ " to deliminate the current time
  // from the duration, e.g. "0:12 / 3:45".
  return "/ " + MediaControlTimeDisplayElement::FormatTime();
}

}  // namespace blink
