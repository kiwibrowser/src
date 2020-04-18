// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/core/animation/timing.h"

namespace blink {

String Timing::FillModeString(FillMode fill_mode) {
  switch (fill_mode) {
    case FillMode::NONE:
      return "none";
    case FillMode::FORWARDS:
      return "forwards";
    case FillMode::BACKWARDS:
      return "backwards";
    case FillMode::BOTH:
      return "both";
    case FillMode::AUTO:
      return "auto";
  }
  NOTREACHED();
  return "none";
}

String Timing::PlaybackDirectionString(PlaybackDirection playback_direction) {
  switch (playback_direction) {
    case PlaybackDirection::NORMAL:
      return "normal";
    case PlaybackDirection::REVERSE:
      return "reverse";
    case PlaybackDirection::ALTERNATE_NORMAL:
      return "alternate";
    case PlaybackDirection::ALTERNATE_REVERSE:
      return "alternate-reverse";
  }
  NOTREACHED();
  return "normal";
}

}  // namespace blink
