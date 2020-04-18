// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef THIRD_PARTY_BLINK_RENDERER_PLATFORM_ANIMATION_COMPOSITOR_ANIMATION_HOST_H_
#define THIRD_PARTY_BLINK_RENDERER_PLATFORM_ANIMATION_COMPOSITOR_ANIMATION_HOST_H_

#include "third_party/blink/renderer/platform/graphics/compositor_element_id.h"
#include "third_party/blink/renderer/platform/platform_export.h"
#include "third_party/blink/renderer/platform/wtf/noncopyable.h"
#include "ui/gfx/geometry/vector2d.h"

namespace cc {
class AnimationHost;
}

namespace blink {

class CompositorAnimationTimeline;

// A compositor representation for cc::AnimationHost.
class PLATFORM_EXPORT CompositorAnimationHost {
  WTF_MAKE_NONCOPYABLE(CompositorAnimationHost);

 public:
  explicit CompositorAnimationHost(cc::AnimationHost*);

  void AddTimeline(const CompositorAnimationTimeline&);
  void RemoveTimeline(const CompositorAnimationTimeline&);

  void AdjustImplOnlyScrollOffsetAnimation(CompositorElementId,
                                           const gfx::Vector2dF& adjustment);
  void TakeOverImplOnlyScrollOffsetAnimation(CompositorElementId);
  void SetAnimationCounts(size_t total_animations_count,
                          bool current_frame_had_raf,
                          bool next_frame_has_pending_raf);
  size_t GetMainThreadAnimationsCountForTesting();
  size_t GetCompositedAnimationsCountForTesting();
  bool CurrentFrameHadRAFForTesting();
  bool NextFrameHasPendingRAFForTesting();

 private:
  cc::AnimationHost* animation_host_;
};

}  // namespace blink

#endif  // CompositorAnimationTimeline_h
