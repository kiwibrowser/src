// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef THIRD_PARTY_BLINK_RENDERER_PLATFORM_ANIMATION_COMPOSITOR_FILTER_ANIMATION_CURVE_H_
#define THIRD_PARTY_BLINK_RENDERER_PLATFORM_ANIMATION_COMPOSITOR_FILTER_ANIMATION_CURVE_H_

#include <memory>

#include "base/memory/ptr_util.h"
#include "third_party/blink/renderer/platform/animation/compositor_animation_curve.h"
#include "third_party/blink/renderer/platform/animation/compositor_filter_keyframe.h"
#include "third_party/blink/renderer/platform/animation/timing_function.h"
#include "third_party/blink/renderer/platform/platform_export.h"
#include "third_party/blink/renderer/platform/wtf/noncopyable.h"

namespace cc {
class KeyframedFilterAnimationCurve;
}

namespace blink {
class CompositorFilterKeyframe;
}

namespace blink {

// A keyframed filter animation curve.
class PLATFORM_EXPORT CompositorFilterAnimationCurve
    : public CompositorAnimationCurve {
  WTF_MAKE_NONCOPYABLE(CompositorFilterAnimationCurve);

 public:
  static std::unique_ptr<CompositorFilterAnimationCurve> Create() {
    return base::WrapUnique(new CompositorFilterAnimationCurve());
  }
  ~CompositorFilterAnimationCurve() override;

  void AddKeyframe(const CompositorFilterKeyframe&);
  void SetTimingFunction(const TimingFunction&);
  void SetScaledDuration(double);

  // blink::CompositorAnimationCurve implementation.
  std::unique_ptr<cc::AnimationCurve> CloneToAnimationCurve() const override;

 private:
  CompositorFilterAnimationCurve();

  std::unique_ptr<cc::KeyframedFilterAnimationCurve> curve_;
};

}  // namespace blink

#endif  // THIRD_PARTY_BLINK_RENDERER_PLATFORM_ANIMATION_COMPOSITOR_FILTER_ANIMATION_CURVE_H_
