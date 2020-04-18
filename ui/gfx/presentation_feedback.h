// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UI_GFX_PRESENTAION_FEEDBACK_H_
#define UI_GFX_PRESENTAION_FEEDBACK_H_

#include <stdint.h>

#include "base/time/time.h"

namespace gfx {

// The feedback for gl::GLSurface methods |SwapBuffers|, |SwapBuffersAsync|,
// |SwapBuffersWithBounds|, |PostSubBuffer|, |PostSubBufferAsync|,
// |CommitOverlayPlanes|,|CommitOverlayPlanesAsync|, etc.
struct PresentationFeedback {
  PresentationFeedback() = default;
  PresentationFeedback(base::TimeTicks timestamp,
                       base::TimeDelta interval,
                       uint32_t flags)
      : timestamp(timestamp), interval(interval), flags(flags) {}

  // The time when a buffer begins scan-out. If a buffer is never presented on
  // a screen, the |timestamp| will be set to 0.
  base::TimeTicks timestamp;

  // An estimated interval from the |timestamp| to the next refresh.
  base::TimeDelta interval;

  enum Flags {
    // The presentation was synchronized to VSYNC.
    kVSync = 1 << 0,

    // The presentation |timestamp| is converted from hardware clock by driver.
    // Sampling a clock in user space is not acceptable for this flag.
    kHWClock = 1 << 1,

    // The display hardware signalled that it started using the new content. The
    // opposite of this is e.g. a timer being used to guess when the display
    // hardware has switched to the new image content.
    kHWCompletion = 1 << 2,

    // The presentation of this update was done zero-copy. Possible zero-copy
    // cases include direct scanout of a fullscreen surface and a surface on a
    // hardware overlay.
    kZeroCopy = 1 << 3,
  };

  // A combination of Flags. It indicates the kind of the |timestamp|.
  uint32_t flags = 0;
};

inline bool operator==(const PresentationFeedback& lhs,
                       const PresentationFeedback& rhs) {
  return lhs.timestamp == rhs.timestamp && lhs.interval == rhs.interval &&
         lhs.flags == rhs.flags;
}

inline bool operator!=(const PresentationFeedback& lhs,
                       const PresentationFeedback& rhs) {
  return !(lhs == rhs);
}

}  // namespace gfx

#endif  // UI_GFX_PRESENTAION_FEEDBACK_H_
