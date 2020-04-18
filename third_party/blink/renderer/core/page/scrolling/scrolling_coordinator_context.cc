// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/core/page/scrolling/scrolling_coordinator_context.h"

#include "third_party/blink/renderer/core/paint/paint_layer.h"

namespace blink {

void ScrollingCoordinatorContext::SetAnimationTimeline(
    std::unique_ptr<CompositorAnimationTimeline> timeline) {
  animation_timeline_ = std::move(timeline);
}

void ScrollingCoordinatorContext::SetAnimationHost(
    std::unique_ptr<CompositorAnimationHost> host) {
  animation_host_ = std::move(host);
}

CompositorAnimationTimeline*
ScrollingCoordinatorContext::GetCompositorAnimationTimeline() {
  return animation_timeline_.get();
}

CompositorAnimationHost*
ScrollingCoordinatorContext::GetCompositorAnimationHost() {
  return animation_host_.get();
}

HashSet<const PaintLayer*>*
ScrollingCoordinatorContext::GetLayersWithTouchRects() {
  return &layers_with_touch_rects_;
}

bool ScrollingCoordinatorContext::ScrollGestureRegionIsDirty() const {
  return scroll_gesture_region_is_dirty_;
}

bool ScrollingCoordinatorContext::TouchEventTargetRectsAreDirty() const {
  return touch_event_target_rects_are_dirty_;
}

bool ScrollingCoordinatorContext::ShouldScrollOnMainThreadIsDirty() const {
  return should_scroll_on_main_thread_is_dirty_;
}

bool ScrollingCoordinatorContext::WasScrollable() const {
  return was_scrollable_;
}

void ScrollingCoordinatorContext::SetScrollGestureRegionIsDirty(bool dirty) {
  scroll_gesture_region_is_dirty_ = dirty;
}

void ScrollingCoordinatorContext::SetTouchEventTargetRectsAreDirty(bool dirty) {
  touch_event_target_rects_are_dirty_ = dirty;
}

void ScrollingCoordinatorContext::SetShouldScrollOnMainThreadIsDirty(
    bool dirty) {
  should_scroll_on_main_thread_is_dirty_ = dirty;
}

void ScrollingCoordinatorContext::SetWasScrollable(bool was_scrollable) {
  was_scrollable_ = was_scrollable;
}

}  // namespace blink
