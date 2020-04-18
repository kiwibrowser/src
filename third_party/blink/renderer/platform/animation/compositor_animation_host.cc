// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/platform/animation/compositor_animation_host.h"

#include "cc/animation/animation_host.h"
#include "cc/animation/scroll_offset_animations.h"
#include "third_party/blink/renderer/platform/animation/compositor_animation_timeline.h"

namespace blink {

CompositorAnimationHost::CompositorAnimationHost(cc::AnimationHost* host)
    : animation_host_(host) {
  DCHECK(animation_host_);
}

void CompositorAnimationHost::AddTimeline(
    const CompositorAnimationTimeline& timeline) {
  animation_host_->AddAnimationTimeline(timeline.GetAnimationTimeline());
}

void CompositorAnimationHost::RemoveTimeline(
    const CompositorAnimationTimeline& timeline) {
  animation_host_->RemoveAnimationTimeline(timeline.GetAnimationTimeline());
}

void CompositorAnimationHost::AdjustImplOnlyScrollOffsetAnimation(
    CompositorElementId element_id,
    const gfx::Vector2dF& adjustment) {
  animation_host_->scroll_offset_animations().AddAdjustmentUpdate(element_id,
                                                                  adjustment);
}

void CompositorAnimationHost::TakeOverImplOnlyScrollOffsetAnimation(
    CompositorElementId element_id) {
  animation_host_->scroll_offset_animations().AddTakeoverUpdate(element_id);
}

void CompositorAnimationHost::SetAnimationCounts(
    size_t total_animations_count,
    bool current_frame_had_raf,
    bool next_frame_has_pending_raf) {
  animation_host_->SetAnimationCounts(total_animations_count,
                                      current_frame_had_raf,
                                      next_frame_has_pending_raf);
}

size_t CompositorAnimationHost::GetMainThreadAnimationsCountForTesting() {
  return animation_host_->MainThreadAnimationsCount();
}

size_t CompositorAnimationHost::GetCompositedAnimationsCountForTesting() {
  return animation_host_->CompositedAnimationsCount();
}

bool CompositorAnimationHost::CurrentFrameHadRAFForTesting() {
  return animation_host_->CurrentFrameHadRAF();
}

bool CompositorAnimationHost::NextFrameHasPendingRAFForTesting() {
  return animation_host_->NextFrameHasPendingRAF();
}

}  // namespace blink
