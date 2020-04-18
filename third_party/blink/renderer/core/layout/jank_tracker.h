// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef THIRD_PARTY_BLINK_RENDERER_CORE_LAYOUT_JANK_TRACKER_H_
#define THIRD_PARTY_BLINK_RENDERER_CORE_LAYOUT_JANK_TRACKER_H_

#include "third_party/blink/renderer/core/core_export.h"
#include "third_party/blink/renderer/platform/geometry/region.h"
#include "third_party/blink/renderer/platform/timer.h"
#include "third_party/blink/renderer/platform/wtf/allocator.h"

namespace blink {

class LayoutObject;
class LayoutRect;
class LocalFrameView;
class PaintLayer;

// Tracks "jank" from layout objects changing their visual location between
// animation frames.
class CORE_EXPORT JankTracker {
  DISALLOW_NEW();

 public:
  JankTracker(LocalFrameView*);
  ~JankTracker() {}
  void NotifyObjectPrePaint(const LayoutObject& object,
                            const LayoutRect& old_visual_rect,
                            const PaintLayer& painting_layer);
  void NotifyPrePaintFinished();
  bool IsActive();
  double Score() const { return score_; }
  float MaxDistance() const { return max_distance_; }

 private:
  void TimerFired(TimerBase*);

  // This owns us.
  UntracedMember<LocalFrameView> frame_view_;

  // The global jank score.
  double score_;

  // The per-frame jank region.
  Region region_;

  // Timer that fires the first time we've had no layout jank for a few seconds.
  TaskRunnerTimer<JankTracker> timer_;
  bool has_fired_;

  // The maximum distance any layout object has moved in any frame.
  float max_distance_;
};

}  // namespace blink

#endif  // THIRD_PARTY_BLINK_RENDERER_CORE_LAYOUT_JANK_TRACKER_H_
