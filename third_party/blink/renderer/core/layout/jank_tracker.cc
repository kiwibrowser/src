// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/core/layout/jank_tracker.h"

#include "third_party/blink/renderer/core/frame/local_dom_window.h"
#include "third_party/blink/renderer/core/frame/local_frame.h"
#include "third_party/blink/renderer/core/frame/location.h"
#include "third_party/blink/renderer/core/layout/layout_object.h"
#include "third_party/blink/renderer/core/layout/layout_view.h"
#include "third_party/blink/renderer/core/paint/paint_layer.h"
#include "third_party/blink/renderer/platform/graphics/paint/geometry_mapper.h"

namespace blink {

static const float kTimerDelay = 3.0;

static FloatPoint LogicalStart(const FloatRect& rect,
                               const LayoutObject& object) {
  const ComputedStyle* style = object.Style();
  DCHECK(style);
  auto logical =
      PhysicalToLogical<float>(style->GetWritingMode(), style->Direction(),
                               rect.Y(), rect.MaxX(), rect.MaxY(), rect.X());
  return FloatPoint(logical.InlineStart(), logical.BlockStart());
}

static float GetMoveDistance(const FloatRect& old_rect,
                             const FloatRect& new_rect,
                             const LayoutObject& object) {
  FloatSize location_delta =
      LogicalStart(new_rect, object) - LogicalStart(old_rect, object);
  return std::max(fabs(location_delta.Width()), fabs(location_delta.Height()));
}

JankTracker::JankTracker(LocalFrameView* frame_view)
    : frame_view_(frame_view),
      score_(0.0),
      timer_(frame_view->GetFrame().GetTaskRunner(TaskType::kInternalDefault),
             this,
             &JankTracker::TimerFired),
      has_fired_(false),
      max_distance_(0.0) {}

void JankTracker::NotifyObjectPrePaint(const LayoutObject& object,
                                       const LayoutRect& old_visual_rect,
                                       const PaintLayer& painting_layer) {
  if (!IsActive())
    return;

  LayoutRect new_visual_rect = object.FirstFragment().VisualRect();
  if (old_visual_rect.IsEmpty() || new_visual_rect.IsEmpty())
    return;

  if (LogicalStart(FloatRect(old_visual_rect), object) ==
      LogicalStart(FloatRect(new_visual_rect), object))
    return;

  const auto* local_transform = painting_layer.GetLayoutObject()
                                    .FirstFragment()
                                    .LocalBorderBoxProperties()
                                    .Transform();
  const auto* ancestor_transform = painting_layer.GetLayoutObject()
                                       .View()
                                       ->FirstFragment()
                                       .LocalBorderBoxProperties()
                                       .Transform();

  FloatRect old_visual_rect_abs = FloatRect(old_visual_rect);
  GeometryMapper::SourceToDestinationRect(local_transform, ancestor_transform,
                                          old_visual_rect_abs);

  FloatRect new_visual_rect_abs = FloatRect(new_visual_rect);
  GeometryMapper::SourceToDestinationRect(local_transform, ancestor_transform,
                                          new_visual_rect_abs);

  // TOOD(crbug.com/842282): Consider tracking a separate jank score for each
  // transform space to avoid these local-to-absolute conversions, once we have
  // a better idea of how to aggregate multiple scores for a page.
  // See review thread of http://crrev.com/c/1046155 for more details.

  IntRect viewport = frame_view_->GetScrollableArea()->VisibleContentRect();
  if (!old_visual_rect_abs.Intersects(viewport) &&
      !new_visual_rect_abs.Intersects(viewport))
    return;

  DVLOG(2) << object.DebugName() << " moved from "
           << old_visual_rect_abs.ToString() << " to "
           << new_visual_rect_abs.ToString();

  max_distance_ = std::max(
      max_distance_,
      GetMoveDistance(old_visual_rect_abs, new_visual_rect_abs, object));

  IntRect visible_old_visual_rect = RoundedIntRect(old_visual_rect_abs);
  visible_old_visual_rect.Intersect(viewport);

  IntRect visible_new_visual_rect = RoundedIntRect(new_visual_rect_abs);
  visible_new_visual_rect.Intersect(viewport);

  region_.Unite(Region(visible_old_visual_rect));
  region_.Unite(Region(visible_new_visual_rect));
}

void JankTracker::NotifyPrePaintFinished() {
  if (!IsActive())
    return;

  if (region_.IsEmpty()) {
    if (!timer_.IsActive())
      timer_.StartOneShot(kTimerDelay, FROM_HERE);
    return;
  }

  IntRect viewport = frame_view_->GetScrollableArea()->VisibleContentRect();
  double viewport_area = double(viewport.Width()) * double(viewport.Height());

  double jank_fraction = region_.Area() / viewport_area;
  score_ += jank_fraction;

  DVLOG(1) << "viewport " << (jank_fraction * 100)
           << "% janked, raising score to " << score_;

  TRACE_EVENT_INSTANT1("blink", "FrameLayoutJank", TRACE_EVENT_SCOPE_THREAD,
                       "viewportFraction", jank_fraction);

  region_ = Region();

  // This cancels any previously scheduled task from the same timer.
  timer_.StartOneShot(kTimerDelay, FROM_HERE);
}

bool JankTracker::IsActive() {
  // This eliminates noise from the private Page object created by
  // SVGImage::DataChanged.
  if (frame_view_->GetFrame().GetChromeClient().IsSVGImageChromeClient())
    return false;

  if (has_fired_)
    return false;
  return true;
}

void JankTracker::TimerFired(TimerBase* timer) {
  has_fired_ = true;

  // TODO(skobes): Aggregate jank scores from iframes.
  if (!frame_view_->GetFrame().IsMainFrame())
    return;

  DVLOG(1) << "final jank score for "
           << frame_view_->GetFrame().DomWindow()->location()->toString()
           << " is " << score_ << " with max move distance of "
           << max_distance_;

  TRACE_EVENT_INSTANT2("blink", "TotalLayoutJank", TRACE_EVENT_SCOPE_THREAD,
                       "score", score_, "maxDistance", max_distance_);
}

}  // namespace blink
